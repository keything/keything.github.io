---
title: hive权威指南学习笔记
date: 2018-09-23 23:02:05
tags:
---

Hive操作


本篇文章是对 `https://cwiki.apache.org/confluence/display/Hive/GettingStarted#GettingStarted-DDLOperations` 

以及

《Hive编程指南》的学习总结


1. DDL Operations (data defination language)

+ create table pokes (foo INT, bar STRING);
+ create table invites (foo INT, bar STRING) PARTITIONED BY (ds STRING);

partition column 是一个虚拟列，并不是数据本身的一部分，但是可以获得某一份特定数据集。 

+ show tables;
+ show tables '.*s'
+ describe invites;

2. DML Operations (data manipulation language)

### 2.1 装载数据

+ LOAD DATA LOCAL INPUT './examples/files/kv1.txt' OVERWRITE INTO TABLE pokes;
+ LOAD DATA LOCAL INPATH './examples/files/kv2.txt' OVERWRITE INTO TABLE invites PARTITION (ds='2008-08-15');
+ LOAD DATA LOCAL INPATH './examples/files/kv3.txt' OVERWRITE INTO TABLE invites PARTITION (ds='2008-08-08');

`LOCAL` 表明这是在本地文件系统上的文件；如果没有的话，那么就从hdfs上获取文件。

`OVERWRITE` 表明表中已经存在的数据将会被删除。 如果没有的话，数据文件被添加到已经存在的数据集合中。 

### 2.2 通过查询语句插入数据


+ `FROM staged_employees se 
  INSERT OVERWRITE TABLE employee PARTITION (country = 'US' and state = 'OR') select *  where se.country = 'US' and se.state = 'OR'
  INSERT OVERWRITE TABLE employee PARTITION (country = 'US' and state = 'CA') select *  where se.country = 'US' and se.state = 'CA'
  `

#### 动态分区插入

前面的语法中有一个问题，即：如果需要创建非常多的分区，那么用户就需要写非常多的SQL。
幸运的是：Hive提供了一个动态分区功能，可以基于查询参数推断出需要创建的分区名称。相比之下，前面看到的都是静态分区。 

`INSERT OVERWRITE TABLE employees PARTITION (country, state) 
 SELECT ..., se.cnty, se.st FROM staged_employees se;
`

注意：Hive 根据SELECT语句中最后2列来确定分区字段country和state的值。
而不是根据命名来匹配的。

在SQL语句使用不同的命名就是为了强调源表字段值和输出分区值直接的关系是根据位置而不是根据命名来匹配的。 

### 2.3 单个查询语句中创建表并加载数据

+ create table u_data_v2 as SELECT userid, rating from u_data limit 10;

### 2.4 导出数据

+ INSERT OVERWRITE LOCAL DIRECTORY '/tmp/hive' SELECT userid, rating FROM u_data; 


3. SQL操作

### SELECT ... FROM 语句
3.1 数据类型

 当用户选择的列是集合数据类型时，Hive会使用JSON语法进行输出。

+ 当列是一个数组时，其值使用一个括在`[...]`的逗号分隔的列进行表示，如["Mary Smith", "Todd Jones"]
+ 当列是一个Map时，使用JSON格式来表达map，即使用一个被括在`{...}`内的以逗号分隔的键值对列表进行表示；如 `{"Federal Taxes":0.2 "State Taxes": 0.05}`
+ 当列是一个Struct时，使用map格式来表示，如`{"Federal Taxes":0.2 "State Taxes": 0.05}`

引用元素的方式：
+ 引用Map元素，使用ARRAY[...]语法；
+ 引用Struct的一个元素，使用`点`符号。

3.2 使用函数


3.3 列别名

如果新产生的结果在源表中不存在的话，通常有必要给这些新产生的列起一个名称，也就是别名。 

+ select userid as uid from u_data;

3.4 嵌套SELECT 语句

`
FROM (
	SELECT upper(name), salary ,deductions["Federal Taxes"] as fed_taxes FROM employees
	) e
SELECT e.name, e.fed_taxes where e.name = "yan"
`

从这个嵌套语句中可以看到，我们将前面的结果集起了个别名，称之为e，在这个语句外面嵌套查询了name fed_taxes两个字段。 同时约束name必须是yan。

3.5 CASE ... WHEN ... THEN 句式

CASE .. WHEN .. THEN 语句和if条件语句类似，用于处理单个列的查询结果。 

`SELECT userid, movieid, 
	CASE 
	WHEN rating <= 1 THEN "low" 
	WHEN rating > 1 and rating <= 3 THEN "middle" ELSE "high"  
	END  as level 
	from u_data limit 10;
`

###  4. WHERE 语句 （对应Hive编程指南的6.2 WHERE语句）


#### 4.1  WHERE语句不能使用`列别名`。 

`
SELECT name, salary,  salary * (1-deductions["Federal Taxes"]) as salary_minus_fed_taxes 
FROM employees
WHERE round(salary * (1-deductions["Federal Taxes"])) > 70000
`

这个查询语句里面，有重复的表达式。下面的查询语句使用一个列别名来消除重复问题，但是并不能生效。 

