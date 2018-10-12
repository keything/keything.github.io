---
title: hive的TRANSFORM操作
date: 2018-09-23 23:03:39
tags:
---

## Transform/Map-Reduce Syntax

hive语言中内置的特性是支持用户自定义mappers/redulers的。 用户可以使用`TRANSFROM ` 子句来内嵌mapper/reduer脚本的。 

By default, columns will be transformed to STRING and delimited by TAB before feeding to the user script; similarly, all NULL values will be converted to the literal string \N in order to differentiate NULL values from empty strings. The standard output of the user script will be treated as TAB-separated STRING columns, any cell containing only \N will be re-interpreted as a NULL, and then the resulting STRING column will be cast to the data type specified in the table declaration in the usual way. User scripts can output debug information to standard error which will be shown on the task detail page on hadoop. These defaults can be overridden with ROW FORMAT ....


注意：

Formally, MAP ... and REDUCE ... are syntactic transformations of SELECT TRANSFORM ( ... ). In other words, they serve as comments or notes to the reader of the query. BEWARE: Use of these keywords may be dangerous as (e.g.) typing "REDUCE" does not force a reduce phase to occur and typing "MAP" does not force a new map phase!


```
clusterBy: CLUSTER BY colName (',' colName)*
distributeBy: DISTRIBUTE BY colName (',' colName)*
sortBy: SORT BY colName (ASC | DESC)? (',' colName (ASC | DESC)?)*
 
rowFormat
  : ROW FORMAT
    (DELIMITED [FIELDS TERMINATED BY char]
               [COLLECTION ITEMS TERMINATED BY char]
               [MAP KEYS TERMINATED BY char]
               [ESCAPED BY char]
               [LINES SEPARATED BY char]
     |
     SERDE serde_name [WITH SERDEPROPERTIES
                            property_name=property_value,
                            property_name=property_value, ...])
 
outRowFormat : rowFormat
inRowFormat : rowFormat
outRecordReader : RECORDREADER className
 
query:
  FROM (
    FROM src
    MAP expression (',' expression)*
    (inRowFormat)?
    USING 'my_map_script'
    ( AS colName (',' colName)* )?
    (outRowFormat)? (outRecordReader)?
    ( clusterBy? | distributeBy? sortBy? ) src_alias
  )
  REDUCE expression (',' expression)*
    (inRowFormat)?
    USING 'my_reduce_script'
    ( AS colName (',' colName)* )?
    (outRowFormat)? (outRecordReader)?
 
  FROM (
    FROM src
    SELECT TRANSFORM '(' expression (',' expression)* ')'
    (inRowFormat)?
    USING 'my_map_script'
    ( AS colName (',' colName)* )?
    (outRowFormat)? (outRecordReader)?
    ( clusterBy? | distributeBy? sortBy? ) src_alias
  )
  SELECT TRANSFORM '(' expression (',' expression)* ')'
    (inRowFormat)?
    USING 'my_reduce_script'
    ( AS colName (',' colName)* )?
    (outRowFormat)? (outRecordReader)?
```

转化的例子1：

``
FROM (
  FROM pv_users
  MAP pv_users.userid, pv_users.date
  USING 'map_script'
  AS dt, uid
  CLUSTER BY dt) map_output
INSERT OVERWRITE TABLE pv_users_reduced
  REDUCE map_output.dt, map_output.uid
  USING 'reduce_script'
  AS date, count;
FROM (
  FROM pv_users
  SELECT TRANSFORM(pv_users.userid, pv_users.date)
  USING 'map_script'
  AS dt, uid
  CLUSTER BY dt) map_output
INSERT OVERWRITE TABLE pv_users_reduced
  SELECT TRANSFORM(map_output.dt, map_output.uid)
  USING 'reduce_script'
  AS date, count;
``

转化的例子2：

``
FROM (
  FROM src
  SELECT TRANSFORM(src.key, src.value) ROW FORMAT SERDE 'org.apache.hadoop.hive.contrib.serde2.TypedBytesSerDe'
  USING '/bin/cat'
  AS (tkey, tvalue) ROW FORMAT SERDE 'org.apache.hadoop.hive.contrib.serde2.TypedBytesSerDe'
  RECORDREADER 'org.apache.hadoop.hive.contrib.util.typedbytes.TypedBytesRecordReader'
) tmap
INSERT OVERWRITE TABLE dest1 SELECT tkey, tvalue
``

将TRANSFORM的输出打出来

脚本的输出默认是string，如果想进行类型转换的需要进行如下操作。

``
SELECT TRANSFORM(stuff)
USING 'script'
AS thing1, thing2
``

类型转化
``
SELECT TRANSFORM(stuff)
USING 'script'
AS (thing1 INT, thing2 INT)
``
