---
title: hadoop权威指南第四版学习笔记
date: 2018-09-23 11:30:21
tags:
---

# Hadoop权威指南第四版-学习笔记



## 第二章 关于MapReduce

MapReduce是一种可用于数据处理的编程模型。 



Hadoop将MapReduce的输入数据划分成等长的小数据块，称为输入分片（input split) 或简称“分片”。hadoop为每个分片构建一个map任务，并由该任务来运行用户自定义的map函数，从而处理分片中的每条记录。 

一个合理的分片大小趋向于HDFS的一个块的大小，默认是128MB。

#### map任务将其输出写入本地硬盘，而非HDFS。这是为什么呢 ？

因为map的输出是中间结果：该中间结果由reduce任务处理后才产生最终输出结果，而且一旦作业完成，map的输出结果就可以删除。 因此，如果把map输出存储在HDFS中并实现备份，难免有些小题大做。 

如果运行map任务的节点在map中间结果传送给reduce任务之前失败，hadoop将在另一个节点重新运行这个map任务以再次构建map中间结果。


reduce任务并不具备数据本地化的优势，单个reduce任务的输入通常来自于所有mapper的输出。

reduce任务的数量并不是由输入数据的大小决定，相反是独立指定的。
如果有多个reduce任务，每个map任务就会针对输出进行分区(partition)，即为每个reduce任务建一个分区，每个分区有许多键（及其对应的值），但每个键对应的key-value 都在同一个分区中。 用户可以使用自定义的分区函数控制分区，但通常使用默认的partitioner 通过哈希函数来区分，很高效。

一般情况下，多个reduce任务的数据流如图2-4所示，该图清楚表明了为什么map任务和reduce任务之间的数据流成为shuffle（混洗），因为每个reduce任务的输入都来自许多map任务。shuffle一般比图中所示的更复杂，而且调整混洗参数对作业总执行时间的影响非常大。 


## 第三章 Hadoop分布式文件系统

### 3.2 HDFS概念

3.2.1 数据块

HDFS同样也有块block的概念，默认是128MB。

3.2.2 namenode和datanode

HDFS集群有两类节点，以 管理节点-工作节点 模式运行，即一个namenode（管理节点）和多个datanode（工作节点）。
namenode 管理文件系统的命名空间。它维护者文件系统树及整颗树内所有的文件和目录。这些信息以两个文件形式永久保存在本地磁盘上：命名空间镜像文件和编辑日志文件。 

namenode也记录着每个文件中各个块所在的数据节点信息，但它并不用就保存块的位置信息，因为这些信息会在系统启动时根据数据节点信息重建。 

datanode是文件系统的工作节点。他们根据需要存储并检索数据块（受客户端或namenode调度），并且定期向namenode发送所存储的块的列表。

3.2.3 块缓存
3.2.4 联邦HDFS
3.2.5 HDFS的高可用性

### 3.3 命令行接口
### 3.4 Hadoop 文件系统
### 3.5 Java接口
### 3.6 数据流


3.6.1 剖析文件读取
3.6.2 剖析文件写入
3.6.3 一致性模型




## 第4章  关于YARN

Apache YARN(Yet Another Resource Negotiator的缩写)是Hadoop的集群资源管理系统。

## 第5章 Hadoop的I/O 操作

5.1 数据完整性
5.2 压缩

5.2.1 codec
5.2.2 压缩和输入分片


5.4 基于文件的数据结构



## 第6章 MapReduce应用开发

MapReduce编程遵循一个特定的流程。首先写map函数和reduce函数，最好使用单元测试来确保函数的运行符合预期。 然后写一个驱动程序来运行作业。


## 第7章 MapReduce的工作机制


### 7.3 shuffle和排序

MapReduce确保每个reducer的输入都是按键排序的。

系统执行排序、将map输出作为输入传给reducer的过程称为shuffle。

7.3.1 map端

map函数开始产生输出时，并不是简单地将它写到磁盘。这个过程更复杂，利用缓冲的方式写到内存并出于效率的考虑进行预排序。
每个map任务都有一个环形内存缓冲区用于存储任务输出。在默认情况下，缓冲区大小是100mb。一旦缓冲内容达到阈值，一个后台线程便开始把内容溢出到splill到磁盘。

在溢出写到磁盘过程中，map输出继续写到缓存区，但如果在此期间缓冲区被填满，map会被阻塞直到写磁盘过程完成。 

在写磁盘之前，线程首先根据数据最终要传的reduer 将数据划分成相应的分区(partition)。在每个分区中，后台线程按键进行内存中排序，如果有一个combiner函数，它就在排序后的输出上运行。运行combiner函数使得map输出结果更紧凑。

每次内存缓冲区达到溢出阈值，就会新建一个溢出文件spill file，因此在map任务写完最后一个输出记录之后，会有几个溢出文件。

{% asset_img 7-4.MapReduce的shuffle和排序.png MapReduce的shuffle和排序 %}


7.3.2 reduce端


### 7.4 任务的执行



## 第8章

8.1 MapReduce的类型

Hadoop的MapReduce中，map函数和reduce函数遵循如下常规格式：

map:(k1, v1) -> list(k2, v2)
combiner: (k2, list(v2)) -> list(k2, v2)
reduce:(k2, list(v2)) -> list(k3, v3)






