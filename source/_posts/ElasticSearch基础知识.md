---
title: ElasticSearch基础知识
date: 2017-11-15 10:25:47
tags: elasticsearch
---
1. 创建一个索引

           curl -XPUT  http://10.95.116.44:9202/test11 -d '{
            "settings":{
                "number_of_shards": 5,
                "number_of_replicas": 1
            },
            "mappings" : {
                "prod": {
                    "dynamic": "strict",
                    "properties": {
                        "brand_code": {
                            "type": "integer"
                        },
                        "brand": {
                            "type": "string"
                        }
                    }
                }
            }
        }'
        
2. 查看索引的setting

        curl -XGET http://10.95.116.44:9202/test11/_mapping
    
3. 添加一条数据


4. 查看一条记录


         curl -XGET "http://10.95.116.44:9202/megacorp/employee/1"
         
5. 检索接口

            curl -XGET "http://10.95.116.44:9202/megacorp/employee/_search?pretty"
        {
          "took" : 16,
          "timed_out" : false,
          "_shards" : {
            "total" : 5,
            "successful" : 5,
            "failed" : 0
          },
          "hits" : {
            "total" : 1,
            "max_score" : 1.0,
            "hits" : [ {
              "_index" : "megacorp",
              "_type" : "employee",
              "_id" : "1",
              "_score" : 1.0,
              "_source" : {
                "first_name" : "John",
                "last_name" : "Smith",
                "age" : 25,
                "about" : "I love to go rock climbing",
                "interests" : [ "sports", "music" ]
              }
            } ]
          }
        }
        
5.1 使用查询语句进行

    curl -XGET "http://10.95.116.44:9202/megacorp/employee/_search" -d '
    {
        "query": {
            "match": {
                "first_name": "John"
            }
        }
    }'
    
5.2 更复杂的查询语句


    curl -XGET "http://10.95.116.44:9202/megacorp/employee/_search" -d '
    {
        "query" : {
            "bool": {
                "must": {
                    "match" : {
                        "last_name" : "Smith" 
                    }
                },
                "filter": {
                    "range" : {
                        "age" : { "gt" : 20 } 
                    }
                }
            }
        }
    }
    '
    
5.3 全文搜索

5.4 短语搜索 match_phrase




## 集群内部原理


1. 查看集群健康

        /_cluster/health
        
2. 当master节点都挂了以后，查看机器健康状态就有问题，会报错：

        master_not_discovered_exception
        


## 数据输入与输出

1. 取回一个文档

    GET /website/blog/123?pretty
    
2. 取回一个文档的一部分

    GET /website/blog/123?pretty&_source=title,text
    
3. 删除一个文档

    DELETE /website/blog/123
4. 判断一个文档是否存在 

    HEAD /website/blog/123
    
    如果有status code = 200;
    如果没有 status code = 404
    
5. 处理冲突

    在数据库领域中，有两种方法通常被用来确保并发更新时变更不会丢失：
    
    + 乐观并发控制

        采用compare_and_set的方式， Elasticsearch 使用这个 _version 号来确保变更以正确顺序得到执行。如果旧版本的文档在新版本之后到达，它可以被简单的忽略。冲突时返回的status code 是 409 Conflict HTTP 响应码。
        
    + 悲观并发控制
        采用加锁的方式来做
    
    
6. mapping新增一个字段 其中employee是type，新增一个string类型的名为onSale的字段


        curl -XPOST "http://10.95.177.126:9201/megacorp/employee/_mapping" -d '
        
        {
             "employee": {
                        "properties": {
                         "onSale":{
                            "type":"string" 
                       }
                    }
                }
        }'
        
7. 文档的部分更新

    https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/partial-updates.html
    
## 分布式存储

