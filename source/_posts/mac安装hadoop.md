---
title: mac安装hadoop
date:   2016-01-03 13:02:01 +0800
tags: mac
---

## 1. 安装
1. 安装brew
2. 本地进行无密码授权
3. 安装hadoop

    brew install hadoop
    brew 默认将hadoop安装在/usr/local/Cellar/hadoop/2.7.1目录下。
    配置文件在/usr/local/Cellar/hadoop/2.7.1/libexec/etc/hadoop

## 2. 配置修改

1. 修改hadoop-env.sh

        export HADOOP_OPTS="$HADOOP_OPTS -Djava.net.preferIPv4Stack=true"
        
    修改为
        
        export HADOOP_OPTS="$HADOOP_OPTS -Djava.net.preferIPv4Stack=true -Djava.security.krb5.realm= -Djava.security.krb5.kdc="


2. 修改core-site.xml

    在configuration配置项内加入多个propery。

        <configuration>
            <property>     
                <name>hadoop.tmp.dir</name>     
                <value>/usr/local/Cellar/hadoop/hdfs/tmp</value>     
                <description>A base for other temporary directories.</description>   
            </property>   
            <property>     
                <name>fs.default.name</name>     
                <value>hdfs://localhost:9000</value>
            </property>
        </configuration>

3. 编辑mapred-site.xml. 默认没有，将mapred-site.xml.templete重命名为mapred-site.xml
添加下面的属性

        <configuration>
            <property>
                <name>mapred.job.tracker</name>
                <value>localhost:9010</value>   
            </property> 
        </configuration>

4. 编辑hdfs-site.xml
变量dfs.replication指定了每个HDFS数据库的复制次数。 通常为3, 由于我们只有一台主机和一个伪分布式模式的DataNode，将此值修改为1。

        <configuration>
            <property>
                <name>dfs.replication</name>
                <value>1</value>
            </property>
            <property>
                <name>dfs.datanode.max.xcievers</name>
                <value>4096</value>
            </property>
        </configuration>

## 3. 启动并运行例子
1. 依次执行下面三个命令

            hadoop namenode -format
            start-dfs.sh
            start-yarn.sh

可以通过jps查看是否正确运行

    6597 DataNode
    6838 ResourceManager
    6935 NodeManager
    6505 NameNode
    10427 Main
    6715 SecondaryNameNode
如果发现没有DataNode那么就需要到core-site.xml中找到hadoop.tmp.dir删除该目录下内容再重启。
如果发现NameNode没有，那么你的datanode没有指定namenode的host。默认就去找localhost。然而你namenode不是localhost。你可以去报错那台机器上看9000端口是否有，是不是 namenode，或者9000端口被占用，所以namenode报错！

2. 运行一个例子
```hadoop jar /usr/local/Cellar/hadoop/2.7.1/libexec/share/hadoop/mapreduce/hadoop-mapreduce-examples-2.7.1.jar pi 2 5```
得到的结果
    File Input Format Counters
    Bytes Read=236
    File Output Format Counters
    Bytes Written=97
    Job Finished in 1.636 seconds
    Estimated value of Pi is 3.60000000000000000000
