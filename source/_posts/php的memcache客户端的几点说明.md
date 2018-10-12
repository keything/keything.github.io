---
title: php的memcache客户端的几点说明
date: 2016-07-03 20:55:17
tags: [nosql,php]
---


## 一. 超时时间
	
1. 对于php的memcache客户端，当客户端认为memcached服务器端超时的时候，客户端会主动断开与memcached服务器之间的连接。
	
2. 对于php的mecache客户端，这个超时时间并不是最低只能是1秒。http://php.net/manual/zh/memcache.addserver.php 中写到timeout的单位是秒，默认是1秒。可是我们可以通过修改变量来调整超时时间。 【http://tech.uc.cn/?p=326 文章中的关于超时时间的论断是错误的】
		
		ini_set("memcache.default_timeout_ms", 1000);
		

## 二. 失败重连机制

先提出两个问题，第一个问题：

	有n个memcache服务器的时候，某一个server失败了，
	那么还会重试下一个server么？
	哪些操作会重试下一个server？

第二个问题：

	当n个server都失败的时候，
	会影响到之后的其他操作么？

这两个问题，稍后会给出详细的解释。

## 三. 实践前基础知识

1. tc命令控制网络延时

 	
 		tc qdisc add dev eth0 root netem delay 100ms 所有请求都延时100ms
    
    	tc qdisc del dev eth0 root netem delay 100ms 删除所有请求都延时100ms的限制

2. 实践的php代码

	有两台memcache服务器，分别是10.16.57.128和10.16.57.191. 

		// test_mc.php
		<?php
		function catchErrHandler($level, $msg, $file, $line)
		{
		    switch ($level)
		    {
		    case E_NOTICE:
		        $hhh = __METHOD__.__LINE__.'|pid='.getmypid().'notice'.'|msg='.$msg.microtime(true)."\n";
		        echo $hhh;
		        error_log($hhh,3,'/tmp/yk.log');
		        //exit;
		        break;
		    default:
		        error_log('default'.__METHOD__.__LINE__.'|pid='.getmypid().'notice'.'|msg='.$msg.microtime(true)."\n",3,'/tmp/yk.log');
		        break;
		    }
		}
		ini_set("memcache.default_timeout_ms", 99);
		ini_set('memcache.allow_failover', 1);
		set_error_handler('catchErrHandler');
		$mcobj = new Memcache();
		$mcobj->addServer('10.16.57.191', 11211);
		$mcobj->addServer('10.16.57.128', 11211);
		$pid = getmypid();
		$max_qid = 1;
		for ($qid = 0; $qid < $max_qid; $qid++)
		{
		    $start = microtime(true);
		    $key = 'task_lock' . $qid;
		    echo $key . "\n";
		    $value = $mcobj->set($key, 1, 300);
		    $end = microtime(true);
		    $consume = $end - $start;
		}

## 四. 实践


1. 当一个server失败了，会尝试下一个server么？

	memcache客户端关于重试failover有两个设置：
	
		memcache.allow_failover boolean 是否在发生错误时（对用户）透明的转移到其他服务器。
		memcache.max_failover_attempts integer 定义在写入和获取数据时最多尝试的服务器次数（即：故障转移最大尝试数），仅和 memcache.allow_failover结合使用。
		
	如果设置 ``ini_set('memcache.allow_failover', 1);``那么会重试下一个server。
	
2. 哪些操作会去重试下一个呢？
	
	从源码来看，php的memcache客户端发送``set/add/replace``指令的时候会先判断server可用不，如果不可用则走failover，如果只有一台memcache服务器，那么这个逻辑就忽略。
