---
title:  "4. Memcached常见问题"
date:   2016-04-17 15:00:01 +0800
tags: [nosql,memcache]
---

## 关于Mc的零散的问题
1. 如何实现cas的
	很简单，全局维持一个64位的静态变量
		
		uint64_t get_cas_id(void) {
	       static uint64_t cas_id = 0;
	       return ++cas_id;
		}
2. Mc启动时-m指定的最大内存maxMem
	
	1. 并不是一启动，就开始分配内存，而是在需要的时候才进行内存分配
	2. 所指定的最大内存并不是其最大限制，也就是说mc所分配的内存大于maxMem

3. 一开始大量使用某一大小A的chunk，占据大量的内存，那么当大量使用某一个大小B的chunk时，就会导致B chunk所使用的内存很小，极有可能只有一个slab（默认1m）的内存。那么数据就会不断被覆盖，从而导致mc存储的内容频繁失效。
4. 分配给大小A的内存，不可以给其他大小的chunk使用。这也导致了上面3的问题
5. memcached对item的过期时间有什么限制？
	item对象的过期时间最长可以达到30天。memcached把传入的过期时间（时间段）解释成时间点后，一旦到了这个时间点，memcached就把item置为失效状态，这是一个简单但obscure的机制
6. memcached最大能存储多大的单个item？
	
	memcached能存储最大1MB的单个item。如果需要被缓存的数据大于1MB，可以考虑在客户端压缩或拆分到多个key中。

7. 使用不同的客户端库，可以访问到memcached中相同的数据吗？

	从技术上说，是可以的。但是可能会遇到下面三个问题：
	
	1）不同的库采用不同的方式序列化数据

	举个例子，perl的Cache::Memcached使用Storable来序列化结构复杂的数据（比如hash references, objects等）。其他语言的客户端库很可能不能读取这种格式的数据。如果您要存储复杂的数据并且想被多种客户端库读取，那么您应该以简单的string格式来存储，并且这种格式可以被JSON、XML等外部库解析。

	2）从某个客户端来的数据被压缩了，从另一个客户端来的却没被压缩。

	3）各个客户端库可能使用不同的哈希算法（阶段一哈希）。在连接到多个memcached服务器端的情况下，客户端库根据自身实现的哈希算法把key映射到某台memcached上。正是因为不同的客户端库使用不同的哈希算法，所以被Perl客户端库映射到memcached A的key，可能又会被Python客户端库映射到memcached B，等等。Perl客户端库还允许为每台memcached指定不同的权重（weight），这也是导致这个问题的一个因素

## 博客中常见问题
http://huoding.com/2012/12/30/205

1. Cache失效后的拥堵问题
	1. 首先，我们可以主动更新Cache。前端程序里不涉及重建Cache的职责，所有相关逻辑都由后端独立的程序（比如CRON脚本）来完成，但此方法并不适应所有的需求。
	2. 好在我们还有Gearman这根救命稻草。当需要更新Cache的时候，我们不再直接查询数据库，而是把任务抛给Gearman来处理，当并发量比较大的时候，Gearman内部的优化可以保证相同的请求只查询一次后端数据库，以PHP为例，伪代码大致如下：

		
			<?php

			function query()
			{
			    $data = $cache->get($key);

			    if ($cache->getResultCode() == Memcached::RES_NOTFOUND) {
			        $data = $gearman->do($function, $workload, $unique);
			        $cache->set($key, $data, $expiration);
			    }

			    return $data;
			}

			?>
		
		说明：如果多个并发请求的$unique参数一样，那么实际上Gearman只会请求一次。

2. Multiget的无底洞问题

	Facebook在Memcached的实际应用中，发现了Multiget无底洞问题，具体表现为：出于效率的考虑，很多Memcached应用都已Multiget操作为主，随着访问量的增加，系统负载捉襟见肘，遇到此类问题，直觉通常都是通过增加服务器来提升系统性能，但是在实际操作中却发现问题并不简单，新加的服务器好像被扔到了无底洞里一样毫无效果。

3. 客户端的Hash算法
http://www.last.fm/user/RJ/journal/2007/04/10/rz_libketama_-_a_consistent_hashing_algo_for_memcache_clients


##适合Mc的业务场景

1）如果网站包含了访问量很大的动态网页，因而数据库的负载将会很高。由于大部分数据库请求都是读操作，那么memcached可以显著地减小数据库负载。

2）如果数据库服务器的负载比较低但CPU使用率很高，这时可以缓存计算好的结果（ computed objects ）和渲染后的网页模板（enderred templates）。

3）利用memcached可以缓存session数据、临时数据以减少对他们的数据库写操作。

4）缓存一些很小但是被频繁访问的文件。

5）缓存Web ‘services’（非IBM宣扬的Web Services）或RSS feeds的结果.。

## 不适合Mc的业务场景
1）缓存对象的大小大于1MB

  Memcached本身就不是为了处理庞大的多媒体（large media）和巨大的二进制块（streaming huge blobs）而设计的。

2）key的长度大于250字符

3）虚拟主机不让运行memcached服务

  如果应用本身托管在低端的虚拟私有服务器上，像vmware, xen这类虚拟化技术并不适合运行memcached。Memcached需要接管和控制大块的内存，如果memcached管理的内存被OS或 hypervisor交换出去，memcached的性能将大打折扣。

4）应用运行在不安全的环境中

   Memcached 未提供任何安全策略，仅仅通过telnet就可以访问到memcached。如果应用运行在共享的系统上，需要着重考虑安全问题。

5）业务本身需要的是持久化数据或者说需要的应该是database的读数据




## 参考文章

http://blog.mimvp.com/2015/01/memcache-cache-bulk-delete-scheme/ mc的telnet操作 查看
客户端的一致性hash http://www.last.fm/user/RJ/journal/2007/04/10/rz_libketama_-_a_consistent_hashing_algo_for_memcache_clients
火丁的mc http://huoding.com/2012/12/30/205
