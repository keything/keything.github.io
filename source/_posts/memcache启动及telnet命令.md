---
title:  "2. Memcache启动及Telnet命令"
date:   2016-04-17 13:00:01 +0800
tags: [nosql,memcache]
---

## 1. Memcached启动选项

1. "p:"   小写p，memcached监听的tcp端口。默认端口为11211, 该选项的参数赋值给settings.port
2. "m:"  小写m。memcached能够使用的最大内存值，默认是64MB。参数单位为MB。该参数赋值给settings.maxbytes
3. "v"    小写v。输出memcached运行时的一些信息。-v -vv -vvv输出的信息依次增加。该选项会增加settings.verbose的值
4. "d"    以守护进程的形式运行memcached
5. "f:"    item的扩容因子。默认值为1.25。该选项的参数值可以是小数但必须大于1.0。该选项参数将赋值给settings.factor
6. "n:"   设置最小的item能存储多少字节的数据。该选项参数赋值给settings.chunk_size.
7. "C:"  大写C。memcached默认是使用CAS的，本选项是禁用CAS。本选项会将settings.use_cas赋值为false
8. "B:"  memcached支持文本协议和二进制协议。该选项的参数用于指定使用的协议。默认情况下是根据客户端的命令而自动判断(也叫协商)，参数只能取auto、binary、ascii这三个字符串值。将参数将赋值给settings.binding_protocol
9. "I:"   大写i。slab分配器中，每一个页的大小。这个选项的参数是一个数值表示页的大小。默认单位是B也可以在数值后面带K或者M(大小写都行)，表示KB和MB。页的大小小于1KB或者大于128MB都是不允许的。不推荐使用该选项。本选项参数会赋值给settings.item_size_max
10. "P:"  大写P。指定pid文件。存放当前mc进程的pid
11. "t:"    该选项的参数用于指定worker线程的个数，不建议超过64个。如果不设置该选项默认有4个线程。该参数会赋值给settings.num_threads
12. "c:"   小写c。最多允许多少个客户端同时在线(这个值不等价于listen函数的第二个参数)，该选项和后面的b选项有所不同。 默认值为1024个。该选项参数赋值给settings.maxconns。
13. "u:"  小写u。当以root用户启动memcached的时候需要指定memcached的所属用户，其他用户启动memcached不需要此选项

举例
	
	memcached -d -p 11211 -P /var/run/memcached.pid -c 4096 -t 4 -m 1024 -u root -v

## 2. 一些高级telnet命令

stats slabs 
查看 memcache的slab class的状态，能够知道数据分布的范围，每个slab有多少页

stats cachedump 1 0
第三个字段是slab的标识 
第四个字段表示拉取的该slab下的数据，第四个字段0表示不受限制（但实际上只会拉取1M的数据， 并不会全拉）

	
## 3. 基础Memcached命令
Memcached 有 4 种类型的命令:
    1. 存储命令(set/add /replace/append/prepend)
    指示服务器储存一些由键值标识的 数据。客户端发送一行命令,后面跟着数据区块;然后,客户端等待接收服务器回传 的命令行,指示成功与否。
    2. 读取命令(get/bget/gets)指示服务器返回与所给键值相符合的数据(一个请求中右 一个或多个键值)。客户端发送一行命令,包括所有请求的键值;服务器每找到一项内容,都会发送回客户端一行关于这项内容的信息,紧跟着是对应的数据区块;直到服务器以一行“END”回应命令结束。
    3. 状态命令(stat)被用于查询服务器的运行状态和其他内部数据。 
    4. 其他命令,如 flush_all,version,quit 等。


	COMMAND	DESCRIPTION	EXAMPLE
	get	Reads a value	get mykey
	set	Set a key unconditionally	set mykey 0 60 5
	add	Add a new key	add newkey 0 60 5
	replace	Overwrite existing key	replace key 0 60 5
	append	Append data to existing key	append key 0 60 15
	prepend	Prepend data to existing key	prepend key 0 60 15
	incr	Increments numerical key value by given number	incr mykey 2
	decr	Decrements numerical key value by given number	decr mykey 5
	delete	Deletes an existing key	delete mykey
	flush_all	Invalidate specific items immediately	flush_all
	Invalidate all items in n seconds	flush_all 900
	stats	Prints general statistics	stats
	Prints memory statistics	stats slabs
	Prints memory statistics	stats malloc
	Print higher level allocation statistics	stats items
	stats detail
	stats sizes
	Resets statistics	stats reset
	version	Prints server version.	version
	verbosity	Increases log level	verbosity
	quit	Terminate telnet session	quit
 

### stats命令详解

	pid	memcache服务器的进程ID
	uptime	服务器已经运行的秒数
	time	服务器当前的unix时间戳
	version	memcache版本
	pointer_size	当前操作系统的指针大小（32位系统一般是32bit）
	rusage_user	进程的累计用户时间
	rusage_system	进程的累计系统时间
	curr_items	服务器当前存储的items数量
	total_items	从服务器启动以后存储的items总数量
	bytes	当前服务器存储items占用的字节数
	curr_connections	当前打开着的连接数
	total_connections	从服务器启动以后曾经打开过的连接数
	connection_structures	服务器分配的连接构造数
	cmd_get	get命令（获取）总请求次数
	cmd_set	set命令（保存）总请求次数
	get_hits	总命中次数
	get_misses	总未命中次数
	evictions	为获取空闲内存而删除的items数（分配给memcache的空间用满后需要删除旧的items来得到空间分配给新的items）
	bytes_read	总读取字节数（请求字节数）
	bytes_written	总发送字节数（结果字节数）
	limit_maxbytes	分配给memcache的内存大小（字节）
	threads	当前线程数




参考文章
http://blog.mimvp.com/2015/01/memcache-start-telnet-command-xiangjie/
