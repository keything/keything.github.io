---
title: Redis与memcache作为缓存时的比较
date: 2016-07-16 18:16:56
tag: [nosql, redis]
---


【说明一个大前提】我觉得拿Redis跟Memcache比较时，是把Redis当做缓存来用。不然一个是缓存一个是持久化，那么比较就失去意义了。

【本文目的】这篇文章是总结了大牛们关于Redis和Memcache的论断，加上部分自己的理解。


### 1. Redis的性能比Memcache好。 

Redis的QPS能够达到10w [数据来源]
	
Memcache的QPS在6w [数据来源 
	
#### 数据来源：
	
[redis作者的声明](http://antirez.com/news/94)
	
[memcache,redis性能测试](http://timyang.net/data/mcdb-tt-redis)
	
自己测试的话Redis选择[RedisBenchmark](http://redis.io/topics/benchmarks)，Memcache可以使用[mcperf](https://github.com/twitter/twemperf)

#### 原因：
	
1. Redis没有使用libevent作为事件处理函数库，而是自己造轮子。libevent因为通用牺牲了不少性能，没有epoll性能好[参考](http://rdc.gleasy.com/%E9%AB%98%E6%80%A7%E8%83%BD%E7%BD%91%E7%BB%9C%E6%9C%8D%E5%8A%A1%E5%99%A8%E9%80%89%E5%9E%8B%E6%AF%94%E8%BE%83epolllibeventjava-mina2.html)
    
   
2. cas问题：CAS是Memcached中比较方便的一种防止竞争修改资源的方法。CAS实现需要为每个cache key设置一个隐藏的cas token，cas相当value版本号，每次set会token需要递增，因此带来CPU和内存的双重开销，虽然这些开销很小，但是到单机10G+ cache以及QPS上万之后这些开销就会给双方相对带来一些细微性能差别。
    
### 2. 更多元化的方式使用Redis

除了key/value的形式来使用Redis，还可以使用hash等数据形式。使用Hash更加节约空间。[实践](http://blog.nosqlfan.com/html/3379.html) [理论](http://redis.io/topics/memory-optimization)


### 3. Redis的内存效率更高
   Redis中特殊编码的小型聚合值内存效率非常高[内存优化](http://redis.io/topics/memory-optimization)
    
   真正的内存效率必须基于手头的应用案例来评估
   
### 4. Redis禁用磁盘IO，纯内存操作

有一种论断是说 Memcached根本没有磁盘IO操作。实际上Redis禁用磁盘IO操作以后，也是纯内存操作。

### 5. 作为缓存时，redis LRU vs Slab内存分配器
redis的LRU最近优化了很多，现在非常接近真正的LRU。进一步的信息可阅读： [lru-cache](http://redis.io/topics/lru-cache) 。如果我没理解错的话，Memcached的LRU依旧是根据它的slab分配器来判断数据过期的，因此有时其行为与真正的LRU相差甚远，但我希望这方面的专家能够针对这个问题说点什么。如果你想测试Redis的LRU，在最近几个版本的Redis中可以使用redis-cli的LRU测试模式




### 6. Memcache的多线程对比Redis的单线程

Memcache是多线程的，在多核上能线性扩展到每秒处理100,000个请求。

如果我们将特殊用例下的Redis看做是Memcached的替代品，执行多个Redis进程也能反驳“Memcached多线程更好”这个观点，或者简单地执行一个Redis进程也行。







### 参考文章

[Redis几点认识误区](http://timyang.net/data/redis-misunderstanding/)

[关于Redis和Memcache的几点澄清](http://www.tuicool.com/wx/bmQFNjm)

[redis内存优化](http://redis.io/topics/memory-optimization)
    
