#RedisCluster详细说明
欢迎来到RedisCluster详细说明。在这儿你可以找到RedisCluster的算法和设计理念。这个文档将会随着Redis的实现而同步更新。


为了下文中描述清楚，先讲一下脑裂(split brain) 。脑裂：如果发生通信故障，导致整个集群被分隔成多个无法互相通信的分区时，就是所谓的脑裂。既然是分区，分区就有大小之分，一个分区里面包含大多数的节点，那么就称之为majority partition；一个分区里面只有少量节点，那么就称之为minority partition。


##主要属性和设计逻辑
###RedisCluster的目标

RedisCluster是Redis的分布式实现，有以下目标，按照设计中的重要性排序：

+ 高性能和最高可到1000个节点的线性扩展。不需要代理，使用异步复制，对于value不进行merge操作。

+ 可接受的写安全：客户端连接majoritiy partition时，系统会尽可能维系写操作。通常会有少量通知成功的写操作会丢失。但当客户端连接的是minority partition的时候，通知成功但是失败的情况会更多。

+ 可用性：大多数主节点可用并且不可用的主节点至少有一个可用的从节点时，RedisCluster能够部分使用的。此外，使用复制转移（replicas migration），不再被任何从节点复制的主节点，会变为从节点。

## 已经实现的子集

RedisCluster实现了单节点版Redis的所有单个key的命令。只要key属于同一个节点，执行复杂多个key的操作如Set的unions或intersection也实现了。

RedisCluster实现了一个叫做``hash tags``的概念，使用``hash tags``能够强迫这些keys存储在同一个节点上。但是，在手工重分片（resharding）中，单个key的操作都是可用的，而多个key的操作可能在某些时候不可用。（比如当分片时，多个key分布在两个节点上时则不可用）

RedisCluster不支持单节点Redis的多数据库。只有一个数据库0，SELECT命令不被允许。

##RedisCluster协议中Client和Server的角色

在RedisCluster中，节点负责保存数据，记录集群的状态，包括将keys映射到正确的节点。集群节点能够自动发现其他节点，检测有问题的节点，当失败发生时推举从节点为主节点。为了执行这个任务，所有的集群节点通过TCP bus和二进制协议进行连接，称之为``Redis Cluster Bus``. 集群中每个节点通过``Redis Cluster Bus``进行连接。节点使用gossip 协议传播集群信息从而发现新的节点，发送ping包来确保所有其他节点都正常工作，发送集群消息来通知某些特定情况。使用Cluster Bus用于传播Pub/Sub消息，策划用户发出的手工错误转移。
因为集群节点不会转发请求，客户端使用重定向错误 -MOVED 和 -ASK，重定向到其他节点。理论上客户端可以向集群中任何一个节点发送请求，如果需要获取重定向，因此客户端不需要保存集群的状态。但是，客户端可以缓存节点和key的映射关系，从而改善性能。

## Write safety

RedisCluster节点间使用异步复制(replication)，最后的故障转移影响合并功能。这意味着最后推举的主节点的数据集 最后会取代其他所有的复制。在这个时间间隔中可能丢写。一个连接到大多数主节点和一个连接到少数主节点的客户端的窗口期是不同的。

RedisCluster会尽力维持连接大多数主节点的客户端的写操作。下面的例子描述的是在失败的时候，大多数主节点的通知写失败的场景：

1. 一个写到达master，但是当master回复client以后，这次写并没有通过异步复制传播给slave。如果在写到达slave之前，master宕机了，如果master宕机过久从而其从节点被推举为新的master以后，那么这次写就永久丢失了。 很难复现，但这确实是真实世界的失败模式。


2. 其他理论上可能写失败的情况如下：

+ 因为分隔，主节点不可达。
+ 从节点升为主
+ 过了一段时间，旧的主节点又可以了
+ 一个拥有过期路由表的客户端可能写到旧主节点，在旧主节点变为新主节点的从节点之前。


第二个失败情况不可能发生，因为在NODE_TIMEOUT时间内主节点不能跟majority中其他主节点通信的话，这个主节点就不再接受写。并且在修复以后，需要通知其他节点配置改变以后，才能接受写。并且这种失败情况还需要：客户端的路由表没有更新。



向minority partition写入时失败率很高。比如多个客户端向minority paritition写入，而majority partition认为minority partition已经失败，推举了minority的从节点为新的主节点，那么在这个过程中写入minority的写入就失败了。

Specifically, for a master to be failed over it must be unreachable by the majority of masters for at least NODE_TIMEOUT, so if the partition is fixed before that time, no writes are lost. When the partition lasts for more than NODE_TIMEOUT, all the writes performed in the minority side up to that point may be lost. However the minority side of a Redis Cluster will start refusing writes as soon as NODE_TIMEOUT time has elapsed without contact with the majority, so there is a maximum window after which the minority becomes no longer available. Hence, no writes are accepted or lost after that time.

特别地，在NODE_TIMEOUT内majority的主节点访问不到主节点时，该主节点被标记为failed，所以在NODE_TIMEOUT之前，被修复，那么写入不会丢失。当minority partition持续了NODE_TIMEOUT，所有写到minority的写入都会失败。但是，一旦过了NODE_TIMEOUT，那么RedisCluster会拒绝写入。因此，在那个时间以后，写入不被允许和丢失。【todo】



##Availability 可用性


对于minority partition， RedisCluster并不可用。对于majority partition（指的是主节点的大多数并且不可达的主节点都有一个从节点），在NODE_TIMEOUT+从节点推举以后，集群变得可用。

这意味着当集群中少量节点失败的时候，RedisCluster可用。面对large net splits时，RedisCluster并不是合适的解决方案。


In the example of a cluster composed of N master nodes where every node has a single slave, the majority side of the cluster will remain available as long as a single node is partitioned away, and will remain available with a probability of ``1-(1/(N*2-1))`` when two nodes are partitioned away (after the first node fails we are left with ``N*2-1`` nodes in total, and the probability of the only master without a replica to fail is ``1/(N*2-1))``.


例如，一个5主5从的集群，但过两个节点脱离集群后，可用性概率是``1/(5*2-1)`` = 11.11%，集群变得不可用。


多亏了RedisCluster的复制迁移特性，真实场景下，孤儿主节点（没有复制的主节点）的复制迁移改善了集群的可用性。对于每次失败事件的成功处理，集群都会重新配置从节点的布局。

##Performance


RedisCluster节点不会将请求代理到正确的节点，而是将客户端重定向到正确的节点。最后客户端获取了一份集群的最新描述，知道哪些节点服务哪些key的子集，所以对于普通操作客户端可以直接连接正确节点发送指定的命令。

因为使用了异步复制，节点不会等待其他节点的写通知（如果没有使用WAIT命令指定）。


因为操作多个key的命令限制为相近的key，除了resharding以外数据不会发生移动。


单个Redis实例可以准确控制单个操作。这意味着在N个主节点的RedisCluster中，你可以期待的性能是 单个RedisCluster * N 。在单个round trip中可以完成一次查询，因为客户端与节点间保持长连接，所以等待时间与单个Redis实例是一样的。

RedisCluster的主要目标是高性能和可扩展性，while preserving weak but reasonable forms of data safety and availability。


##为什么避免merge操作

Redis Cluster design avoids conflicting versions of the same key-value pair in multiple nodes as in the case of the Redis data model this is not always desirable. Values in Redis are often very large; it is common to see lists or sorted sets with millions of elements. Also data types are semantically complex. Transferring and merging these kind of values can be a major bottleneck and/or may require the non-trivial involvement of application-side logic, additional memory to store meta-data, and so forth.
There are no strict technological limits here. CRDTs or synchronously replicated state machines can model complex data types similar to Redis. However, the actual run time behavior of such systems would not be similar to Redis Cluster. Redis Cluster was designed in order to cover the exact use cases of the non-clustered Redis version.