1. 新建、索引和删除文档

    数据写入时如何是写入成功呢？
    
    + 在客户端收到成功响应时，文档变更已经在主分片和所有副本分片执行完成，变更是安全的。
    + 默认情况下，主分片 需要 规定数量(quorum),或大多数的分片 (其中分片副本可以是主分片或者副本分片)在写入操作时可用。这是为了防止将数据写入到网络分区的‘`背面’'。规定的数量定义公式如下：
        +`int( (primary + number_of_replicas) / 2 ) + 1`
        
2. 取回一个文档

    + 为了读取请求，协调节点在每次请求的时候将选择不同的副本分片来达到负载均衡；通过轮询所有的副本分片。

    
    问题：取回是从主分片取还是从副本取呢？？
    
3. 局部更新文档

    在主分片上更新，同时并行转发给replicas分片。
    
4. 多文档模式

    https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/distrib-multi-doc.html
    
## 搜索

1. 空搜索

 响应的字段取值
 
    |字段取值|含义|
    |---|---|
    |took|告诉我们执行整个搜索请求耗费了多少毫秒
    |请求中的timeout|默认情况下，搜索请求不会超时。 如果低响应时间比完成结果更重要，你可以指定 timeout 为 10 或者 10ms（10毫秒），或者 1s（1秒），在请求超时之前，Elasticsearch 将会返回已经成功从每个分片获取的结果。 timeout 不是停止执行查询，它仅仅是告知正在协调的节点返回到目前为止收集的结果并且关闭连接。在后台，其他的分片可能仍在执行查询即使是结果已经被发送了。使用超时是因为 SLA(服务等级协议)对你是很重要的，而不是因为想去中止长时间运行的查询。|
    |响应中timed_out|表示是否超时|
    |_shards| 告诉我们在查询中参与分片的总数，以及这些分片成功了多少个失败了多少个。正常情况下我们不希望分片失败，但是分片失败是可能发生的。如果我们遭遇到一种灾难级别的故障，在这个故障中丢失了相同分片的原始数据和副本，那么对这个分片将没有可用副本来对搜索请求作出响应。假若这样，Elasticsearch 将报告这个分片是失败的，但是会继续返回剩余分片的结果。|
    |hits|它 包含 total 字段来表示匹配到的文档总数，并且一个 hits 数组包含所查询结果的前十个文档.在 hits 数组中每个结果包含文档的 _index 、 _type 、 _id ，加上 _source 字段。这意味着我们可以直接从返回的搜索结果中使用整个文档。这不像其他的搜索引擎，仅仅返回文档的ID，需要你单独去获取文档。

2. 分页

    参数from=0&size=1，
    
    理解为什么深度分页是有问题的，我们可以假设在一个有 5 个主分片的索引中搜索。 当我们请求结果的第一页（结果从 1 到 10 ），每一个分片产生前 10 的结果，并且返回给 协调节点 ，协调节点对 50 个结果排序得到全部结果的前 10 个。
    
    现在假设我们请求第 1000 页--结果从 10001 到 10010 。所有都以相同的方式工作除了每个分片不得不产生前10010个结果以外。 然后协调节点对全部 50050 个结果排序最后丢弃掉这些结果中的 50040 个结果。
    
    可以看到，在分布式系统中，对结果排序的成本随分页的深度成指数上升。这就是 web 搜索引擎对任何查询都不要返回超过 1000 个结果的原因。


## 映射和分析

1. 精确值和全文
2. 倒排索引
3. 分析与分析器

    分析 包含下面的过程：

    + 首先，将一块文本分成适合于倒排索引的独立的 词条 ，
    + 之后，将这些词条统一化为标准格式以提高它们的"可搜索性"，或者 recall

    分析器 实际上是将三个功能封装到了一个包里：
    
    + 字符过滤器
首先，字符串按顺序通过每个 字符过滤器 。他们的任务是在分词前整理字符串。一个字符过滤器可以用来去掉HTML，或者将 & 转化成 `and`。
    + 分词器
其次，字符串被 分词器 分为单个的词条。一个简单的分词器遇到空格和标点的时候，可能会将文本拆分成词条。
    + Token 过滤器
    最后，词条按顺序通过每个 token 过滤器 。这个过程可能会改变词条
    
    ### 内置分析器
    
    对于一个例子
    
        "Set the shape to semi-transparent by calling set_trans(5)"
    
    
    + 标准分析器：它根据 Unicode 联盟 定义的 单词边界 划分文本。删除绝大部分标点。最后，将词条小写。
        `set, the, shape, to, semi, transparent, by, calling, set_trans, 5`
    + 简单分析器
    简单分析器在任何不是字母的地方分隔文本，将词条小写
        `set, the, shape, to, semi, transparent, by, calling, set, trans`
    + 空格分析器
    空格分析器在空格的地方划分文本
    
        `Set, the, shape, to, semi-transparent, by, calling, set_trans(5)`
    + 语言分析器
    
        特定语言分析器可用于 很多语言。它们可以考虑指定语言的特点。例如， 英语 分析器附带了一组英语无用词（常用单词，例如 and 或者 the ，它们对相关性没有多少影响），它们会被删除。 由于理解英语语法的规则，这个分词器可以提取英语单词的 词干 .
        
        `set, shape, semi, transpar, call, set_tran, 5`
        
    ### 什么时候使用分析器
    
    全文查询，理解每个域是如何定义的，因此它们可以做 正确的事：
    
    + 当你查询一个 全文 域时， 会对查询字符串应用相同的分析器，以产生正确的搜索词条列表。
    + 当你查询一个 精确值 域时，不会分析查询字符串， 而是搜索你指定的精确值。
    
    ### 注意：
    
    当Elasticsearch在你的文档中检测到一个新的字符串域 ，它会自动设置其为一个全文 字符串 域，使用 标准 分析器对它进行分析。
    
    
    ### 测试分析器
    
    
        GET /_analyze
        {
          "analyzer": "standard",
          "text": "Text to analyze"
        }
    这样得到分析结果
    
        {
           "tokens": [
              {
                 "token":        "text",
                 "start_offset": 0,
                 "end_offset":   4,
                 "type":         "<ALPHANUM>",
                 "position":     1
              },
              {
                 "token":        "to",
                 "start_offset": 5,
                 "end_offset":   7,
                 "type":         "<ALPHANUM>",
                 "position":     2
              },
              {
                 "token":        "analyze",
                 "start_offset": 8,
                 "end_offset":   15,
                 "type":         "<ALPHANUM>",
                 "position":     3
              }
           ]
        }
        
4. 映射

    

    Elasticsearch 支持 如下简单域类型：

    + 字符串: string
    + 整数 : byte, short, integer, long
    + 浮点数: float, double
    + 布尔型: boolean
    + 日期: date

    JSON中类型与Elasticsearch类型对应
    
    |Json类型|Elasticsearch类型|
    |---|---|
    |true/false|boolean|
    |123|long|
    |123.45|double|
    |字符串，有效日期: 2014-09-15|date|
    |字符串foo bar| string|
    
    ### string域 
    string 域映射的两个最重要 属性是 index 和 analyzer 。
    index的取值
    
    |index取值|含义|
    |---|---|
    | analyzed|首先分析字符串，然后索引它。换句话说，以全文索引这个域|
    | not_analyzed|索引这个域，所以可以搜索到它，但索引指定的精确值。不对它进行分析|
    | no|不索引这个域。这个域不会被搜索到|
    
    analyzer的取值
    
    对于 analyzed 字符串域，用 analyzer 属性指定在搜索和索引时使用的分析器。默认， Elasticsearch 使用 standard 分析器， 但你可以指定一个内置的分析器替代它，例如 whitespace 、 simple 和 `english`

5. 复杂核心域类型

    + 数组 { "tag": [ "search", "nosql" ]}
        +   数组中所有的值必须是相同数据类型的
    + 空域 null,[],[null]
        + 空域不会被索引

    + 多层级对象
        内部对象 经常用于 嵌入一个实体或对象到其它对象中。
        Elasticsearch 会动态 监测新的对象域并映射它们为 对象 ，在 properties 属性下列出内部域，
        + user 和 name 域的映射结构与 tweet 类型的相同。事实上， type 映射只是一种特殊的 对象 映射，我们称之为 根对象 。除了它有一些文档元数据的特殊顶级域，例如 _source 和 _all 域，它和其他对象一样。


            
                {
                  "gb": {
                    "tweet": { 
                      "properties": {
                        "tweet":            { "type": "string" },
                        "user": { 
                          "type":             "object",
                          "properties": {
                            "id":           { "type": "string" },
                            "gender":       { "type": "string" },
                            "age":          { "type": "long"   },
                            "name":   { 
                              "type":         "object",
                              "properties": {
                                "full":     { "type": "string" },
                                "first":    { "type": "string" },
                                "last":     { "type": "string" }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                } 
    
    
        + 内部对象是如何进行索引的。

       
            Lucene 不理解内部对象。 Lucene 文档是由一组键值对列表组成的。为了能让 Elasticsearch 有效地索引内部类，它把我们的文档转化成这样：
            
                {
                    "tweet":            [elasticsearch, flexible, very],
                    "user.id":          [@johnsmith],
                    "user.gender":      [male],
                    "user.age":         [26],
                    "user.name.full":   [john, smith],
                    "user.name.first":  [john],
                    "user.name.last":   [smith]
                }
                
        + 内部对象数组 最后，考虑包含 内部对象的数组是如何被索引的
    
            假设我们有个 followers 数组：
    
                {
                    "followers": [
                        { "age": 35, "name": "Mary White"},
                        { "age": 26, "name": "Alex Jones"},
                        { "age": 19, "name": "Lisa Smith"}
                    ]
                }
            
            这个文档会像我们之前描述的那样被扁平化处理，结果如下所示：
    
                {
                    "followers.age":    [19, 26, 35],
                    "followers.name":   [alex, jones, lisa, smith, mary, white]
                }



## 请求体查询

1. 查询表达式


    一个查询语句的典型结构：
    
        {
            QUERY_NAME: {
                ARGUMENT: VALUE,
                ARGUMENT: VALUE,...
            }
        }
        
    如果针对某个字段，其结构如下：
    
        {
            QUERY_NAME: {
                FIELD_NAME: {
                    ARGUMENT: VALUE,
                    ARGUMENT: VALUE,...
                }
            }
        }
    举个例子，你可以使用 match 查询语句 来查询 tweet 字段中包含 elasticsearch 的 tweet：

        {
            "match":{
                "tweet":"elasticsearch"
            }
        }
        
    完整的查询语句：
    
        GET /_search
        {
            "query": {
                "match": {
                    "tweet": "elasticsearch"
                }
            }
        }
2. 合并查询语句

    查询语句Query Clasues就像一些简单的组合块，这些组合块可以彼此之间合并组成更复杂的查询。如下：
    
    + 叶子语句 Leaf Clauses：用于将查询字符串和一个字段对比
    + 复合Compound语句：合并其他查询语句，比如一个bool语句 允许在你需要的时候组合其他语句，无论是`must`，`must not`, `should`，同时它可以包含不评分的过滤器`filters`

            {
                "bool": {
                    "must":     { "match": { "tweet": "elasticsearch" }},
                    "must_not": { "match": { "name":  "mary" }},
                    "should":   { "match": { "tweet": "full text" }},
                    "filter":   { "range": { "age" : { "gt" : 30 }} }
                }
            }
    
    一条复合语句可以合并 任何 其它查询语句，包括复合语句，了解这一点是很重要的。这就意味着，复合语句之间可以互相嵌套，可以表达非常复杂的逻辑。
    
3. 查询与过滤

    Elasticsearch使用的查询语言可以以无限组合的方式进行搭配，可以在以下两种情况下使用：过滤情况(filtering context)和查询情况（query context)。
    
    + 过滤情况：回答是 yes/no，二者必居其一。
    + 查询情况：查询就变成了一个“评分”的查询。        
    
    但从 Elasticsearch 2.0 开始，过滤（filters）已经从技术上被排除了，同时所有的查询（queries）拥有变成不评分查询的能力
    
    性能差异
    
    + 过滤查询（Filtering queries）只是简单的检查包含或者排除，这就使得计算起来非常快。考虑到至少有一个过滤查询（filtering query）的结果是 “稀少的”（很少匹配的文档），并且经常使用不评分查询（non-scoring queries），结果会被缓存到内存中以便快速读取，所以有各种各样的手段来优化查询结果。
    + 评分查询（scoring queries）不仅仅要找出 匹配的文档，还要计算每个匹配文档的相关性，计算相关性使得它们比不评分查询费力的多。同时，查询结果并不缓存。

    如何选择过滤或评分
    
    通常的规则是，使用 查询（query）语句来进行 全文 搜索或者其它任何需要影响 相关性得分 的搜索。除此以外的情况都使用过滤（filters)。
    
4. 重要的查询

    + `match_all` 查询简单的 匹配所有文档
    + `match`查询：无论你在任何字段上进行的是全文搜索还是精确查询，match 查询是你可用的标准查询。
        + 如果是全文搜索，会先进行分析 { "match": { "tweet": "About Search" }}

    + `multi_match`查询：multi_match 查询可以在多个字段上执行相同的 match 查询

            {
                "multi_match": {
                    "query":    "full text search",
                    "fields":   [ "title", "body" ]
                }
            }
    + `range`查询 查询找出那些落在指定区间内的数字或者时间，被允许的操作符：gt,lt,gte,lte

            {
                "range": {
                    "age": {
                        "gte":  20,
                        "lt":   30
                    }
                }
            }
            
    + `term`查询 term 查询被用于精确值 匹配，这些精确值可能是数字、时间、布尔或者那些 not_analyzed 的字符串

            { "term": { "age":    26           }}
            { "term": { "date":   "2014-09-01" }}
    + `terms`查询：和 term 查询一样，但它允许你指定多值进行匹配。如果这个字段包含了指定值中的任何一个值，那么这个文档满足条件

            { "terms": { "tag": [ "search", "full_text", "nosql" ] }}


5. 组合查询

    + `bool查询`，可以接收以下参数：

        + must:文档 必须 匹配这些条件才能被包含进来。
        + must_not:文档 必须不 匹配这些条件才能被包含进来
        + should:如果满足这些语句中的任意语句，将增加 _score ，否则，无任何影响。它们主要用于修正每个文档的相关性得分
        + filter:必须 匹配，但它以不评分、过滤模式来进行。这些语句对评分没有贡献，只是根据过滤标准来排除或包含文档。

    举例：下面的查询用于查找 title 字段匹配 how to make millions 并且不被标识为 spam 的文档。那些被标识为 starred 或在2014之后的文档，将比另外那些文档拥有更高的排名。如果 _两者_ 都满足，那么它排名将更高：
    
        {
            "bool": {
                "must":     { "match": { "title": "how to make millions" }},
                "must_not": { "match": { "tag":   "spam" }},
                "should": [
                    { "match": { "tag": "starred" }},
                    { "range": { "date": { "gte": "2014-01-01" }}}
                ]
            }
        }
        
    假如不想文档的时间影响评分，则可以`增加带过滤器的查询`：
    
        {
            "bool": {
                "must":     { "match": { "title": "how to make millions" }},
                "must_not": { "match": { "tag":   "spam" }},
                "should": [
                    { "match": { "tag": "starred" }}
                ],
                "filter": {
                  "range": { "date": { "gte": "2014-01-01" }} 
                }
            }
        }
        
    所有查询都可以借鉴这种方式。将查询移到 bool 查询的 filter 语句中，这样它就自动的转成一个不评分的 filter 了。

+ `constant_score`查询

    constant_score 查询也是你工具箱里有用的查询工具。它将一个不变的常量评分应用于所有匹配的文档。它被经常用于你只需要执行一个 filter 而没有其它查询。
    可以使用它来取代只有 filter 语句的 bool 查询。在性能上是完全相同的，但对于提高查询简洁性和清晰度有很大帮助。
    
    	
    term 查询被放置在 constant_score 中，转成不评分的 filter

        {
            "constant_score":   {
                "filter": {
                    "term": { "category": "ebooks" } 
                }
            }
        }


## 排序和相关性

1. 排序

    为了按照相关性来排序，需要将相关性表示为一个数值。在 Elasticsearch 中， 相关性得分 由一个浮点数进行表示，并在搜索结果中通过 _score 参数返回， 默认排序是 _score 降序。
    
    + 按照字段的值进行排序
        
        我们可以使用 sort 参数进行实现
        
        返回结果中`_score`是不会被计算的。
        
            GET /_search
        
            {
                "query" : {
                    "bool" : {
                        "filter" : { "term" : { "user_id" : 1 }}
                    }
                },
                "sort": { "date": { "order": "desc" }}
            }
            
    + 多级排序
    
            GET /_search
            {
                "query" : {
                    "bool" : {
                        "must":   { "match": { "tweet": "manage text search" }},
                        "filter" : { "term" : { "user_id" : 2 }}
                    }
                },
                "sort": [
                    { "date":   { "order": "desc" }},
                    { "_score": { "order": "desc" }}
                ]
            }


2. [相关性](https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/relevance-intro.html)
    
    Elasticsearch 的相似度算法 被定义为检索词频率/反向文档频率， TF/IDF，包括以下内容：
    
3. [Doc Value介绍](https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/docvalues-intro.html)


## 执行分布式检索


## 索引管理
    
1. 创建一个索引

        PUT /my_index
        {
            "settings": { ... any settings ... },
            "mappings": {
                "type_one": { ... any mappings ... },
                "type_two": { ... any mappings ... },
                ...
            }
        }

    action.auto_create_index: false

2. 删除一个索引

        DELETE /my_index
        DELETE /my_index*

3. 索引设置

    两个最重要的设置
    
    + `number_of_shards`
每个索引的主分片数，默认值是 5 。这个配置在索引创建后不能修改。
    + `number_of_replicas`
每个主分片的副本数，默认值是 1 。对于活动的索引库，这个配置可以随时修改。

    修改副本配置
    
        PUT /my_temp_index/_settings
        {
            "number_of_replicas": 1
        }
4. 配置分析器

    第三个重要的索引设置是 analysis 部分， 用来配置已存在的分析器或针对你的索引创建新的自定义分析器。



    `standard `分析器是用于全文字段的默认分析器， 对于大部分西方语系来说是一个不错的选择。 它包括了以下几点：

    + standard 分词器，通过单词边界分割输入的文本。
    + standard 语汇单元过滤器，目的是整理分词器触发的语汇单元（但是目前什么都没做）。
    + lowercase 语汇单元过滤器，转换所有的语汇单元为小写。
    + stop 语汇单元过滤器，删除停用词--对搜索相关性影响不大的常用词，如 a ， the ， and ， is 。
    
        默认情况下，停用词过滤器是被禁用的。如需启用它，你可以通过创建一个基于 standard 分析器的自定义分析器并设置 stopwords 参数。 可以给分析器提供一个停用词列表，或者告知使用一个基于特定语言的预定义停用词列表

5. 自定义分析器

    一个 分析器 就是在一个包里面组合了三种函数的一个包装器
    
    + 字符过滤器：用来处理一个尚未被分词的字符串。可以包含多个字符过滤器
    + 分词器：一个分析器 必须 有一个唯一的分词器。 分词器把字符串分解成单个词条或者词汇单元。 standard 分析器里使用的standard分词器 把一个字符串根据单词边界分解成单个词条，并且移除掉大部分的标点符号。
    + token过滤器：经过分词，作为结果的 词单元流 会按照指定的顺序通过指定的词单元过滤器。

    ### 创建一个自定义的分析器
    
    格式：
    
        PUT /my_index
        {
            "settings": {
                "analysis": {
                    "char_filter": { ... custom character filters ... },
                    "tokenizer":   { ...    custom tokenizers     ... },
                    "filter":      { ...   custom token filters   ... },
                    "analyzer":    { ...    custom analyzers      ... }
                }
            }
        }
    
    字符过滤器：将&替换为and,
    
          "char_filter": {
                "&_to_and": {
                "type":       "mapping",
                "mappings": [ "&=> and "]
          }},
    
    token过滤器：
    
        "filter": {
            "my_stopwords": {
                "type":       "stop",
                "stopwords": [ "the", "a" ]
            }
        },
    分析：
    
        "analyzer": {
            "my_analyzer": {
                "type":         "custom",
                "char_filter":  [ "html_strip", "&_to_and" ],
                "tokenizer":    "standard",
                "filter":       [ "lowercase", "my_stopwords" ]
            }
        }
                    
    整体设置：
    
        PUT /my_index
        {
            "settings": {
                "analysis": {
                    "char_filter": {
                        "&_to_and": {
                            "type":       "mapping",
                            "mappings": [ "&=> and "]
                    }},
                    "filter": {
                        "my_stopwords": {
                            "type":       "stop",
                            "stopwords": [ "the", "a" ]
                    }},
                    "analyzer": {
                        "my_analyzer": {
                            "type":         "custom",
                            "char_filter":  [ "html_strip", "&_to_and" ],
                            "tokenizer":    "standard",
                            "filter":       [ "lowercase", "my_stopwords" ]
                    }}
        }}}
        
    索引被创建以后，使用 analyze API 来 测试这个新的分析器：
    
        curl -XGET "http://10.95.177.126:9201/my_index/_analyze?analyzer=my_analyzer&pretty" -d " The quick & brown fox"
        {
          "tokens" : [ {
            "token" : "quick",
            "start_offset" : 5,
            "end_offset" : 10,
            "type" : "<ALPHANUM>",
            "position" : 1
          }, {
            "token" : "and",
            "start_offset" : 11,
            "end_offset" : 12,
            "type" : "<ALPHANUM>",
            "position" : 2
          }, {
            "token" : "brown",
            "start_offset" : 13,
            "end_offset" : 18,
            "type" : "<ALPHANUM>",
            "position" : 3
          }, {
            "token" : "fox",
            "start_offset" : 19,
            "end_offset" : 22,
            "type" : "<ALPHANUM>",
            "position" : 4
          } ]
        }
        
6. 类型和映射

    + `类型 `在 Elasticsearch 中表示一类相似的文档
    + `映射` 就像数据库中的 schema ，描述了文档可能具有的字段或 属性 、 每个字段的数据类型，以及Lucene是如何索引和存储这些字段的
    
    避免类型陷阱
    
    这导致了一个有趣的思想实验： 如果有两个不同的类型，每个类型都有同名的字段，但映射不同（例如：一个是字符串一个是数字），将会出现什么情况？
    
    简单回答是，Elasticsearch 不会允许你定义这个映射。当你配置这个映射时，将会出现异常。


7. 根对象


    映射的最高一层被称为 根对象 ，它可能包含下面几项：
    
    + 一个 properties 节点，列出了文档中可能包含的每个字段的映射
    + 各种元数据字段，它们都以一个下划线开头，例如 _type 、 _id 和 _source
    + 设置项，控制如何动态处理新的字段，例如 analyzer 、 dynamic_date_formats 和 dynamic_templates
    + 其他设置，可以同时应用在根对象和其他 object 类型的字段上，例如 enabled 、 dynamic 和 include_in_all

    ### 属性
    
    + type：字段的数据类型，例如 string 或 date
    + index：字段是否应当被当成全文来搜索（ analyzed ），或被当成一个准确的值（ not_analyzed ），还是完全不可被搜索（ no ）
    + analyzer：确定在索引和搜索时全文字段使用的 analyzer
    + ip ,`geo_point` , `geo_shape`

    ### 元数据：`_source`字段
    
    这个字段的存储几乎总是我们想要的，因为它意味着下面的这些：

    + 搜索结果包括了整个可用的文档——不需要额外的从另一个的数据仓库来取文档。
    + 如果没有 _source 字段，部分 update 请求不会生效。
    + 当你的映射改变时，你需要重新索引你的数据，有了_source字段你可以直接从Elasticsearch这样做，而不必从另一个（通常是速度更慢的）数据仓库取回你的所有文档。
    + 当你不需要看到整个文档时，单个字段可以从 _source 字段提取和通过 get 或者 search 请求返回。
    + 调试查询语句更加简单，因为你可以直接看到每个文档包括什么，而不是从一列id猜测它们的内容。
    
    ### 元数据: `_all`字段
    
    _all 字段：一个把其它字段值 当作一个大字符串来索引的特殊字段
    
    如果你不再需要 _all 字段，你可以通过下面的映射来禁用：

        PUT /my_index/_mapping/my_type
        {
            "my_type": {
                "_all": { "enabled": false }
            }
        }
        
    ### 元数据：文档标识
    
    + `_id`: 文档的 ID 字符串
    + `_type`:  文档的类型名
    + `_index`: 文档所在的索引
    + `_uid`: `_type` 和 `_id`连接在一起构造成 type#id

8. 动态映射

    当 Elasticsearch 遇到文档中以前 未遇到的字段，它用 dynamic mapping 来确定字段的数据类型并自动把新的字段添加到类型映射。
    
    dynamic取值
    
    + true：动态添加新的字段--缺省
    + false：忽略新的字段
    + strict：如果遇到新字段抛出异常
    + 
    下面的设置中，对于`my_type`出现新增字段就报异常，如果对于字段stash出现新增字段，则动态映射。
    

            PUT /my_index
            {
                "mappings": {
                    "my_type": {
                        "dynamic":      "strict", 
                        "properties": {
                            "title":  { "type": "string"},
                            "stash":  {
                                "type":     "object",
                                "dynamic":  true 
                            }
                        }
                    }
                }
            }
            
9. 自定义动态映射

    `https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/custom-dynamic-mapping.html`
    
10. 缺省映射

    `https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/default-mapping.html`
    
11. 重新索引你的数据

    从ES2.3开始，可以使用[reindex](https://www.elastic.co/guide/en/elasticsearch/reference/master/docs-reindex.html) api，它能够对文档重建索引而不需要任何插件或外部工具。
    
12. 索引别名和零停机

    索引 别名 就像一个快捷方式或软连接，可以指向一个或多个索引，也可以给任何一个需要索引名的API来使用。别名 带给我们极大的灵活性，允许我们做下面这些：

    + 在运行的集群中可以无缝的从一个索引切换到另一个索引
    + 给多个索引分组 (例如， last_three_months)
    + 给索引的一个子集创建 视图

    举例：
    
        PUT /my_index_v1  #创建索引my_index_v1
        PUT /my_index_v1/_alias/my_index  # 设置别名 my_index指向my_index_v1
        
        
        
## 分片内部原理

+ 为什么搜索是 近 实时的？(内存缓冲区不可被搜索，写入文件系统缓存后可以被搜索）
+ 为什么文档的 CRUD (创建-读取-更新-删除) 操作是 实时 的?
+ Elasticsearch 是怎样保证更新被持久化在断电时也不丢失数据?
为什么删除文档不会立刻释放空间？
+ refresh, flush, 和 optimize API 都做了什么, 你什么情况下应该是用他们？


1. 使文本可搜索

    ### 不变性
    
    + 倒排索引被写入磁盘后是 不可改变 的:它永远不会修改。 不变性有重要的价值：

        + 不需要锁。如果你从来不更新索引，你就不需要担心多进程同时修改数据的问题。
        + 一旦索引被读入内核的文件系统缓存，便会留在哪里，由于其不变性。只要文件系统缓存中还有足够的空间，那么大部分读请求会直接请求内存，而不会命中磁盘。这提供了很大的性能提升。
        + 其它缓存(像filter缓存)，在索引的生命周期内始终有效。它们不需要在每次数据改变时被重建，因为数据不会变化。
        + 写入单个大的倒排索引允许数据被压缩，减少磁盘 I/O 和 需要被缓存到内存的索引的使用量

2. [动态更新索引](https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/dynamic-indices.html)

    下一个需要被解决的问题是怎样在保留不变性的前提下实现倒排索引的更新？ 答案是: 用更多的索引。






### 索引和分片

被混淆的概念是，一个 Lucene 索引 我们在 Elasticsearch 称作 分片 。 一个 Elasticsearch 索引 是分片的集合。 当 Elasticsearch 在索引中搜索的时候， 他发送查询到每一个属于索引的分片(Lucene 索引)，然后像 [执行分布式检索](https://elasticsearch.cn/book/elasticsearch_definitive_guide_2.x/distributed-search.html)  提到的那样，合并每个分片的结果到一个全局的结果集。




sync is a standard system call in the Unix operating system, which commits to disk all data in the kernel filesystem buffers, i.e., data which has been scheduled for writing via low-level I/O system calls. Higher-level I/O layers such as stdio may maintain separate buffers of their own.




### 数据写入的流程

要回答的问题是：

+ 为什么搜索是 近 实时的？
+ 为什么文档的 CRUD (创建-读取-更新-删除) 操作是 实时 的?
+ Elasticsearch 是怎样保证更新被持久化在断电时也不丢失数据?
+ 为什么删除文档不会立刻释放空间？
+ refresh, flush, 和 optimize API 都做了什么, 你什么情况下应该是用他们？
    + refresh将内存缓冲区数据写入文件系统缓存的segment，可以被搜索（近实时搜索）
    + flush将文件系统缓存的数据写入磁盘。执行一个提交并且截断 translog 的行为



1. 倒排索引写入文件以后就是不变的
2. 倒排索引写入的流程

    --->写入内存buffer-->写入文件系统缓存（一个segment）
    
    --->translog 
    
    数据写入时在写入内存buffer时，会同时写入translog；
    内存buffer中的数据每隔refresh_interval（es设置，默认是1秒）写入文件系统缓存；这就是为什么我们说 Elasticsearch 是 近 实时搜索: 文档的变化并不是立即对搜索可见，但会在一秒之内变为可见
    当translog到达一定阈值后
        
    + 所有在内存缓冲区的文档会被写入一个新的段
    + 缓冲区被清空
    + 一个提交点被写入硬盘
    + 文件系统缓存通过 fsync 被刷新（flush）
    + 老的 translog 被删除


这个执行一个提交并且截断 translog 的行为在 Elasticsearch 被称作一次 flush 。 `分片每30分钟被自动刷新`（flush），或者在 `translog 太大的时候`也会刷新。请查看 translog 文档 来设置，它可以用来 控制这些阈值：



3.  提交点 的概念 — 一个列出了所有已知段的文件
4. 下面的图演示了一个完整的数据写入流程
    
    {% asset_img elas_新文档写入内存和translog.png 新文档写入内存和translog %}
    {% asset_img elas_刷新缓存清空事务日志不会.png 刷新缓存清空事务日志不会 %}
    {% asset_img elas_事务日志不断积累文档.png 事务日志不断积累文档 %}
    {% asset_img elas_事务日志_内存清空.png 事务日志 内存清空 %}



5. 段合并。由于自动刷新流程每秒会创建一个新的段 ，这样会导致短时间内的段数量暴增。而段数目太多会带来较大的麻烦。 每一个段都会消耗文件句柄、内存和cpu运行周期。更重要的是，每个搜索请求都必须轮流检查每个段；所以段越多，搜索也就越慢。

    启动段合并不需要你做任何事。进行索引和搜索时会自动进行。会进行以下工作：

    + 当索引的时候，刷新（refresh）操作会创建新的段并将段打开以供搜索使用。
    + 合并进程选择一小部分大小相似的段，并且在后台将它们合并到更大的段中。这并不会中断索引和搜索。
    
    ![两个提交了的段和一个未提交的段进行合并](elas_两个提交了的段和一个未提交的段进行合并.png)



    一旦合并结束，老的段被删除，
    
    ![合并结束老的段被删除](elas_合并结束老的段被删除.png)


    通过optimize API大可看做是 强制合并 API 。它会将一个分片强制合并到 `max_num_segments` 参数指定大小的段数目。 这样做的意图是减少段的数量（通常减少到一个），来提升搜索性能
    
        POST /logstash-2014-10/_optimize?max_num_segments=1 






