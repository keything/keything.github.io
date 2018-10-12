---
title: nginx模块执行顺序
date:   2016-05-08 18:00:01 +0800
tags: nginx
---

整篇文章参考
1. http://openresty.org/download/agentzh-nginx-tutorials-zhcn.html
2. http://www.nginxguts.com/2011/01/phases/

# 一、概述

## 1. 简要介绍
 在每个阶段都可以注册多个自定义的handlers。但是下面的几个阶段是不能够注册自定义的handler的：
 
     1. find-config阶段
     2. post-access阶段
     3. post-rewrite阶段
     4. try-files阶段

每个阶段都有一个与之相关的handler的列表。一旦把handler注册到对应阶段，那么handler就会返回下面的取值：

	NGX_OK：请求已经成功处理，请求将会传到下一个阶段。
	
	NGX_DECLINED：请求需要被转发到本阶段的下一个handler
	
	NGX_AGAIN,NGX_DONE：请求已经被正确处理，同时请求被挂起，直到某个事件（子请求结束、socket可写或超时等）到来，handler才会再次被调用

## 2. 如何注册自定义的handler

 为了在某个阶段注册自定义的handler，需要找到```ngx_http_core_module```，并添加（```ngx_array_push```)到指定的phrase向量中。handlers以相反的顺序被调用。因此在配置中最后注册的handler会首先被调用。

  从上面可以看出，请求处理的顺序和配置文件中的配置指令的先后顺序无关，无论配置文件中指令的顺序如何，各个阶段的处理函数都会按照预先的顺序执行。因此一个处理函数需要知道自己什么时候会被调用，何时需要返回NGX_DECLINED，而且保证减少性能损耗。
 
## 3. 各个phase对于返回值的处理：

1. Access阶段

		NGX_OK：允许访问请求URI指定的资源
   		NGX_HTTP_FORBIDDEN，NGX_HTTP_UNAUTHORIZED：不允许访问请求URI指定的资源
2. Content阶段

			NGX_AGAIN或NGX_DONE时，content handler不会再被调用。取而代之的handler改变请求的读或写的handler
	       如果content handler返回NGX_DECLINED， nginx将会将请求转发到content phrase handlers。

3. 关于content phase handler与content handler的区别

a. content phase handler是混乱的：每个到达content阶段的请求都可以调用content phase handler. 对于配置content handler的location，这些请求只能调用一次content handler。

b. 在一个location中可以调用多个content phase handler。但是在一个location中只能有一个content handler。

这儿借用agentzh 文章Nginx配置指令的执行顺序（五）中例子。

    location /test {
        echo_before_body "before...";
        proxy_pass http://127.0.0.1:8080/foo;
        echo_after_body "after...";
    }

    location /foo {
        echo "contents to be proxied";
    }

测试结果表明这一次我们成功了：

    $ curl 'http://localhost:8080/test'
    before...
    contents to be proxied
    after...

    因此我们知道echo_before_body 和echo_after_body指令都属于content phase handler。也就是Agentzh文中的输出过滤器。

# 二、详细介绍

Nginx 处理请求的过程一共划分为 11 个阶段，按照执行顺序依次是 post-read、server-rewrite、find-config、rewrite、post-rewrite、preaccess、access、post-access、try-files、content 以及 log.

## 1. post-read

1. 支持模块注册处理程序
2. 发生阶段:nginx读取并解析完请求头之后立即开始执行 
3. 示例模块：标准模块 ngx_realip，目的：改写请求的来源地址。
4. 举例

	    server {
	        listen 8080;
	
	        set_real_ip_from 127.0.0.1; # 规定来源地址的改写只对那些来自127.0.0.1请求生效
	        real_ip_header   X-My-IP;
	
	        location /test {
	            set $addr $remote_addr;
	            echo "from: $addr";
	        }
    	}

	结果
	
		$curl -H 'X-My-IP: 1.2.3.4' your.domain/test
		from: 1.2.3.4


## 2. server-rewrite

1. 支持模块注册处理程序

## 3. find-config

1. 不支持模块注册处理程序
2. 目的：由nginx核心完成当前请求与location配置块之间的配对工作。在此阶段之前，请求没有与任何location配置块相关联。

## 4. rewrite
1. 支持模块注册处理程序
2. 目的：对当前请求进行各种修改，比如URI，URL，或者创建并初始化一系列后续处理阶段所需要的变量


## 5. post-rewrite
location级别重写的后一阶段，用来检查上阶段是否有uri重写，并根据结果跳转到合适的阶段；

1. 不支持模块注册处理程序
2. 目的：nginx核心完成rewrite阶段所要求的“内部跳转”操作。“内部跳转”本质上其实是把当前请求的处理阶段倒退到find-config阶段，以便重新进行请求URI与location配置块的配对。 
3. 注意：有趣的地方是，倒退回find-config阶段的动作并不是发生在rewrite阶段，而是发生在后面的POST-REWRITE阶段。rewrite指令只是简单地指示Nginx有必要在POST-REWRITE阶段发起“内部跳转”。这个设计的目的是：为了在最初匹配的location块中支持多次反复地改写URI。

	    location /foo {
        rewrite ^ /bar;
        rewrite ^ /baz;

        echo foo;
	    }
	
	    location /bar {
	        echo bar;
	    }
	
	    location /baz {
	        echo baz;
	    }
