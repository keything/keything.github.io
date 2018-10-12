---
title: nginx-rewrite详解
date:   2016-05-05 18:00:01 +0800
tags: nginx
---

### 一. 概述

```ngx_http_rewrite_module```模块通过正则表达式、return重定向和条件选择配置（conditionally select configurations）改变请求的URI。
```ngx_http_rewrite_module```模块中的指令以下面的顺序执行：

	    1. 在server{}中的指令先被执行，并且是根据配置中先后顺序依次执行
	    2. 重复的：
	        1. 根据请求URI匹配一个location
	        2. 在匹配的location中，该模块的指令按先后顺序依次执行
	        3. 如果一个请求URI被重定向，这个循环被重复，但最多不超过10次。

        
【解释】在```ngx_http_rewrite_module```初始化的时候(```ngx_http_rewrite_init```)的时候，在```SERVER_REWRITE```和```REWRITE```两个阶段注册了相同的处理函数```ngx_http_rewrite_handler```.

### 二. 指令

#### 1. break

	Syntax:break;
	Default:—
	Context:server, location, if

停止处理```ngx_http_rewrite_module```模块的指令。
如果在location里面使用，那么这个请求接下来phase的处理也都是在这个location里面。
看下面的例子

	location /proxy3 {
	    rewrite (.*) /third;
	    set $a 35;
	    break;
	    rewrite (.*) /second;
	    set $a 76;
	    echo $a;
	}
	location /second {
	    echo 'second';
	    echo $a;
	}
	location /third {
	    echo 'third';
	    echo $a;
	}
	
curl http://your.domain/proxy3 

|case|result|reason|
|---|---|---|
|有break|输出35|
|没有break|输出second 76|因为4条指令顺序执行，rewrite (.\*) /second是最后一条rewrite指令，生效。set $a 76生效。因此在POST-REWRITE阶段转到location /second|


#### 2. if

	Syntax: if (condition) {...}
	Default: ---
	Context: server, location
	
指定的condition参数被判断。如果为true，在if里面的模块指令被执行，请求进入到if指令里面的配置。在if指令里面的配置继承于上一层的配置。
condition可以是下面几种情况：

	    1. 一个变量名; 如果变量是空字符串或0，则false
	    2. 使用= !=运算符比较
	    3. 使用~(区分大小写) ~*(不区分大小写) 正则表达式包含capture，可以使用$1...$9来使用捕获的值。!~ !~*也可以使用.如果正则表达式包含} ;的话，整个正则表达式要放在单引号或双引号里面。
	    4. 检查文件是否存在 -f !-f
	    5. 检查目录是否存在 -d !-d
	    6. 检查文件，目录，软链是否存在 -e !-e
	    7. 检查是否可执行文件 -x !-x

举例：
	
	if ($http_user_agent ~ MSIE) {
   		rewrite ^(.*)$ /msie/$1 break;
	}
	
	if ($http_cookie ~* "id=([^;]+)(?:;|$)") {
	    set $id $1;
	}
	
	if ($request_method = POST) {
	    return 405;
	}
	
	if ($slow) {
	    limit_rate 10k;
	}
	
	if ($invalid_referer) {
	    return 403;
	}


#### 3. return
	Syntax: return code [text];
	        return code URL;
	        return URL;
	        
 停止处理，并将code返回给客户端。非标准code444 关闭连接并且不发送响应头。
 从版本0.8.42开始，可以指定一个URL（对于301，302，303和307）或者响应体（其他code）。一个响应体和重定向的URL可以包含变量。
 另外，URL可以指定为单个参数，这样就是302临时跳转。这样一个参数需要以http://, https://, $scheme开头。

#### 4. rewrite

	Syntax: rewrite regex replacement [flag];
	Default: ---
	Context: server, location, if

如果指定的正则表达式匹配了一个请求的URI，URI被指定的replacement改变。

配置文件中多个rewrite指令时，按照出现顺序依次执行，只执行最后一个。可以使用[flags]终止指令的处理。

如果replacement以```http:// https://``` 开头，那么处理将会停止，并重定向返回给客户端。

可选参数flag，可以是下面几个中的某一个:

	last:
	    停止处理ngx_http_rewrite_module模块的指令集，然后使用改变后的URI去查找新的location。
	    【注释】即遇到last以后，执行阶段从rewrite阶段回到了find-config阶段。
	break:
	    停止处理ngx_http_rewrite_module模块的指令集。
	    【注释】遇到break以后，执行阶段从rewrite阶段转到下一个阶段。如果是server-rewrite则转到find-config, 如果是rewrite则转到post-rewrite。
	redirect:
	    返回一个302临时跳转。当replacement不是以http://, https://开头的时候使用
	permanent:
	    返回一个301永久跳转。
	    
last其实就相当于一个新的url，对nginx进行了一次请求，需要走一遍大多数的处理过程，最重要的是会做一次find config，提供了一个可以转到其他location的配置中处理的机会，

而break则是在一个请求处理过程中将原来的url(包括uri和args)改写之后，在继续进行后面的处理，这个重写之后的请求始终都是在同一个location中处理。

#### 5. rewrite_log 
	Syntax: rewrite_log on | off;
	Default: rewrite_log off;
	Context: http,server,location,if