#Overview of Redis Cluster main components


### keys分布模型

key的空间被分为16384 2^14个槽位，最大主节点个数是16384，但是建议的最大节点数是1000个。集群中每个主节点负责16384个hash槽位的一个子集。当没有正在进行的集群重新配置时（如hash槽位从一个节点挪到另外一个），集群是稳定的。集群是稳定时，单个hash槽位被单个节点服务。 (however the serving node can have one or more slaves that will replace it in the case of net splits or failures, and that can be used in order to scale read operations where reading stale data is acceptable)。



将keys映射到hash槽位的基本算法是（有hash tag的除外）：

	```HASH_SLOT = CRC16(key) mod 16384```

The CRC16 is specified as follows:

	Name: XMODEM (also known as ZMODEM or CRC-16/ACORN)
	Width: 16 bit
	Poly: 1021 (That is actually x16 + x12 + x5 + 1)
	Initialization: 0000
	Reflect Input byte: False
	Reflect Output CRC: False
	Xor constant to output CRC: 0000
	Output for "123456789": 31C3
	
14 out of 16 CRC16 output bits are used (this is why there is a modulo 16384 operation in the formula above).
In our tests CRC16 behaved remarkably well in distributing different kinds of keys evenly across the 16384 slots.
Note: A reference implementation of the CRC16 algorithm used is available in the Appendix A of this document.


### Keys hash tags


有hash tags时 hash槽位的计算不使用上面的公式。``Hash tags``确保多个key可以位于同一个hash槽位中。这用于在RedisCluster中实现多个key的操作。


