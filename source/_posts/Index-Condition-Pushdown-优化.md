---
title: Index Condition Pushdown 优化
date: 2016-06-26 08:24:56
tags: mysql
---

Index Condition Pushdown是一种优化，针对的是使用索引去表中获取记录rows的优化。没有ICP的话，存储引擎遍历索引来定位数据表中的记录（rows），并将其返回给Mysql的server层，在server层对所获取的记录进行WHERE条件的过滤。
开启ICP后，如果WHERE条件中的某些字段刚好是索引中的字段，那么Mysql的server层会将这些字段下放到存储引擎，存储引擎会使用这些WHERE条件进行过滤，只有满足这些字段的条件，存储引擎才会去数据表中读取整行记录。

ICP的优点就是：减少了存储引擎到达数据表的次数，以及MySQL server层到达存储引擎的次数。

能够使用ICP优化的索引：range，ref， eq\_ref和ref\_or_null。这些索引都需要获取全表的记录。
能够使用ICP优化的引擎：InnoDB和MyISAM。注意 在MySQL5.6的分区表中不支持，MySQL5.7中支持。
注意：只支持辅助索引。因为ICP优化的目标：减少读取整行的次数，从而降低了IO操作。对于InnoDB的聚簇索引，已经读取了整行记录，因此使用ICP并不会降低IO。

优化器没有使用ICP时，数据访问和提取的过程如下：

	1) 当storage engine读取下一行时，首先读取一个索引，然后使用索引在数据表中定位和读取整行数据。
	2) sever层评估where条件，如果该行数据满足where条件则使用，否则丢弃。
	3) 执行1），直到最后一行数据。
![no-icp](/images/no_icp.png)

优化器使用ICP时，server层会将WHERE条件中某些字段（这些字段在索引中）下推到storage engine层。数据访问和提取过程如下：

	1) storage engine从索引中读取下一条索引。
	2) storage engine使用索引评估下推的索引条件。如果没有满足wehere条件，则回到1）。只有当索引满足下推的索引条件的时候，才会继续去数据表中读取数据。
	3) 如果满足WHERE的索引条件，storage engine通过索引定位数据表的行和读取整行数据并返回给server层。
	4) server层评估没有被下推到storage engine层的where条件，如果该行数据满足where条件则使用，否则丢弃。
![icp](/images/icp.png)


举例来说明：一个表中包含一个人和其地址信息，表中有一个索引(zipcode, lastname, firstname)。如果我们知道一个人的zipcode，但不确定其lastname，因此可以像下面一样进行查询

	SELECT * FROM peope WHERE zipcode = '95054' 
	AND lastname LIKE '%etrunica%'
	AND address LIKE '%Main Street%';

zipcode = '95054'可以使用索引，但是第二部分lastname LIKE '%etrunica%'，不使用ICP时不能限制需要扫描的记录的行数，需要获取所有满足zipcode = '95054'的所有用户记录。

如果使用ICP，在去数据表读取整行记录之前，MYSQL会检查lastname LIKE '%etrunica%'部分。这样避免了去读取不满足lastname 这个条件的数据记录。

默认ICP是开启的。可以通过index\_condition\_pushdown来设置。

	 set optimizer_switch='index_condition_pushdown=off';

特别注意：
1. ICP只能用于二级索引，不能用于主索引
2. 也不是全部where条件都可以用ICP筛选，如果某where条件的字段不在索引中，当然还是要读取整条记录做筛选，在这种情况下，仍然要到server端做where筛选。
3. ICP的加速效果取决于在存储引擎内通过ICP筛选掉的数据的比例
4. 概括之：对于完全命中索引的查询，不需要ICP。比如索引是idx_abc(a,b,c)。查询是where a = 1 and b = 2 则不需要ICP。如果查询是 where a = 1 and b > 3 则会使用ICP.

## 参考文章

1. http://dev.mysql.com/doc/refman/5.6/en/index-condition-pushdown-optimization.html
2. http://ourmysql.com/archives/1351
3. http://mdba.cn/?p=315
4. https://mariadb.com/kb/en/mariadb/index-condition-pushdown/
