---
title:  "Mac 安装Hbase环境"
date:   2016-01-03 13:20:01 +0800
tags: mac
---

## 1. 安装
1. 安装brew
2. 安装Hbase

       brew install hbase 

## 2. 说明
Hbase运行模式分为单机、伪分布式、全分布式三种。我尝试使用单机，但由于zookeeper的原因没有搭建成功，目前使用伪分布式的方式运行。
在Hbase中http://hbase.apache.org/book.html#zookeeper 介绍了Hbase和Zookeeper的关系。

## 3. 配置修改
1. 修改hbase-env.sh 
    将
        #export HBASE_MANAGES_ZK=true
    修改为 
        export HBASE_MANAGES_ZK=false

2. 修改hbase-site.xml
其中hbase.rootdir的value必须跟hadoop中core-site.xml中fs.default.name相同。如果不同，则会引起ERROR: ``Can't get master address from ZooKeeper; znode data == null``

        <configuration>
          <property>
           <name>hbase.rootdir</name>
           <value>hdfs://localhost:9000/hbase</value>
          </property>
          <property>
           <name>hbase.cluster.distributed</name>
           <value>true</value>
          </property>
        </configuration>

## 3. 启动并运行例子
1. 依次执行下面三个命令
start-hbase.sh
可以通过jps查看是否正确运行

    13489 Jps
    9633 QuorumPeerMain
    13315 HMaster
    6597 DataNode
    6838 ResourceManager
    6935 NodeManager
    6505 NameNode
    13419 HRegionServer
    6715 SecondaryNameNode

如果没有QuorumPeerMain 要去看hbase-env.sh中是否设置正确
2. 运行一个例子
创建一个名为 test 的表，这个表只有一个 列族 为 cf。可以列出所有的表来检查创建情况，然后插入些值。

    hbase(main):003:0> create 'test', 'cf'
    0 row(s) in 1.2200 seconds
    hbase(main):003:0> list 'test'
    test
    1 row(s) in 0.0550 seconds
    hbase(main):004:0> put 'test', 'row1', 'cf:a', 'value1'
    0 row(s) in 0.0560 seconds
    hbase(main):005:0> put 'test', 'row2', 'cf:b', 'value2'
    0 row(s) in 0.0370 seconds
    hbase(main):006:0> put 'test', 'row3', 'cf:c', 'value3'
    0 row(s) in 0.0450 seconds
以上我们分别插入了3行。第一个行key为row1, 列为 cf:a， 值是 value1。HBase中的列是由 列族前缀和列的名字组成的，以冒号间隔。例如这一行的列名就是a.

检查插入情况.

Scan这个表，操作如下

    hbase(main):007:0> scan 'test'
    ROW        COLUMN+CELL
    row1       column=cf:a, timestamp=1288380727188, value=value1
    row2       column=cf:b, timestamp=1288380738440, value=value2
    row3       column=cf:c, timestamp=1288380747365, value=value3
    3 row(s) in 0.0590 seconds
Get一行，操作如下

    hbase(main):008:0> get 'test', 'row1'
    COLUMN      CELL
    cf:a        timestamp=1288380727188, value=value1
    1 row(s) in 0.0400 seconds
disable 再 drop 这张表，可以清除你刚刚的操作

    hbase(main):012:0> disable 'test'
    0 row(s) in 1.0930 seconds
    hbase(main):013:0> drop 'test'
    0 row(s) in 0.0770 seconds 
关闭shell

hbase(main):014:0> exit

参考文章
1. http://abloz.com/hbase/book.html#quickstart
2. http://www.jianshu.com/p/510e1d599123
