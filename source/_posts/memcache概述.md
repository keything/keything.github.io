---
title: "1. memcache概述"
date:   2016-04-17 12:00:01 +0800
tags: [nosql,memcache]
---

## 概述

1. memcached的特征
    memcached 作为高速运行的分布式缓存服务器,具有以下的特点。
    1. 协议简单
    2. 基于 libevent 的事件处理 
	3. 内置内存存储方式
    4. memcached 不互相通信的分布式
	  mc是分布式缓存服务器，完全取决于客户端的实现

## Memcache的内存分配
	
1. 基本概念
	1. Page
    分配给Slab的内存空间,默认是 1MB。
	分配给Slab之后根据slab的大小切分成chunk。
    2. Chunk 用于缓存记录的内存空间。 
	3. Slab Class：特定大小的Chunk数组
2. 保存
	memcached 根据收到的数据的大小,选择最适合数据大小的 slab。 memcached 中保存着 slab 内空闲 chunk 的列表,根据该列表选择 chunk, 然后 将数据缓存于其中。
3. 调优
	使用Growth Factor进行调优，默认是1.25
	
4. 小结

	将固定大小的内存（page）分割成各种尺寸的块（chunk）
	把尺寸相同的块分组，叫做slab classes
	
## Memcache的过期
 memcached不会释放已分配的内存。记录超时后,客户端就无法再 看见该记录(invisible,透明), 其存储空间即可重复使用。
 Lazy Expiration
 memcached 内部不会监视记录是否过期,而是在 get 时查看记录的时间戳,检查 记录是否过期。 这种技术被称为 lazy(惰性)expiration。因此,memcached 不会在过期监视上耗费 CPU 时间。

	