为了实现hash tags，在某些情况下，计算key的hash槽位有些不同。如果一个key包含了"{...}"模式，只有在{和}之间的子串被用来计算hash槽位。但是，因为{和}会多次出现，算法按照下面的规则指定：

	如果一个key包含了{字符。
	并且如果在{的右侧有一个}字符
	并如果在第一个{和第一个}之间有一个或多个字符，
	那么只是第一个{和第一个}之间的字符被用来计算hash。

举例：

两个key{user1000}.following 和 {user1000}.followers将会被hash到相同的hash操作，因为只有子串user1000被用来计算hash槽位。

对于key ``foo{}{bar}``整个key都被用来计算hash，因为第一个{和}之间没有字符。
对于key``foo{{bar}}`` 用来计算hash的是``{bar``。
对于key ``foo{bar}{zap}子串bar被用来计算hash。因为算法在匹配到第一个{和}以后就停止匹配了。
因此如果key是以{}开始，那么保证这个key被hash到整个集群，对于使用二进制数据作为key的名字十分有用。

Adding the hash tags exception, the following is an implementation of the HASH_SLOT function in Ruby and C language.
Ruby example code:

	def HASH_SLOT(key)
	    s = key.index "{"
	    if s
	        e = key.index "}",s+1
	        if e && e != s+1
	            key = key[s+1..e-1]
	        end
	    end
	    crc16(key) % 16384
	end
C example code:
unsigned int HASH_SLOT(char *key, int keylen) {
    int s, e; /* start-end indexes of { and } */

    /* Search the first occurrence of '{'. */
    for (s = 0; s < keylen; s++)
        if (key[s] == '{') break;

    /* No '{' ? Hash the whole key. This is the base case. */
    if (s == keylen) return crc16(key,keylen) & 16383;

    /* '{' found? Check if we have the corresponding '}'. */
    for (e = s+1; e < keylen; e++)
        if (key[e] == '}') break;

    /* No '}' or nothing between {} ? Hash the whole key. */
    if (e == keylen || e == s+1) return crc16(key,keylen) & 16383;

    /* If we are here there is both a { and a } on its right. Hash
     * what is in the middle between { and }. */
    return crc16(key+s+1,e-s-1) & 16383;
}


## Cluster nodes attributes

集群中每个节点有一个唯一的名字。这个节点的名字是一个160位随机数字的16进制表示，节点首次启动的时候获得。节点将其id保存在节点配置文件中，只要系统管理员不删除节点配置或者不使用CLUSTER RESET命令的话，将永远使用这个相同的ID。

整个集群中使用节点ID来标识每一个节点。对于一个指定节点可以无需改变节点ID，即可改变其IP地址。集群可以检测到IP/PORT的改变并在cluster bus中使用gossip协议进行重新配置。


每个节点的信息不仅仅是节点ID，但节点ID是唯一全局不变的。每个节点还有以下信息与之关联。如这个特定节点的集群配置细节，节点ping的最后时间。


每个节点维护其他节点的以下信息：节点ID，IP，port，一组flags，是否是主节点，节点ping的最后时间，收到pong的最后时间，节点现在的configuration epoch（这点一会儿详细解释），连接状态，所服务的hash槽位的集合。


所有节点字段的详细字段在[CLUSTER NODES](http://redis.io/commands/cluster-nodes)中详细描述。

[CLUSTER NODES](http://redis.io/commands/cluster-nodes)命令可以发送到集群中的任何一个节点，提供整个集群的状态和每个节点的信息。
下面是[CLUSTER NODES](http://redis.io/commands/cluster-nodes)命令的输出示例，集群中只有三个节点：
	
	$ redis-cli cluster nodes
	d1861060fe6a534d42d8a19aeb36600e18785e04 127.0.0.1:6379 myself - 0 1318428930 1 connected 0-1364
	3886e65cc906bfd9b1f7e7bde468726a052d1dae 127.0.0.1:6380 master - 1318428930 1318428931 2 connected 1365-2729
	d289c575dcbc4bdd2931585fd4339089e461a27d 127.0.0.1:6381 master - 1318428931 1318428931 3 connected 2730-4095
	
上面列表中按照顺序字段依次是:

	<id> <ip:port> <flags> <master> <ping-sent> <pong-recv> <config-epoch> <link-state> <slot> <slot> ... <slot>


##The Cluster bus

每个RedisCluster 节点都有另外一个TCP端口用于接收来自其他RedisCluster节点的连接。这个端口与接收客户端连接的正常端口有一个固定偏移。为了取得RedisCluster端口，10000通常是固定偏移。例如，如果一个Redis节点用于客户端的端口是6379，那么Cluster bus的端口16379通常也被打开。


节点与节点通信仅仅使用Cluster bus和Cluster bus协议：一个由不同类型和字节的帧组成的二进制协议。Cluster bus二进制协议没有公开的文档，因为并不需要外部的软件使用该协议与RedisCluster节点通信。当然，你可以通过源码中的cluster.h和cluster.c阅读。

##Cluster topology
Redis Cluster is a full mesh where every node is connected with every other node using a TCP connection.
In a cluster of N nodes, every node has N-1 outgoing TCP connections, and N-1 incoming connections.
These TCP connections are kept alive all the time and are not created on demand. When a node expects a pong reply in response to a ping in the cluster bus, before waiting long enough to mark the node as unreachable, it will try to refresh the connection with the node by reconnecting from scratch.

RedisCluster是一个完整的网，每个节点与其他节点通过TCP连接进行连接。在一个N个节点的集群中，每个节点有N-1个向外的TCP连接和N-1个向内的连接【TODO outgoing, incoming。这些TCP连接一直保活，并且不会按需创建。 When a node expects a pong reply in response to a ping in the cluster bus, before waiting long enough to mark the node as unreachable, it will try to refresh the connection with the node by reconnecting from scratch.

当RedisCluster节点组成一个完整的网，节点使用gossip协议和配置更新机制来避免正常情况下过多信息在节点间交换，所以交换的消息的数量不是指数的。
## Nodes handshake

节点通常在节点bus端口上接收连接，并且 如果pinging的节点不被信任，当收到ping以后会做出相应。但是如果发送消息的节点不是集群的一部分，那么收到消息的节点会丢弃其他包。
【todo】上面这段是什么意思呢？


只在两种方式下，一个节点会接收另外一个节点作为集群的一部分：
如果一个节点使用MEET消息代表自己【todo】。一个meet消息像一个PING消息，但是会强迫接收者去将发送的节点作为集群的一部分。只要系统管理员通过下面命令进行请求，节点将会发送MEET消息给其他节点：

	CLUSTER MEET ip port


如果被信任的节点发送gossip到其他节点，那么节点也会将其他节点注册为集群的一部分。假如A知道B，B知道C，最后B会发送gossip消息给A和C。当这个发生以后，A将会注册C作为网络的一部分，并尝试去连接C。

这意味着只要在任何连接图中加入节点，他们最后会自动形成一个完整的连接。这意味着节点能够自动发现其他节点，但需要由系统管理员来指定信任关系。

这种机制使得集群更加健壮但是but prevents different Redis clusters from accidentally mixing after change of IP addresses or other network related events。


# 重定向和分片
##  MOVED重定向


一个Redis客户端可以向集群中的每个节点（包括从节点）发送请求。节点分析请求，节点通过判断key所属于的hash槽位来判断是否接收这个请求。如果这个节点负责这个hash槽位，那么请求被处理；反之，节点会检查内部的hash槽位和节点的映射关系，然后返回给客户端一个MOVED错误。下面的例子：
	
	GET x
	-MOVED 3999 127.0.0.1:6381
错误中包含了key的hash槽位（3999）和负责这个请求的节点的ip:port。客户端需要将这个请求重发到特定的IP地址和端口。	注意到，如果在客户端重发这个请求之前，cluster配置发生改变，如果3999的hash槽位被其他节点6382服务，那么当客户端重发这个请求时，目标节点将又会恢复MOVED错误。


从cluster角度来看，节点是通过IDs来标识的；与此同时，我们尝试简化客户端的接口，只需要将槽位与IP:PORT对标识的Redis节点间建立映射关系即可。客户端不需要但是应该记住hash槽位3999与服务器的对应关系。



一种方案是当收到MOVED重定向的时候，使用 CLUSTER NODES或CLUSTER SLOTS来更新整个客户端的集群布局。当重定向发生时，可能是多个而不是一个槽位被重新配置，因此尽可能的更新客户端配置是最好的策略。



注意到当集群稳定了（配置中没有持续的改动），最后所有的客户端会维持一份hash槽位到节点的映射关系，因为客户端不需要重定向，代理或其他单点失败的情况，就能直接到达正确节点，从而使得集群更加高效。

一个客户端必须能够处理 -ASK重定向。下面将会描述这种情况，否则就不是一个完整的Cluster客户端。



### 集群运行重新配置 


RedisCluster支持在集群运行时添加和删除节点。添加或删除一个节点可以抽象为相同的操作：将hash槽位从一个节点挪到另外一个节点。这意味着可以使用相同的基本的机制来重新均衡集群，添加或删除节点。为了向集群添加新的节点，首先在集群中添加一个空白节点，从已经存在的节点中挪一些hash槽位到新的节点。

为了从集群中挪出节点，将位于这个节点的hash槽位挪到已经存在的其他节点。

为了重新均衡集群，一组hash槽位在节点间被移动。

实现的核心是能够挪走hash槽位。从实践的角度来看，一个hash槽位是一组keys，因此重新分片（resharding）就是将key从一个实例挪到另外一个实例。移动一个hash槽位意味着移动这个槽位上的所有keys。为了明白这是如何工作的，讲一下在RedisCluster节点间用于操作槽位转移的CLUSTER子命令。

下面是可选的子命令：
CLUSTER ADDSLOTS slot1 [slot2] ... [slotN]
CLUSTER DELSLOTS slot1 [slot2] ... [slotN]
CLUSTER SETSLOT slot NODE node
CLUSTER SETSLOT slot MIGRATING node
CLUSTER SETSLOT slot IMPORTING node


前两个命令，ADDSLOT和DELSLOTS用于分配（或移除）一个节点的slot槽位。分配一个槽位意味着告诉特定主节点，它将要负责存储和服务特定hash槽位。
当hash槽位被分配以后，将会使用gossip协议在集群中广播，这部分将会在传播这一章节中详细介绍。


当创建一个新集群时，通常使用ADDSLOTS为每个主节点分配所有16384个槽位中的一个子集。

DELSLOTS通常用于集群配置的手工修改或者用于调试：线上基本不用。

如果使用的是 SETSLOT <slot> NODE格式，SETSLOT子命令用于将槽位分配给特定的nodeID 。否则，槽位会被设定为两个特定状态MIGRATING和IMPORTING。这两个特别的状态用于将hash槽位从一个节点迁移到另外一个节点。

当一个槽位设置为MIGRATING时，当key存在的时候，节点将会接收这个hash槽位的所有请求。反之，会使用一个-ASK重定向，将其重定向到迁移的目标节点。

当一个槽位设置为IMPORTING时，只有先发送ASKING命令，节点会接收这个hash槽位的所有请求。如果客户端没有发出ASKING命令，那么会通过-MOVED重定向错误重定向到真正的hash 槽位。


让我们举一个hash槽位迁移的例子。假设我们有两个RedisCluster节点A和B。我们想将槽位8从A挪到B，因此我们发出命令：


	We send B: CLUSTER SETSLOT 8 IMPORTING A
	We send A: CLUSTER SETSLOT 8 MIGRATING B

All the other nodes will continue to point clients to node "A" every time they are queried with a key that belongs to hash slot 8, so what happens is that:

当请求的key属于槽位8时，其他所有节点会将客户端重定向到节点A，因此将会发生下面的事情：

节点A中存在的key的所有查询由节点A处理
节点A中不存在的key的所有查询由B处理，因为A会将客户端重定向到B。

这种方式不会在A中创建新的key。与此同时，在重新分片过程中，一个称为redis-trib的程序将会将槽位8中的存在的key从节点A迁移到节点B。通过下面的命令执行：

	CLUSTER GETKEYSINSLOT slot count


上面的命令会返回指定hash槽位中key的个数。对于返回的每个key，redis-trib会向节点A发送 MIGRATE命令，将特定的key以原子方式从节点A移到节点B（在迁移过程中两个实例都要被锁住，从而避免竞争的情况）。MIGRATE工作情况：
	
	MIGRATE target_host target_port key target_database id timeout

MIGRATE会连接目标实例，发送key的序列化版本，一旦收到OK，就会从自己的数据集中删除老的key。在外部客户端来看，在任何时间key只会在A或B中存在。


RedisCluster中不需要指定database，但是MIGRATE是一个普通命令。优化过的MIGRATE可以非常快的进行移动，MIGRATE is optimized to be as fast as possible even when moving complex keys such as long lists, but in Redis Cluster reconfiguring the cluster where big keys are present is not considered a wise procedure if there are latency constraints in the application using the database.



当迁移过程完成以后， ``SETSLOT <slot> NODE <node-id>`` 命令被发送到迁移中的两个节点，从而将槽位设置为正常状态。相同的命令被发送任意其他的节点，从而避免等待新配置在集群中的自然传播。【todo最后一句不懂】
##ASK redirection


在之前的章节中，我们简单讨论了ASK重定向。为什么我们不使用MOVED重定向呢？因为使用MOVED意味着我们认为hash槽位已经永久地位于不同的节点B上了，那么下次请求就会到达这个不同节点B；而ASK意味着只是这一次请求到达不同节点B。



这样做事必须的，因为下一次hash槽位8的请求可能是一个槽位依然在A的key，因此我们希望客户端先去尝试A，如果有必要再去尝试B。因为只是一个槽位，因此cluster的性能是可以接受的额。

We need to force that client behavior, so to make sure that clients will only try node B after A was tried, node B will only accept queries of a slot that is set as IMPORTING if the client sends the ASKING command before sending the query.

我们需要强制指定客户端行为，确保客户端只能是在试完A以后再尝试的B，节点B只会接收某种槽位的请求，这种槽位需要满足：槽位设置为IMPORTING，并且客户端在发送请求之前先发送了ASKING命令。


ASKING命令在客户端上会设置一次性的标识，强迫节点去服务一个IMPORTING节点的请求。

从客户端角度来看，ASK重定向的完整语义是：

+ 如果收到ASK重定向，这是这次请求被重定向到特定的节点，其他请求都被发送到老的节点。

+ 使用ASKING名开始重定向的请求
+ 不会更新本地客户端的映射表，不会将hash 槽位8映射到B。

一旦完成hash槽位8的迁移，节点A就会发送MOVED消息，客户端会永久的将hash槽位8映射到新的ip:port。如果客户端在完成迁移之前，将槽位8映射到新的ip:port也不会有问题，因为在发送请求之前没有发送ASKING命令，因此节点B会使用MOVED重定向错误将客户端重定向到A。【注：如果客户端不发送ASKING命令，就死循环了】

在[CLUSTER SETSLOT](http://redis.io/commands/cluster-setslot)命令文档中，slots迁移以相似的术语但是不同的词语进行了解释，主要是考虑到了文档的冗余。



##客户端首次连接和重定向的控制


一个RedisCluster客户端的实现可以在内存中``不记录``槽位配置信息（即槽位与节点地址对应关系），而是连接任意节点等待重定向，这样的客户端很低效。


RedisCluster客户端应该足够智能的去记录槽位配置信息。但是，这个配置并不需要一直维持最新的。因为连接到错误的节点只是简单的导致重定向，将会触发客户端的一次更新。


客户端在两种不同情况下可以获取到一份完整的节点配置信息：

+ 启动时获得初始的槽位配置信息
+ 收到一个MOVED重定向时

注意到客户端收到MOVED重定向后，可以只是更新映射表中这一个移动的槽位，但因为一次调整通常会有多个槽位发生改变（比如从节点推举为主，原有主的槽位都要发生改变），因此这样并不高效。简单的方法是收到MOVED重定向以后，重新获取一份完整的节点映射关系。

为了获取RedisCluster的槽位配置信息，RedisCluster提供了一个新的命令[CLUSTER SLOTS](http://redis.io/commands/cluster-slots)。这个命令不需要解析，只是提供客户端需要的信息，提供的是一个数组，数组的每个元素是槽位范围和与之关联的master和slave节点。下面是[CLUSTER SLOTS](http://redis.io/commands/cluster-slots)的示例：

	127.0.0.1:7000> cluster slots
	1) 1) (integer) 5461
	   2) (integer) 10922
	   3) 1) "127.0.0.1"
	      2) (integer) 7001
	   4) 1) "127.0.0.1"
	      2) (integer) 7004
	2) 1) (integer) 0
	   2) (integer) 5460
	   3) 1) "127.0.0.1"
	      2) (integer) 7000
	   4) 1) "127.0.0.1"
	      2) (integer) 7003
	3) 1) (integer) 10923
	   2) (integer) 16383
	   3) 1) "127.0.0.1"
	      2) (integer) 7002
	   4) 1) "127.0.0.1"
	      2) (integer) 7005
	      
	      



