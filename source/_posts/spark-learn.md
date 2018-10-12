---
title: spark_learn
date: 2017-03-31 23:07:37
tags: spark
---
## RDD介绍

在Spark集群背后，有一个非常重要的分布式数据架构，即弹性分布式数据集（resilient distributed dataset, RDD)，它是逻辑集中的实体，在集群中的多台机器上进行数据分区。通过多台机器上不同RDD分区的控制，就能够减少机器之间的数据重排（data shuffling）。Spark提供了`partitionBy`运算符，能够通过集群中多台机器对原始RDD进行数据再分配来创建一个新的RDD。

RDD是Spark的核心数据结构，通过RDD的依赖关系形成Spark的调度顺序。通过对RDD的操作形成整个Spark程序。

###一、 RDD的两种创建方式

#### 1. 代码中并行化一个已经存在的集合(Parallelized Collections)
    
    从已经存在集合中创建
    
        data = [1, 2, 3, 4, 5]
        distData = sc.parallelize(data)


    集合并行化（parallel collections）的一个重要参数是：集合切分的分区数。Spark会为集群中每个分区运行一个任务。一般是集群中每个CPU有2-4个分区。通常，Spark会根据集群情况自动设置分区的数量。当然，你也可以手工输入一个数量，作为parallelize的第二个参数（如 `sc.parallelize(data, 10)`)
    一旦创建以后，数据就可以并行处理了。

#### 2. 从外部存储系统中创建。如共享文件系统，HDFS，HBASE，HIVE或其他提供Hadoop输入格式的数据来源
    
可以使用SparkContext's textFile方法创建文本文件RDDs。这个方法使用文件的URL（或者机器的本地文件, hdfs:://, s3n://等等），并作为行的集合进行读取。
从Hadoop获取的方式：
    
    sc.textFile("your_hdfs_path")
        
从Hive中获取：
    
        import org.apache.spark.sql.SQLContext
        val hc = new org.apache.spark.sql.hive.HiveContext(sc)
        val df = hc.sql("show databases")
        df.show
  

####使用spark读取文件的注意事项：
如果使用本地文件系统，If using a path on the local filesystem, the file must also be accessible at the same path on worker nodes. Either copy the file to all workers or use a network-mounted shared file system.

所有spark的基于文件的输入方法，包含`textFile`，支持目录、压缩文件、wildcards等。例如，可以使用`textFile("/my/directory")`, `textFile("/my/directory/*.txt")`, `textFile("/my/directory/*.gz")`.

`textFile`方法可以有第二个参数用于控制分区的个数。默认spark会为每一个文件block创建一个分区（hdfs中默认block是64MB），但是你可以传递更大的分区数。注意，分区数不能少于block数量。

除了`textFile`， spark的python api还支持许多其他格式：

+ `SparkContext.wholeTextFiles`,
+  `RDD.saveAsPickleFile`, `SparkContext.pickleFil`
+  SequenceFile and Hadoop Input/Output Formats


###二、 RDD的操作
RDDs支持两种类型的操作：transformations（从已经存在的Rdd创建一个新的数据集合）和 actions（对数据集合运算后返回一个值）。例如：`map`是一个传入每个数据集合的元素并返回一个新的RDD的transformations。另一方面，`reduce`是一个action：使用一个函数对RDD中所有元素进行聚合，并返回一个最后结果。

在Spark中所有的transformations都是惰性的，并不会马上计算。它只是记录了在base dataset上应用了哪些transformations。只有在有actions的时候才会真的计算。这样的设计使得spark的运行更加高效。

默认，每次进行actions的时候，已经转换过的RDD还需要重新计算。当然，你可以使用`persist(or cache)`方法进行持久化。


1. 分类



    + Transformation和Action两个维度
    + 在Transformation维度会细分为：Value数据类型和Key-value对数据类型。Value类型封装在RDD类中直接使用，Key-Value对数据类型封装在PairRDDFunctions类中，用户需要引入`import org.apache.spark.SparkContext._`才能够使用。

    
### 三、RDD的Value型Transformation操作详细说明

#### 输入分区与输出分区是一对一型
1. map
    
    将函数应用于Rdd的每个元素，并返回结果作为一个新的RDD。
    Applies a transformation function on each item of the RDD and returns the result as a new RDD.
    
    
    举例：
    
    
        >>> data = ["hello", "world12", "nihao"]
        >>> rdd = sc.parallelize(data)
        >>> maprdd = rdd.map(lambda x: len(x))
        >>> maprdd.collect()
            [5, 7, 5]
        >>> total=maprdd.reduce(lambda a,b:a+b)
        
        >>> print total
            17
   实例2：
    
        >>> m = sc.parallelize(["dog", "tiger", "lion"])
        >>> n = m.map(lambda x: (x, 1))
        >>> n.collect()
        [('dog', 1), ('tiger', 1), ('lion', 1)]

