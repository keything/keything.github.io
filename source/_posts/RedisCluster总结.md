---
title: RedisCluster总结
date: 2016-08-10 21:39:06
tag: [nosql, redis]
---

# 一、RedisCluster基本内容
## 1. 项目背景

### a. 整个项目背景
	
+ 云盘memcache服务器分散，监控和替换麻烦

    云盘100多个集群，每个集群4个memcache，需要业务方配置和监控这些memcache。一旦memcache超时或者不可用，需要业务方和ops共同操作才能解决memcache的问题。
+ 云盘缓存并发高、空间大

    云盘缓存有36亿，400G空间，每秒钟25万的请求量，因此需要高并发的缓存集群才能满足需求。
	
### b. 现在的替代方案

使用RedisCluster作为缓存集群。其特点（也是其设计目标）是：

+ 水平扩展：最大可以扩展到1000个节点
+ 可以接受的写安全：当出现网络分区（即脑裂）的时候能保证连接到大多数节点分区的写是安全的。
+ 高可用性：当出现网络分区时，大多数节点所在的分区是可用的；并且通过复制转移（replicas migration）保证没有从节点的主节点获得一个从节点。
    （所谓的复制转移：当一个主节点A有多个从节点A1,A2,A3，而另外一个主节点B没有从节点时，RedisCluster集群会从多个从节点A1,A2,A3中选择一个从节点A2作为主节点B的从节点）
	
目前RedisCluster 目前集群大小是：
+ 一个机房一个大集群
+ bjdt：集群中有24个节点，每个节点一主一从。每个节点18.63G
+ bjcc：集群内有16个节点，每个节点一主一从。每个节点18.63G。

## 2. 使用方法	
### a. 业务方使用：
	
有两种客户端，一种是笨蛋客户端，一种是智能客户端。

``笨蛋客户端``：本地不缓存节点信息，请求达到任意一个RedisCluster节点后，被告知真正的节点，然后再次请求。比如：redis-cli，telnet
    
    redis-cli -c -h 10.10.10.10 -p 6666
    10.10.10.10:6666> get ab
    -> Redirected to slot [13567] located at 11.11.11.11:6666
    "1"
    
``智能客户端``：本地缓存节点信息，通过本地计算出槽位，直接发一次请求即可。

		
### b. phpredis

目前[phpredis](https://github.com/phpredis/phpredis)是智能客户端。

不过在最开始的版本中，phpredis每次new RedisCluster都会发送cluster slots来获取节点信息，其实对于一个业务而言是没有必要的。原因是因为RedisCluster一旦稳定后一般不会调整，那么其节点信息就维持不变，因此没有必要每次都去获取。并且cluster slots非常消耗cluster节点的cpu。这一点[吴晓飞](https://github.com/JacketWoo)同学已经给phpredis提了[pullrequest](https://github.com/phpredis/phpredis/pull/826)
	
## 3. RedisCluster基本概念

### a. 集群创建

1. Redis实例启动，redis.conf中多了一个配置项``cluster-enabled yes``
2. Redis节点通过``cluster meet``形成一个集群，但是集群不可用
3. 分配槽位：需要将16384个槽位分配到集群中各个节点。如果有的槽位没有分配成功，那么集群就可能不可用（跟``cluster-require-full-coverage``参数有关系）

### b. RedisCluster keys分布模型

一共有16384个hash槽位。集群中每个主节点负责16384个槽位中的一部分。如果没有正在进行中的集群重新配置，那么槽位的分布是稳定的。

key与槽位的映射关系计算算法（如果有hash tags则不使用该计算算法）

    HASH_SLOT=CRC(key) mod 16384
	
### c. RedisCluster与Memcache比较

我们知道Memcached是“分布式”缓存服务器，但是服务器端没有分布式功能，需要客户端来实现分布式，也就是需要客户端配置好几台memcached服务器，根据分布式算法（一般采用一致性hash算法）计算哪个key位于哪台memcached机器上。
	
而RedisCluster没有采用一致性hash算法，而是采用了一种``hash 槽位``的概念。与Memcached相比，业务方不需要维护memcached服务器的列表。使用智能客户端，可以先获取一份节点信息，然后在本地进行计算hash槽位。


### d. RedisCluster HashTags

所谓的``hash tags``是指key中出现了``{"字符串"} ``或者空的符合``{}``。 当出现``HashTags``时，key的``hash 槽位``不是计算整个key所在的槽位，而是计算``{}``中间字符串的槽位。
举例：``{user1000}.following`` 与 ``{user1000}.followers`` 这两个key在同一个slot里面。因为其``hash tags``中的字符串是 ``user1000``

### e. RedisCluster和Redis比较

* RedisCluster实现了所有单key的命令，比如get,set
* RedisCluster中多key操作的命令要求多key必须在同一个节点上，可以通过``hash tags``强迫这些key存在相同节点上。
	

### f. RedisCluster重定向

#### 基本概念	

RedisCluster有两种重定向错误：``-MOVED``和``-ASK``两种重定向错误

刚刚在``RedisCluster keys``分布模型中提到如果没有进行中的集群重新配置，那么槽位分布是稳定的。
如果集群正在发生调整呢，那么集群会返回给客户端``-MOVED 8 127.0.0.1:8080``错误或者 ``-ASK 8 127.0.0.1:8080``这两个重定向错误。 


#### 举例
举例：假设集群中节点A(``127.0.0.1:8080``)负责hash槽位13565,13566,13567。 节点B(``127.0.0.2:8080``)负责hash槽位100，101，102。

``根据crc(ab) mod 16384计算得到ab这个key的slot是13567``

a. 如果集群没有调整，那么 向节点B请求get ab ，那么节点B会返回``-MOVED 127.0.0.1:8080``。 

b. 如果集群有所调整，将hash槽位13567从节点A迁移到节点B。

在迁移过程中，节点A处于``MIGRATING``状态，节点B处于``IMPORTING``状态。

此时向节点A请求get ab， 

1) 如果ab这个位于13567的key已经迁移到节点B了，那么将会受到``-ASK 127.0.0.2:8080``，此时客户端先向节点B发送一个ASKING的命令，然后再get ab就可以获得数据。如果客户端不向节点B发送一个ASKING命令，而是直接向节点B发送get ab则会返回``-MOVED``错误。
2) 如果ab这个位于13567的key没有迁移到节点B，即依然在节点A上，那么直接返回数据。