因为server不可用，那么在之后的逻辑就不会发送请求。
	
	从实践来看，``get``操作也会走failover，这也就是 http://tech.uc.cn/?p=326 文章中提到的memcache的分布式问题。
	
	实践步骤：
	
	a. 两台memcache机器，10.16.57.128，10.16.57.191。
	10.16.57.191这台机器延时100ms 命令：``sudo tc qdisc add dev eth0 root netem delay 100ms``。
	
	b. 执行test_mc.php代码
	
	通过wireshark抓包可以看到，10.16.57.191在三次握手的时候，客户端直接发了RST。然后failover，跟10.16.57.128进行三次握手，之后发送set请求，memcache服务器对其进行处理并返回STORED。add, replace, get操作也是一样的会进行failover。

	![set](/images/php_memcache/set.png)
	![replace](/images/php_memcache/replace.png)
	![get](/images/php_memcache/get.png)

	
	
3. 当两个memcache服务器都失败的时候呢？？
	
	retry\_interval是服务器连接失败时重试的间隔时间，默认值15秒。一旦一个连接失败，他将会被成功重新连接，或者被标记为失败连接并等待retry\_interval秒后再践行一次重连。即，当memcache客户端将server的连接标识为失败以后，在retry\_interval时间内都不会再次建立连接，而是直接返回false。
	
	实践步骤：
	
	a. 两台memcache机器， 10.16.57.128，10.16.57.191。
	10.16.57.191和10.16.57.128两台机器延时100ms 命令：``sudo tc qdisc add dev eth0 root netem delay 100ms``。
	
	b. 执行test_mc.php代码
	
	通过wireshark抓包可以看到，客户端分别对两个memcache服务器发送RST，之后并没有请求再发出，而是直接返回了false
	
	![不会进行重试](/images/php_memcache/all_failed.png)
	
	

		

   
## 五. 连接twemproxy和连接memcache

连接twemproxy和直接连接memcache时的failover情况：


1. php直连memcache时

	三次握手时间超过设置的超时时间，使用pconnect、addserver和connect三种方式都能准确failover
	
	即直连memcache时，长短连接都能够准确failover。

2. php连接twemproxy时，

如果只有一台twemproxy（或者通过lvs连接twemproxy集群），即addServer的时候只有一个ip:port，那么不会走failover。

a. PHP设定的timeout 大于 twemproxy的timeout，memcache故障实体机可以及时剔除，connect不出现false，但是pconnect和addserver进程队列里的将继续false很久后才恢复到true；原因：

	虽然twemproxy已经剔除故障机，但是只有一个ip:port, 对于php的memcache客户端而言，他就认为这个ip:port的连接不可用，那么需要在retry_interval时间以后才会发送请求。
	
	对于短连接connect而言，则是新建连接，则不存在这个问题。


b. PHP设定的timeout < twemproxy的timeout，mc故障实体机不一定及时剔除，pconnect、addserver和connect都将继续false很久后才恢复到true。

 
## 六. 出现问题

1. 当切换到twemproxy的时候，发现大量的key写入失败，原因是前端机（跑着php的memcache客户端的机器）的TIME\_WAIT非常高，twemproxy机器的TIME\_WAIT和CLOSE_WAIT都非常高。

2. twemproxy机器的TIME\_WAIT达到18W， CLOSE_WAIT（被动关闭）也非常高。

	TIME\_WAIT高的原因是，twemproxy没有收到memcache服务器的响应，主动断开连接。
CLOSE_WAIT高的原因是，php的memcache客户端没有收到twemproxy服务器的响应，断开连接，造成twemproxy服务器的CLOSE\_WAIT过高， 就是客户端主动关闭，服务器端没有快速响应，因此处于CLOSE\_WAIT。



## 七. 解决方案

1. 调整内核参数解决TIME_WAIT过高的问题

		net.ipv4.tcp_tw_reuse = 1
		net.ipv4.tcp_syncookies = 1
		net.ipv4.tcp_tw_recycle = 0
		net.ipv4.tcp_max_tw_buckets = 5000
		
2. CLOSE_WAIT过高的问题，没有解决

3. 对于twemproxy，可能是我们的使用方式有问题，在使用的过程中出现了twemproxy机器的TIME\_WAIT和CLOSE_WAIT都过高，影响服务，因此我们没有使用twemproxy。