返回数组的每个元素的前两个子元素是槽位起始范围，另外的元素是address-port对。第一个address-port对是主节点，其余都是从节点。

例如输出第一个元素是指：槽位5461到10922是被127.0.0.1:7001服务，也可以在127.0.0.1:7004上进行只读操作。

如果集群配置错误，[CLUSTER SLOTS](http://redis.io/commands/cluster-slots)不能保证返回的范围覆盖了全部的16384个槽位，因此客户端需要初始化槽位配置，使用NULL对象来填充目标节点，如果客户去执行了没有分配的槽位的话客户端需要报错。

发现槽位没有被分配，在给调用者发送错误之前，客户端应该尝试拉取一次槽位配置来检查集群是否已经正确配置。


## 多个key的操作


使用hash tags，客户端能够使用multi-key操作。下面的例子是有效的：

	MSET {user:1000}.name Angela {user:1000}.surname White
当hash槽位重新分片正在进行的时候，multi-key的操作可能不可用。

如果操作的keys不存在或者在分片进行时keys被分隔在源节点和目标节点中，那么将会产生一个TRYAGAIN错误。过一段时间以后，客户端尝试或者返回错误。

只要特定hash槽位的迁移完成，对于这个hash槽位的multi-key操作又可以了。

## 使用从节点扩展读


正常情况下，对于给定的命令，从节点会将客户端重定向到hash槽位所在的主节点，但是可以使用从节点通过[READONLY](http://redis.io/commands/readonly)命令来水平扩展读的能力。[READONLY](http://redis.io/commands/readonly)告诉RedisCluster从节点，客户端可以读取数据但是不能写数据。

当连接是只读模式时，只有key不位于从节点的主节点的时候，集群才会发送一个重定向。发生的原因是：

+  当从节点的主节点没有负责这个槽位时，
+  集群被重新配置，从节点不再负责这个特定的槽位时，

当这种情况发生时，客户端要按照之前章节解释的来更新其hash槽位的映射。使用READWRITE命令可以去除连接的只读状态。

#容错（Fault Tolerance）
## 心跳和流言消息（Heartbeat and gossip messages）

RedisCluster节点持续地交换ping和pong包。这两种类型的包有相同的结构，运载着重要的配置信息。唯一不同的是消息的类型字段。我们将ping和pong包合称为心跳包。

通常节点发送ping包，ping包会触发接收者回应一个pong包。但是这不必是真的。对于节点而言，可以带着他们的配置向其他节点发送pong包，而不必触发回应。例如，这对于广播新配置很有用。
通常一个节点每秒会ping一些节点，因此不管集群中节点数量多少，每个节点发送的ping包数据是固定的。
如果有些节点other在``NODE_TIMEOUT/2``时间内没有被节点A ping过，那么节点A会在``NODE_TIMEOUT/2``时间后向这些other节点发送ping。在NODE_TIMEOUT到期之前，节点户尝试连接其他节点，从而确保节点的不可达不仅仅是当前TCP连接的问题。


如果NODE_TIMEOUT设置的很小并且节点数N很大，那么交换的信息非常大，因为在``NODE_TIMEOUT/2``时间内每个节点会尝试去ping其他节点。

例如一个100个节点的集群，集群超时时间设置为60妙，那么每个节点每30秒要发送99个pings，即每秒3.3个pings。乘以100个节点，那么整个集群每秒要发送330个ping包。

有很多方法可以减少消息的数量，但是目前还没有关于RedisCluster 错误检测所带来的带宽问题的issues，因此使用明确和直接的设计。注意，即使在上面的例子中，每秒交换330个包被分散在100个不同节点上，因此可以接受。


##Heartbeat packet content
Ping and pong packets contain a header that is common to all types of packets (for instance packets to request a failover vote), and a special Gossip Section that is specific of Ping and Pong packets.
ping和pong包包含了一个所有类型包通用的头部，以及一个具体的gossip section。

通用头部有下列信息：
+ Node ID。节点最开始创建的时候分配的一个160字节的随机字符串。在集群整个生命期中，Node ID不变。
+ The currentEpoch and configEpoch fields of the sending node that are used to mount the distributed algorithms used by Redis Cluster (this is explained in detail in the next sections). If the node is a slave the configEpoch is the last known configEpoch of its master.
+ The node flags, indicating if the node is a slave, a master, and other single-bit node information.
+ A bitmap of the hash slots served by the sending node, or if the node is a slave, a bitmap of the slots served by its master.
+ The sender TCP base port (that is, the port used by Redis to accept client commands; add 10000 to this to obtain the cluster bus port).
The state of the cluster from the point of view of the sender (down or ok).
+ The master node ID of the sending node, if it is a slave.



Ping和Pong包通常包含一个gossip 部分。This section offers to the receiver a view of what the sender node thinks about other nodes in the cluster.The gossip section only contains information about a few random nodes among the set of nodes known to the sender. gossip部分中提到的节点数量与集群的大小是成比例的。

gossip section中包含节点以下字段：

+ Node ID.
+ IP and port of the node.
+ Node flags.


Gossip sections allow receiving nodes to get information about the state of other nodes from the point of view of the sender. This is useful both for failure detection and to discover other nodes in the cluster.

Gossip章节允许接收的节点去获取其他节点状态的信息。这对于错误检查和发现集群中的其他节点是有用的。


## 错误检测



RedisCluster错误检测用于发现：当大多数节点认为一个master或slave节点 不可用时，会推举一个slave节点作为master节点。 当``从推举``不可能的时候，集群会进入一个错误状态，从而阻止接收来自客户端的请求。

正如之前提到的，每个节点获取其他已知节点的一组flags。有两种用于错误检测的flag，称为PFAIL和FAIL。PFAIL以为这可能失败，是一种non-acknowledged失败类型。FAIL意味着一个节点失败了，并且在固定时间内被大多数主节点确认过失败的。


### PFAIL flag:


当超过NODE_TIMEOUT时间，节点不可达的时候，一个节点会使用PFAIL来标识其他节点。不管其类型是什么，主节点和从节点都可以标识其他节点为PFAIL。

在RedisCluster节点中``不可达``的概念是：一个active ping超过NODE_TIMEOUT后依然没有收到响应。这种机制下，NODE_TIMEOUT要大于网络的往返时间（round trip time）。


在正常操作下，为了增加可靠性，一旦超过NODE_TIMEOUT的一半还没有收到ping的响应，那么节点会尝试与其他节点进行重连。这种机制确保连接保活，错误的连接不会导致错误的节点错误报告。


### FAIL flag：


PFAIL标识是每个节点记录其他节点的本地信息，这不足以触发从节点的推举。对于一个PFAIL状态的节点会升级为FAIL状态。

正如文档中节点心跳部分概括的，每个节点会向其他节点发送gossip消息，这些消息包含了其他已知节点的状态。每个节点最后接收一组其他节点的节点标识。This way every node has a mechanism to signal other nodes about failure conditions they have detected.



当下列条件满足的时候PFAIL条件会升级为FAIL条件：
+ 节点A已经将节点B标识为PFAIL。
+ 通过gossip部分，节点A从集群中大多数主节点那儿收集了节点B的状态。
+ 在``NODE_TIMEOUT * FAIL_REPORT_VALIDITY_MULT``时间内，多数master节点标记为PFAIL或PFAIL条件（现在的实现中正确的系数设置为2，因此是2倍的NODE_TIMEOUT时间）。

如果上面的条件为真，节点A将会：
+ 将节点标记为FAIL
+ 向所有可以到达的节点发送FAIL消息。

FAIL消息会强迫每个收到消息的节点将这个节点A标识为FAIL状态，而不管这些收到消息的节点是否标识节点A为PFAIL状态。注意到FAIL标识通常是一个方式。节点可以从PFAIL到FAIL状态，但FAIL状态只能在下面情况下被清除：

+

The FAIL message will force every receiving node to mark the node in FAIL state, whether or not it already flagged the node in PFAIL state.
Note that the FAIL flag is mostly one way. That is, a node can go from PFAIL to FAIL, but a FAIL flag can only be cleared in the following situations:
The node is already reachable and is a slave. In this case the FAIL flag can be cleared as slaves are not failed over.
The node is already reachable and is a master not serving any slot. In this case the FAIL flag can be cleared as masters without slots do not really participate in the cluster and are waiting to be configured in order to join the cluster.
The node is already reachable and is a master, but a long time (N times the NODE_TIMEOUT) has elapsed without any detectable slave promotion. It's better for it to rejoin the cluster and continue in this case.
It is useful to note that while the PFAIL -> FAIL transition uses a form of agreement, the agreement used is weak:
Nodes collect views of other nodes over some time period, so even if the majority of master nodes need to "agree", actually this is just state that we collected from different nodes at different times and we are not sure, nor we require, that at a given moment the majority of masters agreed. However we discard failure reports which are old, so the failure was signaled by the majority of masters within a window of time.
While every node detecting the FAIL condition will force that condition on other nodes in the cluster using the FAIL message, there is no way to ensure the message will reach all the nodes. For instance a node may detect the FAIL condition and because of a partition will not be able to reach any other node.
However the Redis Cluster failure detection has a liveness requirement: eventually all the nodes should agree about the state of a given node. There are two cases that can originate from split brain conditions. Either some minority of nodes believe the node is in FAIL state, or a minority of nodes believe the node is not in FAIL state. In both the cases eventually the cluster will have a single view of the state of a given node:
Case 1: If a majority of masters have flagged a node as FAIL, because of failure detection and the chain effect it generates, every other node will eventually flag the master as FAIL, since in the specified window of time enough failures will be reported.
Case 2: When only a minority of masters have flagged a node as FAIL, the slave promotion will not happen (as it uses a more formal algorithm that makes sure everybody knows about the promotion eventually) and every node will clear the FAIL state as per the FAIL state clearing rules above (i.e. no promotion after N times the NODE_TIMEOUT has elapsed).
The FAIL flag is only used as a trigger to run the safe part of the algorithm for the slave promotion. In theory a slave may act independently and start a slave promotion when its master is not reachable, and wait for the masters to refuse to provide the acknowledgment if the master is actually reachable by the majority. However the added complexity of the PFAIL -> FAIL state, the weak agreement, and the FAIL message forcing the propagation of the state in the shortest amount of time in the reachable part of the cluster, have practical advantages. Because of these mechanisms, usually all the nodes will stop accepting writes at about the same time if the cluster is in an error state. This is a desirable feature from the point of view of applications using Redis Cluster. Also erroneous election attempts initiated by slaves that can't reach its master due to local problems (the master is otherwise reachable by the majority of other master nodes) are avoided.
#Configuration handling, propagation, and failovers
## 集群当前时代（Cluster current epoch）

RedisCluster使用一个类似Raft算中的概念，在RedisCluster中这个术语叫做时代（epoch），其目的是给予事件一个增长的版本。当多节点提供冲突消息时，对于其他节点可以获取到最新的状态。


####当前时代（currentEpoch）是一个64位的无符号数字
At node creation every Redis Cluster node, both slaves and master nodes, set the currentEpoch to 0.

在RedisCluster节点创建的时候，从节点和主节点currentEpoch都是0.
每次从其他节点收到包，如果发送者的epoch大于本地节点epoch，那么currentEpoch就会更新为发送者的epoch。因为这样的话，最后所有节点都会得到集群中的最大的configEpoch。
当交换集群状态和一个节点执行某些操作的时候回使用这个信息。现在只有在``从节点推举``的时候才会发生。epoch对于集群而言是一个逻辑锁，面对小的epoch时，所提供的信息以大的epoch的信息为准。


## 配置时代（Configuration epoch）
Every master always advertises its configEpoch in ping and pong packets along with a bitmap advertising the set of slots it serves.
The configEpoch is set to zero in masters when a new node is created.
A new configEpoch is created during slave election. Slaves trying to replace failing masters increment their epoch and try to get authorization from a majority of masters. When a slave is authorized, a new unique configEpoch is created and the slave turns into a master using the new configEpoch.
As explained in the next sections the configEpoch helps to resolve conflicts when different nodes claim divergent configurations (a condition that may happen because of network partitions and node failures).
Slave nodes also advertise the configEpoch field in ping and pong packets, but in the case of slaves the field represents the configEpoch of its master as of the last time they exchanged packets. This allows other instances to detect when a slave has an old configuration that needs to be updated (master nodes will not grant votes to slaves with an old configuration).
Every time the configEpoch changes for some known node, it is permanently stored in the nodes.conf file by all the nodes that receive this information. The same also happens for the currentEpoch value. These two variables are guaranteed to be saved and fsync-ed to disk when updated before a node continues its operations.
The configEpoch values generated using a simple algorithm during failovers are guaranteed to be new, incremental, and unique.
##从节点选举和推举 Slave election and promotion


从节点的选举和推举是由从节点操作，由主节点投票，推举从节点。当主节点的状态是FAIL，并且主节点有从节点时，从节点会进行选举并有可能成为一个主节点。

对于一个从节点，想成为一个主节点的话，需要开始一个选举并且赢取选举。当主节点状态是FAIL时，该master的所有从节点会开始选举，但是只有一个从节点会赢得选举并推举自己为主节点。

当下来条件满足时，一个从节点开始选举：

+ 从节点的主节点是FAIL状态
+ 主节点负责的槽位个数大于0
+ 从节点与主节点的复制连接已经断开一段时间，目的是为了保证被推举从节点的的数据足够新。这个时间是用户可配置的

为了进行选举，从节点的额第一步是增加currentEpoch计数器，并请求主节点实例的投票。

从节点向集群中每个主节点发送 FAILOVER_AUTH_REQUEST的包，从而发起投票的请求。之后等待投票的最长时间是2*NODE_TIMEOUT。

一旦一个主节点为某个从节点投票以后，即回复FAILOVER_AUTH_ACK的包，那么这个主节点在2*NODE_TIMEOUT时间内就不能投票该主节点的其他从节点了。 这不是为了保证安全，但可以防止多个从节点同时被推举，这不是我们想要的。

当投票请求中的epoch小于节点的currentEpoch时，从节点放弃AUTH_ACK响应。这确保了节点不会统计之前推举的投票。

一旦从节点收到大部分主节点的ACKS，那么其赢取选举。如果在``2*NODE_TIMEOUT``时间内，没有收到大多数节点的ACKS，那么就会放弃这次选举，并在``4*NODE_TIMEOUT``后开始新的推举。




##Slave rank
As soon as a master is in FAIL state, a slave waits a short period of time before trying to get elected. That delay is computed as follows:
一旦主节点处于FAIL状态，那么从节点需要在短时间等待以后才会尝试选举，这个时延计算如下：


``DELAY = 500 milliseconds + random delay between 0 and 500 milliseconds +
        SLAVE_RANK * 1000 milliseconds``.


固定的时延确保FAIL状态能够在集群中传播，否则其他主节点没有收到FAIL状态时，从节点的选举就会被拒绝。


随机时延的目的是为了避免从节点同时进行推荐。



The SLAVE_RANK is the rank of this slave regarding the amount of replication data it has processed from the master. Slaves exchange messages when the master is failing in order to establish a (best effort) rank: the slave with the most updated replication offset is at rank 0, the second most updated at rank 1, and so forth. In this way the most updated slaves try to get elected before others.
Rank order is not strictly enforced; if a slave of higher rank fails to be elected, the others will try shortly.
Once a slave wins the election, it obtains a new unique and incremental configEpoch which is higher than that of any other existing master. It starts advertising itself as master in ping and pong packets, providing the set of served slots with a configEpoch that will win over the past ones.
In order to speedup the reconfiguration of other nodes, a pong packet is broadcast to all the nodes of the cluster. Currently unreachable nodes will eventually be reconfigured when they receive a ping or pong packet from another node or will receive an UPDATE packet from another node if the information it publishes via heartbeat packets are detected to be out of date.
The other nodes will detect that there is a new master serving the same slots served by the old master but with a greater configEpoch, and will upgrade their configuration. Slaves of the old master (or the failed over master if it rejoins the cluster) will not just upgrade the configuration but will also reconfigure to replicate from the new master. How nodes rejoining the cluster are configured is explained in the next sections.
##Masters reply to slave vote request
In the previous section it was discussed how slaves try to get elected. This section explains what happens from the point of view of a master that is requested to vote for a given slave.
Masters receive requests for votes in form of FAILOVER_AUTH_REQUEST requests from slaves.
For a vote to be granted the following conditions need to be met:
A master only votes a single time for a given epoch, and refuses to vote for older epochs: every master has a lastVoteEpoch field and will refuse to vote again as long as the currentEpoch in the auth request packet is not greater than the lastVoteEpoch. When a master replies positively to a vote request, the lastVoteEpoch is updated accordingly, and safely stored on disk.
A master votes for a slave only if the slave's master is flagged as FAIL.
Auth requests with a currentEpoch that is less than the master currentEpoch are ignored. Because of this the master reply will always have the same currentEpoch as the auth request. If the same slave asks again to be voted, incrementing the currentEpoch, it is guaranteed that an old delayed reply from the master can not be accepted for the new vote.
Example of the issue caused by not using rule number 3:
Master currentEpoch is 5, lastVoteEpoch is 1 (this may happen after a few failed elections)
Slave currentEpoch is 3.
Slave tries to be elected with epoch 4 (3+1), master replies with an ok with currentEpoch 5, however the reply is delayed.
Slave will try to be elected again, at a later time, with epoch 5 (4+1), the delayed reply reaches the slave with currentEpoch 5, and is accepted as valid.
Masters don't vote for a slave of the same master before NODE_TIMEOUT * 2 has elapsed if a slave of that master was already voted for. This is not strictly required as it is not possible for two slaves to win the election in the same epoch. However, in practical terms it ensures that when a slave is elected it has plenty of time to inform the other slaves and avoid the possibility that another slave will win a new election, performing an unnecessary second failover.
Masters make no effort to select the best slave in any way. If the slave's master is in FAIL state and the master did not vote in the current term, a positive vote is granted. The best slave is the most likely to start an election and win it before the other slaves, since it will usually be able to start the voting process earlier because of its higher rank as explained in the previous section.
When a master refuses to vote for a given slave there is no negative response, the request is simply ignored.
Masters don't vote for slaves sending a configEpoch that is less than any configEpoch in the master table for the slots claimed by the slave. Remember that the slave sends the configEpoch of its master, and the bitmap of the slots served by its master. This means that the slave requesting the vote must have a configuration for the slots it wants to failover that is newer or equal the one of the master granting the vote.
Practical example of configuration epoch usefulness during partitions
This section illustrates how the epoch concept is used to make the slave promotion process more resistant to partitions.
A master is no longer reachable indefinitely. The master has three slaves A, B, C.
Slave A wins the election and is promoted to master.
A network partition makes A not available for the majority of the cluster.
Slave B wins the election and is promoted as master.
A partition makes B not available for the majority of the cluster.
The previous partition is fixed, and A is available again.
At this point B is down and A is available again with a role of master (actually UPDATE messages would reconfigure it promptly, but here we assume all UPDATE messages were lost). At the same time, slave C will try to get elected in order to fail over B. This is what happens:
B will try to get elected and will succeed, since for the majority of masters its master is actually down. It will obtain a new incremental configEpoch.
A will not be able to claim to be the master for its hash slots, because the other nodes already have the same hash slots associated with a higher configuration epoch (the one of B) compared to the one published by A.
So, all the nodes will upgrade their table to assign the hash slots to C, and the cluster will continue its operations.
As you'll see in the next sections, a stale node rejoining a cluster will usually get notified as soon as possible about the configuration change because as soon as it pings any other node, the receiver will detect it has stale information and will send an UPDATE message.
Hash slots configuration propagation
An important part of Redis Cluster is the mechanism used to propagate the information about which cluster node is serving a given set of hash slots. This is vital to both the startup of a fresh cluster and the ability to upgrade the configuration after a slave was promoted to serve the slots of its failing master.
The same mechanism allows nodes partitioned away for an indefinite amount of time to rejoin the cluster in a sensible way.
There are two ways hash slot configurations are propagated:
Heartbeat messages. The sender of a ping or pong packet always adds information about the set of hash slots it (or its master, if it is a slave) serves.
UPDATE messages. Since in every heartbeat packet there is information about the sender configEpoch and set of hash slots served, if a receiver of a heartbeat packet finds the sender information is stale, it will send a packet with new information, forcing the stale node to update its info.
The receiver of a heartbeat or UPDATE message uses certain simple rules in order to update its table mapping hash slots to nodes. When a new Redis Cluster node is created, its local hash slot table is simply initialized to NULL entries so that each hash slot is not bound or linked to any node. This looks similar to the following:
0 -> NULL
1 -> NULL
2 -> NULL
...
16383 -> NULL
The first rule followed by a node in order to update its hash slot table is the following:
Rule 1: If a hash slot is unassigned (set to NULL), and a known node claims it, I'll modify my hash slot table and associate the claimed hash slots to it.
So if we receive a heartbeat from node A claiming to serve hash slots 1 and 2 with a configuration epoch value of 3, the table will be modified to:
0 -> NULL
1 -> A [3]
2 -> A [3]
...
16383 -> NULL
When a new cluster is created, a system administrator needs to manually assign (using the CLUSTER ADDSLOTS command, via the redis-trib command line tool, or by any other means) the slots served by each master node only to the node itself, and the information will rapidly propagate across the cluster.
However this rule is not enough. We know that hash slot mapping can change during two events:
A slave replaces its master during a failover.
A slot is resharded from a node to a different one.
For now let's focus on failovers. When a slave fails over its master, it obtains a configuration epoch which is guaranteed to be greater than the one of its master (and more generally greater than any other configuration epoch generated previously). For example node B, which is a slave of A, may failover B with configuration epoch of 4. It will start to send heartbeat packets (the first time mass-broadcasting cluster-wide) and because of the following second rule, receivers will update their hash slot tables:
Rule 2: If a hash slot is already assigned, and a known node is advertising it using a configEpoch that is greater than the configEpoch of the master currently associated with the slot, I'll rebind the hash slot to the new node.
So after receiving messages from B that claim to serve hash slots 1 and 2 with configuration epoch of 4, the receivers will update their table in the following way:
0 -> NULL
1 -> B [4]
2 -> B [4]
...
16383 -> NULL
Liveness property: because of the second rule, eventually all nodes in the cluster will agree that the owner of a slot is the one with the greatest configEpoch among the nodes advertising it.
This mechanism in Redis Cluster is called last failover wins.
The same happens during reshardings. When a node importing a hash slot completes the import operation, its configuration epoch is incremented to make sure the change will be propagated throughout the cluster.
UPDATE messages, a closer look
With the previous section in mind, it is easier to see how update messages work. Node A may rejoin the cluster after some time. It will send heartbeat packets where it claims it serves hash slots 1 and 2 with configuration epoch of 3. All the receivers with updated information will instead see that the same hash slots are associated with node B having an higher configuration epoch. Because of this they'll send an UPDATE message to A with the new configuration for the slots. A will update its configuration because of the rule 2 above.
How nodes rejoin the cluster
The same basic mechanism is used when a node rejoins a cluster. Continuing with the example above, node A will be notified that hash slots 1 and 2 are now served by B. Assuming that these two were the only hash slots served by A, the count of hash slots served by A will drop to 0! So A will reconfigure to be a slave of the new master.
The actual rule followed is a bit more complex than this. In general it may happen that A rejoins after a lot of time, in the meantime it may happen that hash slots originally served by A are served by multiple nodes, for example hash slot 1 may be served by B, and hash slot 2 by C.
So the actual Redis Cluster node role switch rule is: A master node will change its configuration to replicate (be a slave of) the node that stole its last hash slot.
During reconfiguration, eventually the number of served hash slots will drop to zero, and the node will reconfigure accordingly. Note that in the base case this just means that the old master will be a slave of the slave that replaced it after a failover. However in the general form the rule covers all possible cases.
Slaves do exactly the same: they reconfigure to replicate the node that stole the last hash slot of its former master.
Replica migration
Redis Cluster implements a concept called replica migration in order to improve the availability of the system. The idea is that in a cluster with a master-slave setup, if the map between slaves and masters is fixed availability is limited over time if multiple independent failures of single nodes happen.
For example in a cluster where every master has a single slave, the cluster can continue operations as long as either the master or the slave fail, but not if both fail the same time. However there is a class of failures that are the independent failures of single nodes caused by hardware or software issues that can accumulate over time. For example:
Master A has a single slave A1.
Master A fails. A1 is promoted as new master.
Three hours later A1 fails in an independent manner (unrelated to the failure of A). No other slave is available for promotion since node A is still down. The cluster cannot continue normal operations.
If the map between masters and slaves is fixed, the only way to make the cluster more resistant to the above scenario is to add slaves to every master, however this is costly as it requires more instances of Redis to be executed, more memory, and so forth.
An alternative is to create an asymmetry in the cluster, and let the cluster layout automatically change over time. For example the cluster may have three masters A, B, C. A and B have a single slave each, A1 and B1. However the master C is different and has two slaves: C1 and C2.
Replica migration is the process of automatic reconfiguration of a slave in order to migrate to a master that has no longer coverage (no working slaves). With replica migration the scenario mentioned above turns into the following:
Master A fails. A1 is promoted.
C2 migrates as slave of A1, that is otherwise not backed by any slave.
Three hours later A1 fails as well.
C2 is promoted as new master to replace A1.
The cluster can continue the operations.
Replica migration algorithm
The migration algorithm does not use any form of agreement since the slave layout in a Redis Cluster is not part of the cluster configuration that needs to be consistent and/or versioned with config epochs. Instead it uses an algorithm to avoid mass-migration of slaves when a master is not backed. The algorithm guarantees that eventually (once the cluster configuration is stable) every master will be backed by at least one slave.
This is how the algorithm works. To start we need to define what is a good slave in this context: a good slave is a slave not in FAIL state from the point of view of a given node.
The execution of the algorithm is triggered in every slave that detects that there is at least a single master without good slaves. However among all the slaves detecting this condition, only a subset should act. This subset is actually often a single slave unless different slaves have in a given moment a slightly different view of the failure state of other nodes.
The acting slave is the slave among the masters with the maximum number of attached slaves, that is not in FAIL state and has the smallest node ID.
So for example if there are 10 masters with 1 slave each, and 2 masters with 5 slaves each, the slave that will try to migrate is - among the 2 masters having 5 slaves - the one with the lowest node ID. Given that no agreement is used, it is possible that when the cluster configuration is not stable, a race condition occurs where multiple slaves believe themselves to be the non-failing slave with the lower node ID (it is unlikely for this to happen in practice). If this happens, the result is multiple slaves migrating to the same master, which is harmless. If the race happens in a way that will leave the ceding master without slaves, as soon as the cluster is stable again the algorithm will be re-executed again and will migrate a slave back to the original master.
Eventually every master will be backed by at least one slave. However, the normal behavior is that a single slave migrates from a master with multiple slaves to an orphaned master.
The algorithm is controlled by a user-configurable parameter called cluster-migration-barrier: the number of good slaves a master must be left with before a slave can migrate away. For example, if this parameter is set to 2, a slave can try to migrate only if its master remains with two working slaves.
##configEpoch conflicts resolution algorithm
When new configEpoch values are created via slave promotion during failovers, they are guaranteed to be unique.
However there are two distinct events where new configEpoch values are created in an unsafe way, just incrementing the local currentEpoch of the local node and hoping there are no conflicts at the same time. Both the events are system-administrator triggered:
CLUSTER FAILOVER command with TAKEOVER option is able to manually promote a slave node into a master without the majority of masters being available. This is useful, for example, in multi data center setups.
Migration of slots for cluster rebalancing also generates new configuration epochs inside the local node without agreement for performance reasons.
Specifically, during manual reshardings, when a hash slot is migrated from a node A to a node B, the resharding program will force B to upgrade its configuration to an epoch which is the greatest found in the cluster, plus 1 (unless the node is already the one with the greatest configuration epoch), without requiring agreement from other nodes. Usually a real world resharding involves moving several hundred hash slots (especially in small clusters). Requiring an agreement to generate new configuration epochs during reshardings, for each hash slot moved, is inefficient. Moreover it requires an fsync in each of the cluster nodes every time in order to store the new configuration. Because of the way it is performed instead, we only need a new config epoch when the first hash slot is moved, making it much more efficient in production environments.
However because of the two cases above, it is possible (though unlikely) to end with multiple nodes having the same configuration epoch. A resharding operation performed by the system administrator, and a failover happening at the same time (plus a lot of bad luck) could cause currentEpoch collisions if they are not propagated fast enough.
Moreover, software bugs and filesystem corruptions can also contribute to multiple nodes having the same configuration epoch.
When masters serving different hash slots have the same configEpoch, there are no issues. It is more important that slaves failing over a master have unique configuration epochs.
That said, manual interventions or reshardings may change the cluster configuration in different ways. The Redis Cluster main liveness property requires that slot configurations always converge, so under every circumstance we really want all the master nodes to have a different configEpoch.
In order to enforce this, a conflict resolution algorithm is used in the event that two nodes end up with the same configEpoch.
IF a master node detects another master node is advertising itself with the same configEpoch.
AND IF the node has a lexicographically smaller Node ID compared to the other node claiming the same configEpoch.
THEN it increments its currentEpoch by 1, and uses it as the new configEpoch.
If there are any set of nodes with the same configEpoch, all the nodes but the one with the greatest Node ID will move forward, guaranteeing that, eventually, every node will pick a unique configEpoch regardless of what happened.
This mechanism also guarantees that after a fresh cluster is created, all nodes start with a different configEpoch (even if this is not actually used) since redis-trib makes sure to use CONFIG SET-CONFIG-EPOCH at startup. However if for some reason a node is left misconfigured, it will update its configuration to a different configuration epoch automatically.
##Node resets
Nodes can be software reset (without restarting them) in order to be reused in a different role or in a different cluster. This is useful in normal operations, in testing, and in cloud environments where a given node can be reprovisioned to join a different set of nodes to enlarge or create a new cluster.
In Redis Cluster nodes are reset using the CLUSTER RESET command. The command is provided in two variants:
CLUSTER RESET SOFT
CLUSTER RESET HARD
The command must be sent directly to the node to reset. If no reset type is provided, a soft reset is performed.
The following is a list of operations performed by a reset:
Soft and hard reset: If the node is a slave, it is turned into a master, and its dataset is discarded. If the node is a master and contains keys the reset operation is aborted.
Soft and hard reset: All the slots are released, and the manual failover state is reset.
Soft and hard reset: All the other nodes in the nodes table are removed, so the node no longer knows any other node.
Hard reset only: currentEpoch, configEpoch, and lastVoteEpoch are set to 0.
Hard reset only: the Node ID is changed to a new random ID.
Master nodes with non-empty data sets can't be reset (since normally you want to reshard data to the other nodes). However, under special conditions when this is appropriate (e.g. when a cluster is totally destroyed with the intent of creating a new one), FLUSHALL must be executed before proceeding with the reset.
## Removing nodes from a cluster
It is possible to practically remove a node from an existing cluster by resharding all its data to other nodes (if it is a master node) and shutting it down. However, the other nodes will still remember its node ID and address, and will attempt to connect with it.
For this reason, when a node is removed we want to also remove its entry from all the other nodes tables. This is accomplished by using the CLUSTER FORGET <node-id> command.
The command does two things:
It removes the node with the specified node ID from the nodes table.
It sets a 60 second ban which prevents a node with the same node ID from being re-added.
The second operation is needed because Redis Cluster uses gossip in order to auto-discover nodes, so removing the node X from node A, could result in node B gossiping about node X to A again. Because of the 60 second ban, the Redis Cluster administration tools have 60 seconds in order to remove the node from all the nodes, preventing the re-addition of the node due to auto discovery.
Further information is available in the CLUSTER FORGET documentation.
## Publish/Subscribe
In a Redis Cluster clients can subscribe to every node, and can also publish to every other node. The cluster will make sure that published messages are forwarded as needed.
The current implementation will simply broadcast each published message to all other nodes, but at some point this will be optimized either using Bloom filters or other algorithms.
# Appendix
Appendix A: CRC16 reference implementation in ANSI C
/*
 * Copyright 2001-2010 Georges Menie (www.menie.org)
 * Copyright 2010 Salvatore Sanfilippo (adapted to Redis coding style)
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University of California, Berkeley nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* CRC16 implementation according to CCITT standards.
 *
 * Note by @antirez: this is actually the XMODEM CRC 16 algorithm, using the
 * following parameters:
 *
 * Name                       : "XMODEM", also known as "ZMODEM", "CRC-16/ACORN"
 * Width                      : 16 bit
 * Poly                       : 1021 (That is actually x^16 + x^12 + x^5 + 1)
 * Initialization             : 0000
 * Reflect Input byte         : False
 * Reflect Output CRC         : False
 * Xor constant to output CRC : 0000
 * Output for "123456789"     : 31C3
 */

static const uint16_t crc16tab[256]= {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
    0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
    0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
    0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
    0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
    0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
    0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
    0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
    0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
    0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
    0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
    0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
    0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
    0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
    0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
    0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
    0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
    0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
    0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
    0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
    0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
    0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
    0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
    0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
    0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
    0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
    0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
    0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
    0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
    0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
    0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

uint16_t crc16(const char *buf, int len) {
    int counter;
    uint16_t crc = 0;
    for (counter = 0; counter < len; counter++)
            crc = (crc<<8) ^ crc16tab[((crc>>8) ^ *buf++)&0x00FF];
    return crc;
}
