---
title: mysql 锁的问题
date: 2016-01-09 14:52:01 +0800
tags: mysql 
---
# 6.5 锁问题

## 0. 概述
通过锁定机制可以实现事务的隔离性要求，使得事务可以并发地工作。锁提高了并发，但是却会带来潜在的问题。不过好在因为事务隔离性的要求，锁只会带来三种问题，如果可以防止这三种情况的发生，那将不会产生并发异常。这三种问题分别是脏读（dirty read),不可重复读（Non-repeatable read），更新丢失。

## 1. 脏读

脏数据是指未提交的数据，如果读到了脏数据，即一个事务可以读到另外一个事务中未提交的数据，则显然违反了数据库的隔离性。脏读指的就是在不同的事务下，当前事务可以读到另外事务未提交的数据，简单来说就是可以读到脏数据。

在下表中，事务的隔离级别由默认的可重复读（repeatable read)变成了未提交读( read uncommitted).SessionB读取了SessionA中没有提交的数据，违反了数据库的隔离性。
		
|step 	| session A 			| sessionB	     |
|---	|---					|	---			     |
|1    	|set @@tx_isolation='read-uncommitted'	|--|
|2    	|--| set @@tx_isolation='read-uncommitted'|
| 3    |start transaction|start transaction|
| 4    |--|select mtime from learn2 where id = 9; *** 9.row *** mtime = 9000|
|5	|update learn2 set mtime = mtime - 100 where id = 9|---|
|6    	|--| select * from learn2 where id = 9 mtime = 8900|

在MysqlInnoDb中虽然是脏读，但如果在sessionB中执行update learn2 set mtime = mtime - 100 where id = 9;依然需要等待，sessionA中为id=9这一行加锁了


## 2. 不可重复读

1. 在一个事务的两次查询之中数据不一致，这可能是两次查询过程中间插入了一个事务更新的原有的数据。在第一个书屋的两次读数据之间，由于第二个事务的修改并提交commit以后，那么第一个事务的两次读到的数据就可能不一样，这样在一个事务内两次读到的数据是不一样的情况，称为不可重复读。
2. 不可重复读和脏读区别	
	
|脏读 	| 不可重复读| 
|---|---|
|读取到没有提交的数据|读取到其他事务已经提交的数据|
|违反了隔离性| 违反了一致性|
	
举例

|step 	| session A 			| sessionB	     |
|---	|---					|	---			     |
|1    	|set @@tx_isolation='read-committed'	|--|
|2    	|--| set @@tx_isolation='read-ncommitted'|
|3    |start transaction|start transaction|
|4    |--|select mtime from learn2 where id = 9; ### *** 9.row *** mtime = 8900|
|5	|update learn2 set mtime = mtime - 100 where id = 9|---|
|6    	|--| select * from learn2 where id = 9 #mtime = 999|
|7    	|commit;| ---|
|8    	|---| select * from learn2 where id = 9 #mtime = 8800|

在会话A step5更新后，会话B step6没有发生脏读现象。但在commit以后，会话B step8得到的mtime与会话B step6的mtime取值不同。

在InnoDB存储引擎中，通过使用Next-Key Lock算法来避免不可重复读的问题。在MySQL官方文档中将不可重复读的问题定义为Phantom Problem即幻像问题。在Next-Key Lock算法下，对于索引的扫描，不仅是锁住扫描到的索引，而且还锁住这些索引覆盖的范围（gap）。因此在这个范围内的插入都是不允许的。这样就避免了另外的事务在这个范围内插入数据导致的不可重复读的问题。因此，InnoDB存储引擎的默认事务隔离级别是READ REPEATABLE，采用Next-Key Lock算法，避免了不可重复读的现象。 

## 3. 更新丢失


case1 

|step 	| session A 			| sessionB	     |
|---	|---					|	---			     |
|1    	|start transaction	|start transaction|
|2    	|select * from learn2 where id = 9; mtime = 9999| select * from learn2 where id = 9 mtime = 9999|
| 3    |update learn2 set mtime = mtime - 999 where id = 9|--|
|4		|--|update learn2 set mtime = mtime - 100 where id = 9|
|5     |commit|---|
|6     |---|commit|
|7    	|select * from learn2 where id = 9; mtime = 8900| select * from learn2 where id = 9 mtime = 8900|

第3步update操作锁住id=9这一行（排它锁），第4步sessionB更新的时候进行等待，只有第5步commit以后第4步的update才更新成功。
由于排它锁的存在并且update基于是mtime的数据库取值，保证了不会存在```丢失更新```的问题。但如果update xx set mtime = some_value 这样的操作，那么就会出现```丢失更新```的现象。 

case2 

|step 	| session A 			| sessionB	     |
|---	|---					|	---			     |
|1    	|start transaction	|start transaction|
|2    	|select * from learn2 where id = 9; mtime = 9999| select * from learn2 where id = 9 mtime = 9999|
| 3    |update learn2 set mtime = 999 where id = 9|--|
|4		|--|update learn2 set mtime = 100 where id = 9|
|5     |commit|---|
|6     |---|commit|
|7    	|select * from learn2 where id = 9; mtime = 100| select * from learn2 where id = 9 mtime = 100|


如何避免这种```丢失更新```的情况发生呢
case3 

|step 	| session A 			| sessionB	     |
|---	|---					|	---			     |
|1    	|start transaction	|start transaction|
|2    	|select * from learn2 where id = 9 for update; mtime = 9999| ---|
|3    	|---| select * from learn2 where id = 9 for update; # 等待 其真正发生是在sessionA的commit以后|
| 4    |一系列操作|一系列操作|
| 5    |update learn2 set mtime = 999 where id = 9|--|
|4		|--|update learn2 set mtime = 100 where id = 9|
|5     |commit|---|
|6     |---|commit|
|7    	|select * from learn2 where id = 9; mtime = 100| select * from learn2 where id = 9 mtime = 100|


在这个例子中，sessionB的select ..for update操作是在sessionA commit以后才会发生。commit之前，sessionB一直处于等待状态。sessionA提交以后，select的结果是mtime = 999;

在上述的例子中为什么不直接允许UPDATE语句，而首先要进行SELECT…FOR UPDATE的操作。的确，直接使用UPDATE可以避免丢失更新的问题发生。在实际应用中，应用程序可能需要首先检测用户的余额信息，查看是否可以进行转账操作，然后再进行最后的UPDATE操作，因此在SELECT与UPDATE操作之间可能还存在一些其他的SQL操作。

有用的mysql命令

	查询隔离级别
	SELECT @@global.tx_isolation; 
	SELECT @@session.tx_isolation; 
	SELECT @@tx_isolation;
	设置隔离级别
	set @@tx_isolation='read-committed'



参考文章：
    http://techlog.cn/article/list/10182853
    姜承尧. “MySQL技术内幕：InnoDB存储引擎 第2版”
