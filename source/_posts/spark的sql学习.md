---
title: spark的sql学习
date: 2018-09-22 21:19:27
tags: spark
---

# Spark SQL, DataFrames and Datasets Guide

##概述：

Spark SQL时候spark中处理结构化数据的模块。 

Spark SQL可以从已经安装好的hive中读取数据，也可以通过命令行 or JDBC/ODBC 读取数据。 


### Datasets and DataFrames:


Dataset 是数据的分布式集合（A Dataset is a distributed collection of data）。Dataset可以从JVM对象中构建，也可以使用转化进行操作（transformations 如map,flatMap,filter 等等)。

DataFrame 是将Dataset组织为命名列(named columns)，在python/R 中等同于关系型数据库中的表(table)。





### DataFrame Operations：

df.printSchema()
df.select("name").show()
df.select(df['name'], df['age'] + 1).show()
df.filter(df['age'] > 21).show()
df.groupBy("age").count().show()


Running SQL Queries Programmatically


在SparkSession中的sql函数会执行sql查询语句，并返回 DataFrame的结果。 
sqlDF = spark.sql("SELECT * FROM people")
sqlDF.show()


### Programmatically Specifying the Schema

官方引导的方式个人感觉有点繁琐。 http://spark.apache.org/docs/latest/sql-programming-guide.html#programmatically-specifying-the-schema

可以使用to.DF()的方式来做。 

```import org.apache.spark.sql.types._
from pyspark.sql import Row

lines =  sc.textFile("file:///usr/local/Cellar/spark/2.0.1/examples/src/main/resources/people.txt")
parts = lines.map(lambda l: l.split(","))
peoples  = parts.map(lambda p: Row(name=p[0], age=p[1]))
peoples.toDF()

// Creates a temporary view using the DataFrame
peopleDF.createOrReplaceTempView("people")

// SQL can be run over a temporary view created using DataFrames
val results = spark.sql("SELECT name FROM people")
```



## Data Sources



Spark SQL 通过DataFrame接口 支持不同类型的数据源。 
将DataFrame注册为一个临时视图（temporary view)，允许你可以通过SQL查询数据。
这部分描述的是加载和保存数据的通用方法，并会介绍built-in data sources的特殊选项。

### Generic Load/Save Functions

```
df = spark.read.load("examples/src/main/resources/users.parquet")
df.select("name", "favorite_color").write.save("namesAndFavColors.parquet")
```


### 手工指定特殊参数
```
df = spark.read.load("examples/src/main/resources/people.json", format="json")
df.select("name", "age").write.save("namesAndAges.parquet", format="parquet")
```



## 例子


```

#-*- coding:utf-8 -*-

import sys
reload(sys)
sys.setdefaultencoding('utf8')
import json
import subprocess
import time
import pandas as pd
from pyspark.sql import Row

import math
from datetime import datetime, timedelta
from pyspark import SparkContext, SparkConf
from pyspark import HiveContext
import collections

def hiveSql():
    sql = '''
    select param['datatype'] as datatype
     from xxx between aaa and bbb
    '''
    return sql



def parse_poi(row):
    # 第二列取值：
    # 1 参数错误并且经纬度为0，
    # 2 参数错误并且经纬度不是0
    # 3 query为空
    # 4 其他异常情况
    parse_result = "empty\tempty\tempty"
    json_str = row.json_str
    if type(json_str) != str and type(json_str) != unicode:
        return parse_result
    try:
        res = json.loads(json_str)
    except:
        return parse_result

    if row.searchname != '':
        parse_result = '%s\t%s\t%s'%(row.nnn, 0, row.oooo)
        return parse_result



def parse_mmm(sc):
    hc = HiveContext(sc)

    all_sug_poi = sc.parallelize([])
    sql = hiveSql()
    print sql
    df = hc.sql(sql)
    mmmm = df.rdd.map(parse_poi).map(lambda x:x.split("\t"))

    ooooo = mmmm.map(lambda p: Row(searchname=p[0], stattype=p[1], datatype=p[2]))

    dudtf = ooooo.toDF()
    dudtf.printSchema()
    dudtf.groupBy('stattype').count().show()
    dudtf.groupBy('stattype', 'datatype').count().show()


if __name__ == "__main__":
    conf = SparkConf()
    conf.setAppName('mmmmxxxx')
    sc = SparkContext(conf = conf)
    parse_mmm(sc)
```

在这个例子中，

+ 首先查询hive，要使用HiveContext去查询。
+ 再次，由于数据存在一定的污染，因此在进行json.loads之前必须对row.json_str进行类型断言；并且当json.loads抛出异常时，进行捕获。
+ 在执行mmmm.map时使用map，而不是flatMap。
+ 


##参考文章：

+ http://spark.apache.org/docs/latest/sql-programming-guide.html