2. flatMap

    与map类似，但是允许在映射中释放多个元素。
    
    举例：
    
        >>> data = ["hello", "world12", "nihao"]
        >>> rdd.map(lambda x: (x,x)).collect()
            [('hello', 'hello'), ('world12', 'world12'), ('nihao', 'nihao')]
        >>> rdd.flatMap(lambda x: (x,x)).collect()
            ['hello', 'hello', 'world12', 'world12', 'nihao', 'nihao']

3. mapPartitions
4. glom

#### 输入分区与输出分区多对一型

1. union
    
    执行标准集合操作：A union B。并不进行去重操作，保存所有元素。如果想去重，可以使用distince()。

        >>> datamm = sc.parallelize([1,2,3,4,5]).union(sc.parallelize([4,5,6,7,8]))
        >>> datamm.collect()
            [1, 2, 3, 4, 5, 4, 5, 6, 7, 8]
    
2. cartesian

    对两个RDD内所有元素进行笛卡尔积操作。
    Computes the cartesian product between two RDDs 
    

#### 输入分区与输出分区多对多

1. groupBy：将元素按照函数生成相应的key，数据就转换为key-value格式，之后将key相同的元素分为一组。

        >>> m = sc.parallelize(["tiger", "mmmmm", "nnn", "hhh"])
        >>> n = m.groupBy(len)
        >>> n.collect()
        [(5, <pyspark.resultiterable.ResultIterable object at 0x106acbed0>), (3, <pyspark.resultiterable.ResultIterable object at 0x106acbb50>)]
        >>> def tra(k_records):
        ...     k = k_records[0]
        ...     print "k=",k
        ...     records = k_records[1]
        ...     for record in records:
        ...         print "record:", record
        ...
        >>> o = n.map(tra)
        >>> o.collect()
        k= 3
        record: nnn
        record: hhh
        k= 5
        record: tiger
        record: mmmmm
        [None, None]

#### 输出分区为输入分区子类型

1. filter: 对元素进行过滤，对每个元素应用f函数，返回值为true的元素保留；反之，过滤。

        >>> def tigerFilter(x):
        ...     if x == "tiger":
        ...             return 0
        ...     else:
        ...             return 1
        ...
        >>> m = sc.parallelize(["tiger", "mmmmm", "nnn", "hhh"])
        >>> m.filter(tigerFilter).collect()
        ['mmmmm', 'nnn', 'hhh']
2. distinct：将RDD中的元素进行去重操作。
3. subtract:相当于进行集合的差操作。         
4. sample
5. cache

### Key-Value型Transformation算子

1. mapValues:针对(key, value)中的value进行操作，而不对key进行处理
2. 对单个RDD或两个RDD聚集

    + combineByKey
    + reduceByKey
    + partitionBy
3. 连接

    + join
    + leftOutJoin

## Action算子

1. 无输出 foreach
2. HDFS
    
    + saveAsTextFile
    + saveAsObjectFile

3. Scala集合和数据类型

    + collect
    + collectAsMap
    + lookup
    + reduceByKeyLocally
    + count
    + top
    + reduce
    + fold
    + aggregate
    +                                                                             

    
#### working with key-value Pairs

有一些特别的操作只能应用于k/v对。最常见的是分布式重排操作（distributed shuffle)，如通过key进行grouping, aggregating。





## Spark学习

## spark函数学习

1. spark函数讲解 aggregate

    + https://www.iteblog.com/archives/1268
    + http://lib.csdn.net/article/scala/34642==

2. 所有spark函数讲解

    + http://homepage.cs.latrobe.edu.au/zhe/ZhenHeSparkRDDAPIExamples.html

    
### Spark数据输入
1. 从HDFS读取日志文件

    var file = sc.textFile("hdfs://xxx")
    


### Spark中数据处理

1. 过滤

    var errors = file.filter(line => line.contains("error")
    



aggregate函数

[理解Spark RDD中的aggregate函数](http://lib.csdn.net/article/scala/34642)
[learn](http://homepage.cs.latrobe.edu.au/zhe/ZhenHeSparkRDDAPIExamples.html)

groupBy函数

combineByKey函数
countByKey函数
reduce
reduceByKey
sortBy
sortByKey



S
