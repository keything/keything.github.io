---
title: mac安装hive
date: 2016-11-27 09:42:54
tags: mac
---
## Mac安装hive
### 安装
1. brew install hive

### 配置修改
2. 设置`HIVE_HOME=/usr/local/Cellar/hive/2.1.0/libexec`
4. `cp $HIVE_HOME/conf/hive-default.xml.template $HIVE_HOME/conf/hive-default.xml`
5. hive-site.xml 文件中包含两部分，第一部分mysql是hive的metastore，存在mysql中。第二部分是metastore的目录。关于hive的metastore保存可以参考 官方wiki:[Hive Metastore](http://wiki.apache.org/hadoop/Hive/AdminManual/MetastoreAdmin)。其中配置中的用户名和密码都是hive

         <?xml version="1.0" encoding="UTF-8" standalone="no"?>
        
        <configuration>
        
            <property>
                <name>javax.jdo.option.ConnectionURL</name>
                <value>jdbc:mysql://127.0.0.1:3306/pdw?useSSL=false</value>
            </property>
        
            <property>
                <name>javax.jdo.option.ConnectionDriverName</name>
                <value>com.mysql.jdbc.Driver</value>
            </property>
        
            <property>
                <name>javax.jdo.option.ConnectionUserName</name>
                <value>hive</value>
            </property>
        
            <property>
                <name>javax.jdo.option.ConnectionPassword</name>
                <value>hive</value>
            </property>
        
            <property>
                <name>hive.metastore.warehouse.dir</name>
                <value>/user/hive/warehouse</value>
            </property>
        
        </configuration>
    
4. 因为我们使用mysql存储meta信息，因此需要下载一个连接的jar包并将其放在`$HIVE_HOME/lib`中 [jar包下载](http://dev.mysql.com/downloads/connector/j/)

    `cp ~/Downloads/mysql-connector-java-5.1.40/mysql-connector-java-5.1.40-bin.jar $HIVE_HOME/lib`

## 配置spark支持hive

1. Spark支持hive需要三个jar包，如果有的话则已经支持；如果没有的话则需要编译。在通过brew install spark 安装的spark2.0.1默认已经支持

    ➜  ~ cd $SPARK_HOME/jars
    
    ➜  ls -l d*
        
        -rw-r--r--@ 1 keything  staff   339666 Sep 29 08:03 datanucleus-api-jdo-3.2.6.jar
        -rw-r--r--@ 1 keything  staff  1890075 Sep 29 08:03 datanucleus-core-3.2.10.jar
        -rw-r--r--@ 1 keything  staff  1809447 Sep 29 08:03 datanucleus-rdbms-3.2.9.jar

2. 配置文件的拷贝

    + 复制hive配置文件
    
        `cp $HIVE_HOME/conf/hive-site.xml $SPARK_HOME/conf/hive-site.xml`
    + 复制hdfs配置文件
        
        `cp $HADOOP_HOME/libexec/etc/hadoop/hdfs-site.xml $SPARK_HOME/conf/hdfs-site.xml`
        `cp $HADOOP_HOME/libexec/etc/hadoop/core-site.xml $SPARK_HOME/conf/core-site.xml`

3. 启动

    `./bin/spark-shell --jars $HIVE_HOME/lib/mysql-connector-java-5.1.xx-bin.jar`

4. 测试

    
        import org.apache.spark.sql.SQLContext
        val hc = new org.apache.spark.sql.hive.HiveContext(sc)
        val df = hc.sql("show databases")
        df.show


注意：在执行`val df = hc.sql("show databases")`中会报出`16/11/27 09:09:35 ERROR metastore.RetryingHMSHandler: AlreadyExistsException(message:Database default already exists)
	at org.apache.hadoop.hive.metastore.HiveMetaStore$HMSHandler.create_database(HiveMetaStore.java:891)`的错误，可以忽略掉，如何不让这个错误出来，目前未知。
	


注意：在hadoop 2.7.3 下 启动hive 3.1 及 hive 2.3 都会报错：
hive 命令启动 
> show databases;
会有下面的报错
```FAILED: HiveException java.lang.RuntimeException: Unable to instantiate org.apache.hadoop.hive.ql.metadata.SessionHiveMetaStoreClient```

换成hive 1.2.2以后没有问题。 
hive下载地址:http://mirror.bit.edu.cn/apache/hive/
	
