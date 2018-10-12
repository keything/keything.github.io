---
title: Kafka-学习笔记
date: 2015-11-28 17:02:01 +0800
tags: 基础知识
---

## 1. Kafka基本概念
1. 话题topic: 特定类型的消息流，话题是消息的分类名。物理上不同topic的消息分开存储，逻辑上一个Topic消息可以保存在一个或多个broker上，但用户只需要指定指定消息的topic即可生产或消费数据而不必关系数据存于何处
2. 生产者producer：能够发布消息到话题的任何对象
3. kafka集群：已经发布的消息保存的服务器
4. 代理broker：kafka集群中的服务器
5. 分片partition：Partition：topic物理上的分组，一个topic可以分为多个partition，每个partition是一个有序的队列
6. 消费组 consumer group：每个consumer属于一个特定的Consumer Group，可以为每个Consumer指定group name，如果不指定，则属于默认的group。每一个consumer实例都属于一个consumer group，每一条消息只会被同一个consumer group里的一个consumer实例消费。每条消息可以被多个consumer group消费
7. segment：partition物理上由多个segment组成
8. offset：每个partition都由一系列有序的、不可变的消息组成，这些消息被连续的追加到partition中。partition中的每个消息都有一个连续的序列号叫做offset,用于partition唯一标识一条消息


## 2. Topic，Partition，Segment，Offset
### topic和Partition关系
1. topic在逻辑上可以认为是一个queue，每条消息都必须指定它的topic
2. 为了使Kafka吞吐率可以线性提高，物理上把Topic分成一个或多个Partition，每个Partition在物理上对应一个文件夹，该文件夹下存储这个Partition的所有消息和索引文件。

### Partition的存储分布
1. 参考http://tech.meituan.com/kafka-fs-design-theory.html 
2. 目录：在config/server.properties的log.dirs中。同一个topic下有多个不同partition，每个partition是一个目录，目录命名是topic名称+有序序号。
3. Partition中文件存储方式
	1. 每个partition相当于一个巨型文件被平均分配到多个大小相同的segment段数据文件中。但每个段segmentfile的消息数量不一定相等。
	2. 每个partition只需要支持顺序读写就行了，segment文件生命周期由服务端配置参数决定。
4. Partition中segment文件存储结构
	1. segment file组成：由两大部分组成，分别为index file和data file。这两个文件一一对应，后缀.index和.log，分别表示segment索引文件、数据文件
	2. segment命名规则:partion全局的第一个segment从0开始，后续每个segment文件名为上一个segment文件最后一条消息的offset值。这种命名很重要，有助于通过offset查找message。
	3. 一个片段segment的index和data file的对应关系，索引中存储大量元数据，数据文件存储大量消息，索引文件中元数据指向对应数据文件中message的物理偏移地址。

5. Segment数据文件组成
	1.  segment data file由许多message组成
6. 如何在Partition中通过offset查找message
	1. 通过二分查找offset，找到该offset所在的segment文件
	2. 在所找到的segment文件中，顺序到在index文件中的元数据物理位置和log文件中物理偏移地址。
	3. segment index file采取稀疏索引存储方式，它减少索引文件大小，通过mmap可以直接内存操作，稀疏索引为数据文件的每个对应message设置一个元数据指针,它比稠密索引节省了更多的存储空间，但查找起来需要消耗更多的时间
7. Partition的好处
	1. 实现并发：传统的队列，为了保证数据的有序性，只能锁定一个队列。而kafka由于采用Partition的方式，则可以实现并发。
	2. 实现并发的设计：将一个topic中的所有分片partition分配给一个consumer group中的消费者们。这样的话，每个分片就可以被consumer group中的准确的某个消费者消费。
	3. 注意：确保一个consumer group中消费者的个数不要多于分片的个数
	4. kafka的有序性只能保证某个分片内的有序，不能保证分片之间的有效性。如果要想保证所有数据的有序，那么可以只有一个分片。

摘录来自: kafka.apache.org. “Apache Kafka”。 iBooks. 

## 3. 生产者 Producer
### 1. 负载均衡
1. 由producer客户端决定消息被路由到哪个partition。producer将会和Topic下所有partition leader保持socket连接;消息由producer直接通过socket发送到broker,中间不会经过任何"路由层".路由策略比如random, key-hash。

### 2. 异步发送
将多条消息暂且在客户端buffer起来，并将他们批量的发送到broker，小数据IO太多，会拖慢整体的网络延迟，批量延迟发送事实上提升了网络效率。不过这也有一定的隐患，比如说当producer失效时，那些尚未发送的消息将会丢失

### 3. 消息传送保证机制
有几种方式：
	1) at most once: 最多一次,这个和JMS中"非持久化"消息类似.发送一次,无论成败,将不会重发.
	2) at least once: 消息至少发送一次,如果消息未能接受成功,可能会重发,直到接收成功.
	3) exactly once: 消息只会发送一次
	
## 4. 消费者 Consumer
1. 通常来讲，消息有两种模式：a.队列 b.发布订阅。队列的话，一条消息只能被一个消费者消费。发布订阅的话，消息被所有消费者消费。如果所有consumer实例属于一个consumer group，则实现队列。如果每个consumer group只有一个consumer实例，则实现发布订阅。
2. consumer端向broker发送"fetch"请求,并告知其获取消息的offset;此后consumer将会获得一定条数的消息;consumer端也可以重置offset来重新消费消息.
2. Consumer与Consumer Group
	1. 每个consumer实例都属于一个consumer group。每个消息只能被同一个consumer group里的一个consumer实例消费，但不同consumer group可以同时消费同一条消息

## 5. 系统级别
### 1. kafka的broker是无状态的
1. kafka代理是无状态的，意味着消费者必须维护已消费的状态信息。
	1. 从代理broker删除消息很棘手，因为代理不知道消费者是否已经使用了该消息。kafka使用一个简单的基于时间的SLA应用于保留策略。当消息在代理中超过一定时间以后，将会自动删除。
	2. 好处是：消费者可以故意倒回到老的偏移量再次消费数据。
	
### 2. broker数据删除
1. 不管消息被消费与否，kafka集群会保留所有消息。由于磁盘限制，kafka提供两种策略删除旧数据：一种基于时间，一种基于partition文件大小。可以通过config/server.properties配置

### 3. ZooKeeper与Kafka
1. zookeeper的关注问题：
	1. 所有分布式系统的一个常见问题是：如何在任一时间点确定哪些服务器活着并且在工作中
	2. 面对网络失败，带宽限制，可变延迟连接，安全问题，跨多个数据中心时可能发生错误时，如何可靠做这些事情

### 4. Kafka设计目标
1. 以时间复杂度O(1)的方式提供消息持久化能力
2. 高吞吐率
3. 支持Kafka Server间的消息分区，及分布式消费，同时保证每个Partition内的消息顺序传输
4. 同时保持离线数据处理和实时数据处理。
5. 支持在线水平扩展

### 5. 为何使用消息系统
1. 解耦
2. 扩展性
3. 灵活性&峰值处理能力
4. 顺序保证
5. 异步通信
6. demo


参考文章：
1.  http://tech.meituan.com/kafka-fs-design-theory.html
2. http://my.oschina.net/frankwu/blog/305010
3. http://dataunion.org/9307.html?utm_source=tuicool&utm_medium=referral