`
SELECT name, salary,  salary * (1-deductions["Federal Taxes"]) as salary_minus_fed_taxes 
FROM employees
WHERE round(salary_minus_fed_taxes) > 70000;
报错：Invalid table alias or column reference 'salary_minus_fed_taxes'
`

正如错误信息所提示的，WHERE语句不能使用`列别名`。
不过我们可以使用一个嵌套的SELECT语句

`
SELECT e.* FROM 
(SELECT name,salary, salary * (1-deductions["Federal Taxes"]) as salary_minus_fed_taxes FROM employees) e 
WHERE round(e.salary_minus_fed_taxes) > 70000;
`

#### 4.2 谓词操作符

#### 4.3 GROUP BY语句

GROUP BY 通常会和聚合函数一起使用，按照一个或者多个列队结果进行分组，然后对每个组执行聚合操作。 


### 4.4 JOIN 语句

Hive支持通常的SQL JOIN语句，但是只支持`等值连接`，并且在ON子句中只支持AND。

4.4.1 INNER JOIN

`
SELECT a.ymd, a.price_close, b.price_close, c.price_close 
FROM stocks a JOIN stocks b ON a.ymd = b.ymd
			  JOIN stocks c ON a.ymd = c.ymd
WHERE a.symbol = 'AAPL' AND b.symbol = 'IBM' AND c.symbol = 'GE';
`

在这个例子中，会首先启动一个MapReduce job对表a和表b进行连接操作，然后会再启动一个MapReduce job将第一个MapReduce job的输出和表c进行连接操作。 

Hive表总是按照从左到右的顺序执行的。 

#### JOIN优化

Hive同时假定查询中最后一个表是最大的那个表。在对每行记录进行连接操作时，它会尝试将其他表缓存起来，然后扫描最后那个表进行计算。

因此用户需要保证`连续查询中的表的大小是从左到右依次增加的`。

#### LEFT OUTER JOIN
#### OUTER JOIN
#### RIGHT OUTER JOIN
#### FULL OUTER JOIN
#### LEFT SEMI-JOIN
#### 笛卡尔积JOIN
#### map-side JOIN

### 4.5 ORDER BY 和 SORT BY

### 4.6 含有SORT BY 的DISTRIBUTE BY


### 4.10 UNION ALL

UNION ALL将2个或多个进行合并。每个union子查询都必须有相同的列，并且每个字段的字段类型必须是一致的。

比如，如果第2个字段是FLOAT类型，那么所有其他子查询的第2个字段必须都是FLOAT类型的。

举例，将日志数据进行合并的例子:

``
SELECT log.ymd, log.level, log.message 
	FROM(
			SELECT l1.ymd, l1.level, l1.message, 'Log1' AS source
			FROM log1 l1
		UNION ALL
			SELECT l2.ymd, l2.level, l2.message, 'Log2' AS source
			FROM log2 l2 
		) log
	SORT BY log.ymd ASC;
``

## 5 视图（hive编程指南第7章 视图）




3.3 一些用法

+ SELECT a.* FROM invites a WHERE a.ds='2008-08-15';
+ FROM pokes t1 JOIN invites t2 ON (t1.bar = t2.bar) INSERT OVERWRITE TABLE events SELECT t1.bar, t1.foo, t2.foo;
+ CREATE TABLE u_data (
  userid INT,
  movieid INT,
  rating INT,
  unixtime STRING)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
STORED AS TEXTFILE;

+ LOAD DATA LOCAL INPATH '/Users/keything/Downloads/ml-100k/u.data' OVERWRITE INTO TABLE u_data;
+ `SELECT COUNT(*) FROM u_data;`




一个简单的例子：

创建表：

```
CREATE TABLE u_data (
  userid INT,
  movieid INT,
  rating INT,
  unixtime STRING)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t'
STORED AS TEXTFILE;
```

获取数据集

```
wget http://files.grouplens.org/datasets/movielens/ml-100k.zip
unzip ml-100k.zip

```

加载数据集

```
LOAD DATA LOCAL INPATH '<path>/u.data' OVERWRITE INTO TABLE u_data;
```

创建weekday_mapper.py

```
import sys
import datetime

for line in sys.stdin:
  line = line.strip()
  userid, movieid, rating, unixtime = line.split('\t')
  weekday = datetime.datetime.fromtimestamp(float(unixtime)).isoweekday()
  print '\t'.join([userid, movieid, rating, str(weekday)])

```
对于这个脚本是否正确，可以通过自己创建文件来验证。

创建mapper脚本：

```
CREATE TABLE u_data_new (
  userid INT,
  movieid INT,
  rating INT,
  weekday INT)
ROW FORMAT DELIMITED
FIELDS TERMINATED BY '\t';

add FILE weekday_mapper.py;

INSERT OVERWRITE TABLE u_data_new
SELECT
  TRANSFORM (userid, movieid, rating, unixtime)
  USING 'python weekday_mapper.py'
  AS (userid, movieid, rating, weekday)
FROM u_data;

SELECT weekday, COUNT(*)
FROM u_data_new
GROUP BY weekday;
```

最后得到

```
1	12254
2	13579
3	14430
4	15114
5	14743
6	18229
7	11651
```


