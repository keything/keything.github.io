

1. 查询时是否查询所有分片。 

	如果在查询中指定了routing字段， 那么将只在特定shard上进行操作。 
	如果在查询中没有指定routing字段，默认会在该索引的所有shard上进行操作。 
	routing字段有两个位置：
		a. query string中加入。 _search?routing=hello,mmm 的方式
		b. _msearch的header中加入 {"index":"myindex","routing":"hello,mm"}

	打开trace日志，观察查询过程。可以清晰看到是query then fetch的执行过程。 
	[链接](https://www.elastic.co/guide/en/elasticsearch/reference/6.6/search-multi-search.html)

2. 写入时路由规则指定。

	[bulk接口文档](https://www.elastic.co/guide/en/elasticsearch/reference/6.6/docs-bulk.html)

	bulk是用于创建、删除、更新索引使用。 
	其中routing是要放在query string中才生效。 

	写入举例：
	`
	curl  -s -H "Content-Type: application/x-ndjson" -XPOST localhost:9201/_bulk -d '
	{"index":{"_index":"index_sorting_2","_type":"prod"}, "routing":"hello"}
	{"address":"北京我爱你","address_nor_pre":"北京我爱你","click_score":6}
	'
	`
3. 关于search_type解释
	
	[search_type文档](https://www.elastic.co/guide/en/elasticsearch/reference/current/search-request-search-type.html)

	注意：在es5.3版本后，已经移除search_type=query_and_fetch的选项。 

	原因：query_and_fetch 主要是用来进行内部优化的，如果你只有一个分片，你完全不需要分两个阶段（记住，你只有一个分片，不用管其他分片的情况），先拿一次分片下的得分信息再拿具体的文档，不如一次性拿回来就好了。因此query_and_fetch从来不是直接给用户使用的。 

	search_type=QUERY_THEN_FETCH 从日志来看
	
	`
	[2019-02-04T09:50:43,512][TRACE][o.e.a.s.TransportSearchAction] [ToJiwmL] got first-phase result from [ToJiwmLwTlaxtovU98sizw][index_sorting_2][0]
	[2019-02-04T09:50:43,513][TRACE][o.e.a.s.TransportSearchAction] [ToJiwmL] got first-phase result from [ToJiwmLwTlaxtovU98sizw][index_sorting_2][1]
	[2019-02-04T09:50:43,514][TRACE][o.e.a.s.TransportSearchAction] [ToJiwmL] [query] Moving to next phase: [fetch], based on results from: [ToJiwmLwTlaxtovU98sizw][index_sorting_2][0],[ToJiwmLwTlaxtovU98sizw][index_sorting_2][1] (cluster state version: 15)
	[2019-02-04T09:50:43,517][TRACE][o.e.a.s.TransportSearchAction] [ToJiwmL] [fetch] Moving to next phase: [expand], based on results from: [ToJiwmLwTlaxtovU98sizw][index_sorting_2][0],[ToJiwmLwTlaxtovU98sizw][index_sorting_2][1] (cluster state version: 15)
	[2019-02-04T09:50:43,518][TRACE][o.e.a.s.TransportSearchAction] [ToJiwmL] [expand] Moving to next phase: [response], based on results from: [ToJiwmLwTlaxtovU98sizw][index_sorting_2][0],[ToJiwmLwTlaxtovU98sizw][index_sorting_2][1] (cluster state version: 15)
	`

4. reroute操作

	[reroute接口文档]()

	举例：
	`
	curl -XPOST -H"Content-Type: application/json" 'localhost:9200/_cluster/reroute' -d '{  
    "commands" : [ 
    	{  
	        "move" :   {  
	              "index" : "index_sorting_1", 
	              "shard" : 3,   
	              "from_node" : "ToJiwmL", 
	              "to_node" : "Febek5M"  
	        }  
	    }
	      
    ]  
	}'  
	`

5. 磁盘空间阈值的设置

	在本地mac电脑执行reroute时报错：

	NO(the node is above the low watermark cluster setting [cluster.routing.allocation.disk.watermark.low=85%], using more disk space than the maximum allowed [85.0%]

	
	[文章](https://elasticsearch.cn/question/4911)中介绍了：
	1. 这个参数是对“未分配过的"主分片无效，即新创建的索引的场景。 也就是说一个结点的磁盘空间消耗达到low watermark以后，新创建的索引不受限制，依然可以分配到上面
	2. 不能够分配上去的场景主要是以下两个:
	集群有结点挂掉或者用户更改了集群的复制片数量， 需要挑选某个结点复制分片的时候
	集群触发了rebalance的时候


	在我们的case中需要做的是：
	`
	curl -XPUT -H"Content-Type: application/json" "localhost:9200/_cluster/settings" -d '
	{
		"transient":{
			"cluster.routing.allocation.disk.watermark.low":"90%"
		}  
	}
	'`

