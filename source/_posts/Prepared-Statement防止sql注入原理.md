---
title: 为什么使用Prepared Statement?
date: 2016-08-18 17:54:54
tags: mysql
---


1. 查询执行的基础
MySQL发送一个请求的时候，MySQL到底做了什么：

    a. 客户端发送一条查询给服务器
    b. 服务器先检查``查询缓存``，如果命中了缓存，则like返回存储在缓存中的结果。否则进入下一阶段
    c. 服务器端进行SQL解析，预处理，再由优化器生成对应的执行计划
    d. MySQL根据优化器生成的执行计划，调用存储引擎的API来执行查询
    e. 将结果返回给客户端。

    即：SQL执行过程包括以下阶段：查询缓存是否命中->词法解析->语法分析->语义分析->查询优化->执行。

2. 硬解析

    ``词法分析``->``语法分析``这两个阶段我们称之为硬解析。词法分析识别SQL中每个词，语法分析解析SQL是否符合SQL语法（如关键字顺序是否正确），并得到一颗语法树。对于只是参数不同，其他均相同的SQL，他们执行时间不同但是硬解析时间是相同的。

3. 查询优化器
    现在语法树被认为是合法了，并且由优化器将其转化成执行计划。一条查询可以有很多种执行方式，最后返回相同的结果。
优化器的作用：就是找到这其中最好的执行计划

4. Prepare的目的
    Prepare的出现就是为了优化硬解析的问题。Prepare在服务器端执行过程如下：

    a. Prepare接受客户端带``?``的SQL，硬解析得到语法树stmt->lex， 缓存在线程所在的preparestatement cache中。此cache是一个HASH MAP. Key为stmt->id. 然后返回客户端stmt->id等信息。
    b. Execute 接收客户端stmt->id和参数等信息。注意这里客户端不需要再发sql过来。服务器根据stmt->id在preparestatement cache中查找得到硬解析后的stmt, 并设置参数，就可以继续后面的优化和执行了

5. Prepared Statements的好处：
    a. 安全
    Prepared Statements通过sql逻辑与数据分离来增加安全，sql逻辑与数据的分离能够防止普通类型的SQL注入攻击（sql injection attck)。
    b. 性能
    prepare经过语法解析器和预处理生成了解析树。通过prepare方式，当执行多次时，就不会有额外的负担了。

6. Prepared 实例

表结构

    CREATE TABLE `z` (
        `a` int(11) NOT NULL,
        `b` int(11) DEFAULT NULL,
        PRIMARY KEY (`a`)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8

Prepare

    prepare stmt1 from 'select * from z where a = ?';
    set @a = 20;
    execute stmt1 using @a;





[MySQL prepare原理](http://www.cnblogs.com/justfortaste/p/3920140.html)