请求

		 $ curl localhost:8080/foo
	    baz
	   
	日志
	
		$ grep 'using config' logs/error.log
		   [debug] 89449#0: *1 using configuration "/foo"
		   [debug] 89449#0: *1 using configuration "/baz"
		   
4. server配置块中的rewrite指令

	server配置块中的rewrite指令对请求URI进行改写，则不会涉及”内部跳转“，因为此时URI改写发生在server-rewrite阶段，早于location配对的find-config阶段。
	
## 6. pre-access

1. 标准模块```ngx_limit_req```和```ngx_limit_zone``` 就运行在此阶段，前者控制请求的访问频度，后者限制访问的并发度。
2. 【建议】 尽量在server配置块中配置ngx_realip这样的模块



## 7. acess

1. 标准模块 ngx_access
2. 第三方模块 ngx_auth_request, access_by_lua


## 8. post-access阶段

1. 不支持模块注册处理程序
2. 目的：主要配合acess阶段实现标准ngx_http_core模块提供的配置指令satisfy的功能。在access阶段注册多个处理指令，satisfy指令可以用于控制它们彼此之间的协作方式。有两种协作方式，一种是all（与关系），一种是any（或关系）。
3. 举例：
	
	    location /test {
        satisfy all;

        deny all;
        access_by_lua 'ngx.exit(ngx.OK)';

        echo something important;
    	}
    	
    请求
    
    	satisfy all; $curl your.domain/test ----403 forbidden
    	satisfy any; $curl your.domain/test ---- something important
    	
## 9. try-files

1. 不支持模块注册处理程序
2. 目的：专门实现标准配置指令```try_files```功能
3. ```try_files``` 指令接受两个以上任意数量的参数，每个参数都指定了一个 URI. 这里假设配置了 N 个参数，则 Nginx 会在 try-files 阶段，依次把前 N-1 个参数映射为文件系统上的对象（文件或者目录），然后检查这些对象是否存在。一旦 Nginx 发现某个文件系统对象存在，就会在 try-files 阶段把当前请求的 URI 改写为该对象所对应的参数 URI（但不会包含末尾的斜杠字符，也不会发生 “内部跳转”）。如果前 N-1 个参数所对应的文件系统对象都不存在，try-files 阶段就会立即发起“内部跳转”到最后一个参数（即第 N 个参数）所指定的 URI. 
4.  ```try_files``` 指令本质上只是有条件地改写当前请求的 URI，而这里说的“条件”其实就是文件系统上的对象是否存在。当“条件”都不满足时，它就会无条件地发起一个指定的“内部跳转”。当然，除了无条件地发起“内部跳转”之外， try_files 指令还支持直接返回指定状态码的 HTTP 错误页，例如try_files /foo /bar =404;
5. 特别注意：对于```try_files```指令，通过URI末尾的斜杠字符来区分”目录“和”文件“的。

		 root /var/www/;
	
	    location /test {
	        try_files /foo /bar/ /baz;
	        echo "uri: $uri";
	    }
	
	    location /foo {
	        echo $uri;
	        echo "foo";
	    }
	
	    location /bar/ {
	 		  echo $uri;
	         echo bar;
	    }
	
	    location /baz {
	        echo $uri;
	        echo baz;
	    }
	    
case1 ：根目录下没有foo文件，bar目录，输出/baz, baz
		

	$ grep trying logs/error.log
    [debug] 3869#0: *1 trying to use file: "/foo" "/var/www/foo"
    [debug] 3869#0: *1 trying to use dir: "/bar" "/var/www/bar"
    [debug] 3869#0: *1 trying to use file: "/baz" "/var/www/baz"
    
 这里最后一条“调试信息”容易产生误解，会让人误以为 Nginx 也把最后一个参数 /baz 给映射成了文件系统对象进行检查，事实并非如此。当 try_files 指令处理到它的最后一个参数时，总是直接执行“内部跳转”，而不论其对应的文件系统对象是否存在。
 
 case2 :根目录下有foo文件，输出 uri: /foo
 
 
## 10. content

1. 目的：内容生成阶段。肩负着生成“内容”， 并输出HTTP响应的使命。echo, proxy_pass都属于该阶段。

2. 对于content阶段，有两种情况：
     	
     a. 没有使用任何content阶段的指令。那么就会执行静态资源服务模块：```ngx_index```, ```ngx_autoindex```, ```ngx_static```模块
     
     b. 如果使用了content阶段的指令，如echo，proxy_pass。
          每个location只能有一个内容处理程序content handler.有多个时content阶段指令时，只有一个能够被注册并执行。
     比如指令中有 ```echo ‘hello’;proxy_pass so.com;```到底哪个被执行是不一定的。因为在同一个阶段phase中，各个handler的执行顺序是不一定的。
     
     
## 11. log



