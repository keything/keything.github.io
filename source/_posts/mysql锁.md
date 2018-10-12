---
title: chapter6:mysql锁
date: 2016-07-28 08:52:01 +0800
tags: mysql
---

## 6.1 什么是锁

锁机制用于管理对共享资源的并发访问

MyISAM引擎的锁是表锁。

InnoDB提供一致性的非锁定读、行级锁支持。行级锁没有相关额外开销，并可以同时得到并发性和一致性

## 6.2 lock与latch

一般我们提到的是lock。lock的对象是事务，用来锁定的是数据库中的对象，如表，页，行。并且一般lock的对象只在事务commit和rollback后进行释放（不同隔离级别有所不同）。

latch的目的是用来保证并发线程操作临界资源的正确性，并且没有死锁检测的机制。


## 6.3 InnoDB存储引擎中的锁

### 6.3.1 锁的类型

InnoDB实现了如下两种标准的行级锁：
    
    1. 共享锁（S Lock）允许事务读一行数据
    2. 排他锁（X Lock）允许事务删除或更新一行数据

InnoDB支持意向锁，其意向锁即为表级别的锁。


### 6.3.2 一致性非锁定读 consistent nonlocking read

一致性非锁定读指InnoDB存储引擎通过行多版本控制的方式来读取当前执行时间数据库中行的数据。如果读取的行正在执行DELETE或UPDATE操作，这时读取操作不会因此等待行上锁的释放，而是会读取行的一个快照数据。

快照数据是指该行的之前版本的数据，该实现是通过undo段来完成，undo段是用来在事务中回滚数据因此没有额外的开销。

在不同事务隔离级别下，读取方式的不同，并不是在每个事务隔离级别下都是采用非锁定的一致性读。此外，即使都是使用非锁定的一致性读，但是对于快照数据的定义也不相同。

在事务隔离级别READ COMMITTED和REPEATABLE READ下，InnoDB存储引擎使用非锁定的一致性读，但是快照数据定义不相同。在REPEATABLE READ隔离级别下，非一致性锁定读总是读取事务开始时的行数据版本。在READ COMMITTED下，总是读取被锁定行的最新一份快照数据。


### 6.3.3 锁定读 [locking reads](http://dev.mysql.com/doc/refman/5.7/en/innodb-locking-reads.html)
在某些情况下用户需要显式对数据库读取操作进行加锁以保障数据逻辑的一致性。即使对于SELECT的只读操作，也要支持加锁语句。

支持两种一致性的锁定读（locking read)操作：

    SELECT .. FOR UPDATE
    SELECT .. LOCK IN SHARE MODE

## 6.4 锁的算法

### 6.4.1 行锁的3种算法

InnoDB有3种行锁算法，其分别是：

    Record Lock：单个行记录上的锁
    Gap Lock：间隙锁，锁定一个范围，但不包含记录本身
    Next-Key Lock: Gap Lock + Record Lock 锁定一个范围并且锁定记录本身。

在默认隔离级别REPEATABLE READ下，使用的是Next-Key Lock。对于查询的索引含有唯一属性时，InnoDB会将Next-KeyLock进行优化，降级为RecordLock，即仅锁住索引本身，而不是范围。

而对于辅助索引，其加上Next-KeyLock InnoDB存储引擎还会辅助索引下一个键值加上gapLock，即会锁住这个辅助索引附近的两个范围。


### 6.4.2 解决幻象问题

幻象是指在同一个事务下，连续执行两次同样的SQL语句可能导致不同的结果，第二次SQL可能会返回之前不存在的行。

在REPEATABLE READ下采用Next-key Lock加锁

在READ COMMITTED下 只是采用RecordLock。


#### 通过InnoDB存储引擎的Next-Key Locking 机制在应用层面实现唯一性检查。

一个场景：想查询一下某个id是否存在，如果不存在则插入，如果存在则不插入。两种做法：外部加锁或者通过数据库。

数据库方式：

    select * from table where col = xx LOCK IN SHARE MODE;
    if not found any row:
        # unique for insert value 需要是唯一索引，不然也存在多条插入的情况
        INSERT INTO table VALUES (...);



## 6.5 [锁问题](http://keything.github.io/2016/01/09/mysql-%E9%94%81%E7%9A%84%E9%97%AE%E9%A2%98/)

### 6.5.1 脏读
脏读：指的是在不同事务下，当前事务可以读到另外事务未提交的数据
### 6.5.2 不可重复读
不可重复读：指的是在一个事务内两次读到的数据不一样，多了或者少了数据。
### 6.5.3 丢失更新
丢失更新：简单来说，就是一个事务的更新操作会被另一个事务的更新操作所覆盖，从而导致数据的不一致。
解决方法：让事务在这种情况下的操作变成串行化。比如使用select ... FOR UPDATE.




### 几个概念
多版本技术：一个行记录可能有不止一个快照数据，称之为行多版本技术。


由此带来的并发控制，称之为多版本并发控制 multi version concurrency controll mvcc。


## 6.6 参考文章

1. [15.5 InnoDBlocking and Transaction Model](http://dev.mysql.com/doc/refman/5.7/en/innodb-locking-transaction-model.html)
2. 姜承尧 InnoDB存储引擎第二版 6.锁