c. 如果集群已经调整完毕，即hash槽位13567位于节点B，那么向节点A请求 get ab就会返回``-MOVED 127.0.0.2:8080``

#### ``-MOVED``和``-ASK``两个重定向区别：

1. MOVED代表槽已经完全从一个节点迁移到另外一个节点
2. ASK是槽位迁移的中间态，代表这个槽位的节点正在迁移。只针对这个槽位的这个key进行重定向，该槽位上其他key依然先到原来的节点


### f. RedisCluster节点属性

    redis-cli cluster nodes
    d1861060fe6a534d42d8a19aeb36600e18785e04 127.0.0.1:6379 myself - 0 1318428930 1 connected 0-1364
    3886e65cc906bfd9b1f7e7bde468726a052d1dae 127.0.0.1:6380 master - 1318428930 1318428931 2 connected 1365-2729
    d289c575dcbc4bdd2931585fd4339089e461a27d 127.0.0.1:6381 master - 1318428931 1318428931 3 connected 2730-4095
	
通过[cluster nodes](http://redis.io/commands/cluster-nodes)命令可以知道RedisCluster集群的很多信息，比如当前槽位分布，当前节点个数及其ip:port等等
	
# 二、RedisCluster的高可用原理

## 1. RedisCluster的CAP

RedisCluster本身是一个分布式NoSQL，因此必然符合CAP定理中的相关内容。

### CAP定理
CAP定理基本描述：

给定”一致性Consistency“、”可用性Availability“、”分区耐受性Partition tolerance"这三个属性，我们只能同时满足其中两个属性。

+ 分区耐受性：如果发生通信故障，导致整个集群被分隔成多个无法互相通信的分区时（这种情况也叫“脑裂” split brain），集群依然可用。
	
	在RedisCluster中有个参数cluster-require-full-coverage 
	该参数取值为yes时，出现分区时则不可用；反之则不可用

	![脑裂](/images/redis_cluster/脑裂.png)
+ ``一致性``：在各个节点上数据一样。
	一致性的关键在于：将请求序列化，使之成为原子的（atomic）、相互隔离的（Isolated）”工作单元“（work unit）。

+ ``可用性``：如果客户可以同集群中的某个节点通信，那么该节点就必须能够处理读取及写入操作。
CAP定理中将可用性定义为：系统中某个无故障节点所接受的每一条请求，无论成功或失败，都必将得到响应。所以按照这个定义，发生故障且无法响应客户请求的节点，并不会导致系统失去“CAP定理”所定义的那种“可用性”。

这意味着你可以构建一个CA集群，如果出现“分区”现象，那么所有节点必须全部停止工作。

尽管CAP定理经常表述为“三个属性中只能保有两个”，但实际上是在讲：当系统可能遭遇“分区”状况时，我们需要在“一致性”与“可用性”之间进行权衡。通常我们会略微舍弃“一致性”，以获取某种程度的“可用性”。这样产生的系统，既不具备完美的“一致性”，也不具备完美的“可用性”，但是这两种不完美的特性结合起来却能够满足特定需求。

### RedisCluster满足了AP，最终一致性

1. 分区耐受性：大多数节点可用时，RedisCluster可以分区可用。当出现分区时，连接到多数节点分区和连接到少数节点分区有很大不同。
2. 可用性：通过复制转移（replicas migration），没有从节点的主节点会获得一个从节点，从而提供可用性。
3. 高性能并且能够线性扩展到1000个节点。
4. 最终一致性，异步复制，存在写入安全问题。
	
	最终一致性：eventually consistent：也就是说在任意时刻，节点中都可能存在“复制不一致”问题，然而只要不再继续执行其他更新操作，那么上一次更新操作的结果最终将会反映到全部节点中去。
	
### RedisCluster写入的流程

RedisCluster的写入是异步复制。
+ client先写入主，主回复``+OK``，此时client认为写入成功
+ 主节点开始异步复制，将写入同步到从节点

### RedisCluster写入安全
+ 写入安全情况1: 
    先写入主
    再由主传播到从。
    主从式分布模型，存在主写入成功，主挂没有传播
    从升为主，数据丢失。


+ 写入安全情况2: 发生分区时写丢失

	举例：6节点集群，3主3从，节点是A,B,C, A1,B1,C1。还有一个客户端Z1。发生分区时，一个分区是A,C,A1,B1,C1，另外一个分区是B和Z1.此时Z1还可以向B写入，B也可以接收写操作。
    如果在很短时间内，B恢复了那么集群正常；
    如果分区持续太久，那么B1就被推举为新主，此时过了NODE_TIMEOUT时间，节点B也不写，那么分区后写入B的数据就丢失了。
	写操作丢失的最大持续时间是NODE_TIMEOUT+从推举时间

	NoSQL倡导者经常说，与关系型数据库所支持的ACID事务不同，NoSQL系统具备“BASE属性“（基本可用，柔性状态，最终一致性）英文是 Basically Available, Soft state, Eventual consistency.




## 2. 容错 Fault Tolerance


### 心跳包和流言消息

心跳包的目的：RedisCluster集群中各个节点会通过发送心跳包（ping包和pong包）来进行通信，更新集群的配置信息。通常节点发送ping包以后，期待对方回复pong包。

RedisCluster一个节点一次只向集群中某些节点发送心跳包（目前一次发送的节点数量是总节点个数的十分之一）。同时考虑到RedisCluster发送对象节点是随机的，所以存在两个节点很久都没有交换消息，为了保证集群状态能够在很多时间内达到一致性，RedisCluster规定当两个节点超过``NODE_TIMEOUT一半``的时间没有交换消息时，下次发心跳包交换消息。

比如，对于100个节点的集群，NODE_TIMEOUT设置为60秒，那么根据上面的理论，一个节点在30s内要向99个节点发送ping，对于100各节点则每秒发送330个pings。[近千节点的Redis Cluster高可用集群案例](https://mp.weixin.qq.com/s?__biz=MzAwMDU1MTE1OQ==&mid=2653547585&idx=1&sn=9a664b16f656f757632cd4eb29f9a5dc&scene=0&key=8dcebf9e179c9f3a346c582e0a5712dda1ec87878842175cce35a5ea1cd92ee99c770c79d2f99f97dcb43597000373f8&ascene=0&uin=NTE5MDc2ODU1&devicetype=iMac+MacBookPro11%2C1+OSX+OSX+10.11.3+build(15D21)&version=11020201&pass_ticket=4CBC9RgbSswvChRwX4aHuDwbNTxwAjmPNbOVneP4ac8%2BaS%2BQ8YWN5LJF3ipxB8fR)

通过这一点可以看出，集群间通信占用大量带宽资源。


### 故障检测

目的：当大多数节点不能访问某个主或从节点时，其从节点就会被推举为主节点。当从推举失败后，集群是error state, 将会停止接收来自客户端的请求。

实现：

1. PFAIL状态：
集群中有n个节点，当节点A自己认为节点B不可用了，并不能认为节点B不可用，
2. FAIL状态：
必须集群中大多数节点认为节点B不可用了，节点B才是真的不可用。
当在``NODE_TIME * FAIL_REPORT_VALIDITY_MULT``时间内超过一半的节点认为B不可达时，节点B才真的是不可达
（当前实现中该validity参数是2）。 
	节点不可达的概念：节点发送的ping包超过NODE_TIMEOUT时间依然收不到pong包。
	工作原理：当发送的ping包，在``NODE_TIMEOUT/2``时间后依然收不到pong包时，节点会去重连集群中的其他节点。
3. 广播：
   此时节点A会广播一条FAIL消息，告知大家节点B不可达。所有收到FAIL消息的节点，都被强制设定节点B不可达。
	FAIL标志只是为了安全的触发从推举的算法。

### 故障转移 failover

1. 从节点选举

    从已经下线的主节点的所有从节点里面，选中一个从节点。从节点的选举需要得到大多数主节点的授权

2. 成为主节点

    被选中的从节点会执行SLAVEOF no one命令，成为新的主节点并且负责旧主节点的槽位

3. 广播

    广播一条pong消息，通知其他节点更新节点映射信息


### 复制转移 replicas migration

复制转移的定义：如果一个主节点A没有从节点，而集群中有的主节点B有多个从节点s1,s2,s3；那么RedisCluster会从多个从节点中选择一个从节点作为主节点A的从节点。
复制转移的目的：为了提高系统的可用性。

比如一个集群中A,B,C 3个主节点，其从节点分别是A1,B1,C1,C2，其中C的从节点有两个。
当B挂了以后，B1升为主节点，此时B1节点没有从节点。

a. 如果没有复制转移的话，那么B1再挂了的话，这个节点上的槽位都不可用了。
b. 如果有复制转移的话，那么会从C的两个从节点C1和C2中选择一个作为B1的从节点，我们假设选择的是C2。那么现在集群的情况是A-A1，B1-C2，C-C1。如果这时候B节点又可用了，那么他将会作为某个主节点的从节点。

## 4. 配置操作、传播、故障转移

### Cluster currentEpoch

其目的是为了当节点信息发生冲突的时候来解决冲突。解决冲突的方法很简单，epoch高的配置覆盖epoch低的配置，即以epoch高的配置为准。


### configuration epoch

1. 新节点创建时，configEpoch是0
2. 从节点推举后，生成新的configEpoch，从节点尝试取代失败主节点的epoch，并且获得大多数主节点的授权，那么configEpoch会加1. 
3. configEpoch帮助解决的是当不同节点声明不同配置配置时，用于解决冲突。

RedisCluster配置参数：

	cluster-enabled 
	cluster-config-file <filename>
	cluster-node-timeout <milliseconds>
	cluster-slave-validity-factor <factor>
	cluster-migration-barrier <count>
	cluster-require-full-coverage <yes/no>
	
# 三、期间出现问题

1. 客户端cluster slots没有缓存，造成RedisCluster节点cpu过高。

    phpredis虽然是智能客户端，
    但对于每个RedisCluster类都需要发送``cluster slots``命令获取节点和slot的对应关系，从而造成节点CPU过高。
    因为``cluster slots``命令需要执行两层主循环，分别是循环nodes和slot。对于master节点还需要扫描slave，cpu的计算开销就出来了
		
2. timewait过高

	由于我们代码中设置100ms超时，当RedisCluster服务器返回过慢时，客户端会主动断开连接，因此出现大量timewait

3. 为何不使用长连接。

	a. 因为云盘前端机非常多，300台前端机，每台前端机128个进程，那么对于cluster节点而言是38400个长连接，cluster节点所占用的内存和fd开销非常大。
    b. 长连接：cpu明显下降，但是连接数暴涨 
    c. 短连接：通过客户端缓存节点信息能够降低部分cpu，但cpu依然偏高
4.  del返回warning

    原因是del对返回值校验严格，要求必须是整形（即``:1``)这种格式，当出现``-MOVED``错误或者超时没有读取到数据时，则会报warning
    出现时机：当RedisCluster的CPU过高时，del在100ms（设定的读超时是100ms）内没有响应，那么返回``?``的值，不是整形，因此会报warning

## 参考文章

1. [Redis Cluster的FailOver失败案例分析](http://itindex.net/detail/52699-redis-cluster-failover)
2. [Redis 百万QPS挑战](http://itindex.net/detail/54098-redis-cluster-%E7%99%BE%E4%B8%87)
3. [Redis Cluster Specification](http://redis.io/topics/cluster-spec)