开启或关闭```ngx_http_rewrite_module```模块指令处理结果到error_log文件，日志级别notice.

## 三. 例子

1. echo $a被if继承。因此输出45

		   location /proxy3 {
		        echo $a;
		        set $a 35;
		        if ($a = 35) {
		            set $a 45;
		        }
		   }
2. echo $a被if继承，但if指令里面有echo指令，根据配置的基本规则，if里面的echo指令有效。因此输出43 【todo】配置的基本原则是毛线？？？

		   location /proxy3 {
		        echo $a;
		        set $a 35;
		        set $b 43;
		        if ($a = 35) {
		            set $a 45;
		            echo $b;
		        }
		   }
3. 由于if处于rewrite阶段，执行if以后依然处于rewrite阶段，因此if{}后面的模块指令依然执行。
因此输出67;

		   location /proxy3 {
		        echo $a;
		        set $a 35;
		        if ($a = 35) {
		            set $a 45;
		        }
		        set $a 67;
		   }
4.  演示rewrite

		location /proxy3 {
		    #rewrite (.*) http://your.domain/third;
		    rewrite (.*) /third;
		    set $a 67;
		    echo $a;
		}
		set $a 99;
		location /third {
		    echo $a;
		}
如果rewrite后面是/third 则输出67
如果是http:// 则输出99。
解释：
set $a 99 发生在server-rewrite阶段 
之后在find-config阶段，找到location /proxy3
在location /proxy3里面 如果是``rewrite http://...``那么直接跳转。类似 rewrite (.*) /third redirect;
如果是rewrite (.*) /third呢 则还会继续执行接下来的rewrite阶段的指令，set $a 67将会被执行。然后在post-rewrite阶段调转到location /third

5. rewrite中带有flag
  
		  location /proxy3 {
		        set $a 35;
		        if ($a = 35) {
		            set $a 73;
		            rewrite (.*) /main break;
		        }
		        rewrite (.*) /second;
		        set $a 76;
		        echo 'proxy3';
		        echo $uri;
		        echo $a;
		    }
		    location /main {
		        echo 'main';
		        echo $a;
		    }
		    location /second {
		        echo 'second';
		        echo $a;
		    }

    如果是```rewrite (.*) main break``` 。 则输出proxy3 /main 73
    
    如果是```rewrite (.*) main; break;```，则输出proxy3 /main 73
    原因是而break则是在一个请求处理过程中将原来的url(包括uri和args)改写之后，在继续进行后面的处理，这个重写之后的请求始终都是在同一个location中处理；
    
    如果是```rewrite (.*) main last```，则输出main 35
    
    如果是```rewrite (.*) main``` 不加break,last, 则rewrite (.*) /second; set $a 76执行 输出 second 76
    
    

6.  多个rewrite只有最后一个起作用

		location /proxy3 {
		    rewrite (.*) /third;
		    set $a 35;
		    if ($a = 35) {
		        rewrite (.*) /main;
		    }
		    rewrite (.*) /second; 
		    set $a 76;
		    echo $a;
		}
		
## 四. 几点事实：
1. rewrite 指令被称为 action directives. 下一层级的location不会从上一级中继承。
2. server 作用域中的 rewrite 模块指令也不会向下传递到 location 作 用域，但是这些指令会在 SERVER_REWRITE 阶段 (先于 REWRITE 阶段) 被执行.
3. rewrite 模块提供的指令的执行顺序和其在配置文件中的定义顺序一致
4. if 指令在nginx内部创建了一个无名location，if条件为真时，nginx使用这个无名location作用域的配置处理当前请求。 

5. rewrite模块
rewrite 模块是一个 phase handler， 其初始化函数 ```ngx_http_rewrite_init``` 在 SERVER REWRITE 和 REWRITE 阶段 注册了相同的处理函数``` ngx_http_rewrite_handler```。其中，SERVER REWRITE 阶段 的处理函数用于执行 server 作用域中的 rewrite 模块指令，而 REWRITE 阶 段的处理函数用于执行 location 和 if 作用域的 rewrite 模块指令。

6. POST REWRITE 是 Nginx 内部定义的阶段，通过检查请求 uri 是否被 rewrite 模块修改 (r->uri_changed)，判断是否需要使用修改后的 uri 重新开始 FIND CONFIG 以重新匹配合适的 location。比如在 location 中有配置如 rewrite ... last; 且 rewrite 成功和请求 uri 匹配成功时。



7. 指令执行顺序
 
nginx大部分模块提供的配置指令的生效顺序和其在配置文件中的顺序并没有什么关系。但有些是有关系的，比如
    
    1. http core模块中使用正则表达式的location。跟顺序有关
    2. rewrite模块中提供的if，break，return，rewrite指令

## 五. 参考文章


http://ialloc.org/posts/2015/07/28/ngx-notes-http-evil-if-1/
http://nginx.org/en/docs/http/ngx_http_rewrite_module.html
https://blog.martinfjordvald.com/2012/08/understanding-the-nginx-configuration-inheritance-model/
http://ialloc.org/posts/2013/07/18/ngx-notes-conf-parsing/
http://blog.csdn.net/brainkick/article/details/7475770


