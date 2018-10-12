---
title: 'Redis内存优化之小的聚合数据类型的特殊编码'
date: 2016-06-10 08:24:56
tag: [redis,nosql]
---

## Redis内存优化之小的聚合数据类型的特殊编码
从Redis2.2开始，很多数据类型被优化为使用更小的空间。对于由数字组成的Hashes，Lists， Sets，Sorted Sets 在某些情况下可以使用使用更加有效的内存方式来进行编码，这种更有效的内存方式可以节约最少5倍最大10倍的内存。那么在什么情况这些数字组成的类型可以使用这种更有效的内存方式呢?

    当元素个数小于给定的数量时，
    当到了一个最大的元素大小时，


从用户和API角度来看，这完全是透明的。因为这是一种CPU/内存的平衡。可以使用redis.conf中的指令来调整元素的最大个数和最大的元素大小。

    hash-max-zipmap-entries 512 (hash-max-ziplist-entries for Redis >= 2.6)
    hash-max-zipmap-value 64  (hash-max-ziplist-value for Redis >= 2.6)
    list-max-ziplist-entries 512
    list-max-ziplist-value 64
    zset-max-ziplist-entries 128
    zset-max-ziplist-value 64
    set-max-intset-entries 512


如果一个特定编码值超过了设置的最大值，Redis被自动将其转为普通编码。这个转换过程，对于小的取值非常快，但如果你为了使用特殊编码值而改变了配置，那么就需要允许压力测试来测试这个转换时间了。 

## 实践

进行三个实践。

第一个使用key/value的形式存储1w个数据，占用空间608175字节。

第二个使用hash的形式存储1w个数据，占用空间79424字节

1. 采用k/v形式


		#! /bin/bash
  
		redis-cli info|grep used_memory:
	
		for (( start = 10000; start <20000 ; start++ ))
		do
		    redis-cli set $start 100 > /dev/null
		done
	
		redis-cli info|grep used_memory:
	
2. 采用hash形式


		#! /bin/bash
	
		redis-cli info|grep used_memory:
	
		for (( start = 10000; start < 20000 ; start++ ))
		do
		    hash=$((start % 100))
		    redis-cli hset $hash $start 100 > /dev/null
		done
	
		redis-cli info|grep used_memory:



	
	
## 参考文章

[redis内存优化](http://redis.io/topics/memory-optimization)
