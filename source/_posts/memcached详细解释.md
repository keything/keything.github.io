---
title: "3. Memcached详细解释"
date:   2016-04-17 14:00:01 +0800
tags: [nosql,memcache]
---

## 流程概述
1. 数据存储：保存数据A到mc时，需要先分配空间Item给数据A。
2. 数据查找：对于获取数据，通过Hash表进行查找。
3. 数据过期：通过LRU队列进行过期处理

## 数据存储
为了解释是如何分配空间Item的，那么就要先讲一下Mc的整体结构。

Mc声明了一个slabclass\_t的结构体类型，一个slabclass_t类型数组slabclass(全局变量）。数组的每一个元素是一个slab链表。每一个slab链表所分配的内存大小不同。

我们在启动的时候就还会看到这个
	
	memcached -p 11211 -m 44m -vv -f1.25
	slab class   1: chunk size        96 perslab   10922
	slab class   2: chunk size       120 perslab    8738
	....
	slab class  12: chunk size      1184 perslab     885
	.... 
	slab class  41: chunk size    771184 perslab       1
	slab class  42: chunk size   1048576 perslab       1



如何分配

1. 在do\_item_alloc中获取item
	1. 首先根据LRU队列，选择过期的item，返回该item，使用该item
	2. 如果都没有过期，那么分配新的slab。实际上就是分配新的page，然后根据chunk size进行内存切割。选择一个item，返回该item。【注意】即使启动参数-m 44M限定的最大内存使用完毕，如果某一个slabclass没有slab，那么依然可以分配一个page，就会出现内存大于44m的情况。在do\_slabs_newslab函数中
	3. 如果都没有过期，并且不能够分配新的slab，那么就会强行霸占没有过期的item。返回该item

    有一篇大牛的文章（2009年）中提到了相同的问题。 http://timyang.net/data/Memcached-lru-evictions/ 

2. 场景：
	我以下面的方式启动
	
		memcached -p 11211 -m 44m -vv -f1.25

下面的方式写入数据

		for (i = 0; i < 10922 * 44m; i++)
		{
			set( i, '1', 0); // 写入长度为1，0代表过期时间
		}

这样的话44m空间全部用完。当我们再次写入长度为1000的，将会写入到 slab class 12中。但是我们知道44m空间，已经用完，但实际上我们发现slab class 12可以写入。这儿刚好验证了【注意】中的内容

##LRU队列：

上面讲述了如何分配item，那么分配以后的item如何进行管理和查找呢，就需要LRU队列和Hash表。LRU队列管理Item，LRU队列是一个双向链表，个数与subclass的个数相同。同一个subclass的所有item都归同一个LRU队列管理。

在item.c中

	static item *heads[LARGEST_ID];
	static item *tails[LARGEST_ID];
	static crawler crawlers[LARGEST_ID];
	static itemstats_t itemstats[LARGEST_ID];
	static unsigned int sizes[LARGEST_ID];
	
通过LRU队列的操作，可以更清楚明白LRU队列的作用：

1. 插入：插入到LRU队列的头部
2. 更新：在LRU队列中，排的越靠后就认为越少被使用的item，那么其被淘汰的概率就越大，所以新的item（访问时间新）要排在不那么新的item前面，所以插入在头部是必须的。
do\_item_update就是先把item从LRU队列中删除，再插入LRU队列，更新time字段（time表示最后访问时间）

## Hash表
1. 当item分配并赋值以后，需要注册到LRU队列和Hash表中。注册到Hash表以后，将方便我们进行查询。利用item的数据部分（data字段）和键长（nkey字段）进行hash。Hash表的结构是常见的拉链结构。
2. Hash表的扩展
	Hash表的长度并不是不变的。当Hash表中的Item数量达到了Hash表长度的1.5倍时，那么就会扩展哈希表的长度，然后重新进行Hash
	由于这一步耗时，是通过专门的线程 迁移线程进行的。
3. Hash表与Item的关系
	Item在分配以后，会通过do\_item_link do_\item_unlink将Item注册到Hash表和LRU队列中

## 过期失效处理
1. 一个item在两种情况下会过期失效

		item的expire时间戳到了
		用户使用flush_all将全部item变成过期失效
		
2. 懒惰删除
	只有在进行get时，才会进行判断，然后进行删除



## 重要原则：
1. 按照预定大小，将分配的slab分割成特定长度的块
2. 分配给某个slab的内存不能给其他slab使用。已经分配出去的slab不会被回收或者重新分配，因为这种原理，也就出现了【注意】里面所描述的情况
3. item里面不仅仅存放缓存对象的value，还包括key，expire，time，flag等详细信息。其中time表示最后一次访问时间，expire表示过期时间。


## 场景
场景1：
	一开始大量长度为1kb，全部占用，之后存储大量10kb，只有一个slab，那么10kb大量访问，那么就会出现没到过期时间，数据全部失效。
