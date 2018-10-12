---
title: nginx-location匹配
date:   2016-05-04 18:00:01 +0800
tags: nginx
---
这篇文字是对官方文档中location[1]:http://nginx.org/en/docs/http/ngx_http_core_module.html#location 的解读。

## 一. 语法
	Syntax:
	
	location [ = | ~ | ~* | ^~ ] uri { ... }
	location @name { ... }
	
	Default:
	
	—
	
	Context:
	
	server, location
## 二. 解释
根据请求URI进行配置。
     
与归一化的URI进行匹配。 归一化包括：1）将已经编码为%XX格式的URI解码为普通格式; 2）解析references到相对路径. 和..; 3）将多个连接的斜杠/ 变为单个斜杠/。

1. 一个location可以是前缀字符串，也可以是正则表达式。
2. 正则表达式由 ~*（大小写不敏感） 或 ~（大小写敏感）指定。
3. 为了找到与给定请求相匹配的location，nginx首先检查```前缀字符串（前缀location）```定义的location。在这些前缀字符串中，选择```最长匹配的前缀```并将其记住。
4. 之后检查```正则表达式``，检查正则表达式是按照其在配置文件中的顺序检查，当正则表达式第一次匹配后，就停止正则表达式的匹配，对应的正则的location的配置就会被使用。 
5. 如果没有与正则表达式相匹配的location，那么最长前缀location就被使用。
6. 如果最长匹配前缀location有^~的修饰符，那么就不会检查正则表达式。
7. 同时，可以使用=修饰符，该修饰符定义了URI和location的准确匹配。如果一个准确的match被找到，那么搜索停止。



location检查的策略：


    1. 首先检查指定了前缀的location块 普通的location块。
    2. 其次检查带正则的location块 顺序匹配，一旦匹配到第一个就停止后面的匹配。
    对于匹配正则location并不是所有情况都进行，有两种情况是例外：
    1）普通location前面指定了^~ 告诉nginx只要普通location一旦匹配上，则不需要继续正则匹配。匹配上了普通location就不进行正则匹配。
    2）普通location恰好严格匹配上，不是最大前缀匹配，则不再继续匹配正则。
   
   	简要概述：
		正则 location 匹配让步普通 location 的严格精确匹配结果；但覆盖普通 location 的最大前缀匹配结果
	如下图所示：
![location_match](/images/nginx_location_match.png)
		
举例说明：

	location = / {
	    [ configuration A ]
	}
	
	location / {
	    [ configuration B ]
	}
	
	location /documents/ {
	    [ configuration C ]
	}
	
	location ^~ /images/ {
	    [ configuration D ]
	}
	
	location ~* \.(gif|jpg|jpeg)$ {
	    [ configuration E ]
	}

	请求/ 匹配配置A，/index.html匹配配置B，/documents匹配配置C，/images/1.gif匹配配置D，/documents/1.jpg匹配配置E


注意：@前缀定义了一个命名location。这样一个location不是用于正则请求处理，而是用于请求的重定向。他们不能嵌套在location中，也不能包含嵌套的location。

参考文章

http://nginx.org/en/docs/http/ngx_http_core_module.html#location
