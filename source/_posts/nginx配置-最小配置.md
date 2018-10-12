---
title: nginx配置-最小配置
date: 2016-05-04 18:00:01 +0800
tags: nginx
---

## 前述

很多时候都会提到最小模块。对于Nginx的配置，当然也要有最小配置呢。当然下面的配置也不是最小配置，因为里面有很多配置都是有默认值的。大家可以查看一下nginx.org上的文档说明。

## 主配置文件nginx.conf详细说明

在nginx.conf中包括了事件模块events{}, http模块http{}。并没有看到预想的虚拟主机server{}。因为我们通过include host/*.conf将虚拟主机的配置放在了vhost文件夹中。类似C语言中include。

	##nginx.conf
	error_log  /usr/local/var/log/nginx/error.log  info;
	pid        /usr/local/var/run/nginx.pid;
	
	events {
	    worker_connections  1024;
	}
	
	http {
	    include       mime.types;
	    default_type  application/octet-stream;
	
	    include vhost/*.conf;
	}

## 普通HTTP虚拟主机配置

vhost/basic.conf
     basic.conf配置中必要的配置是listen，root，location /, location ~ \.php$ {}.
其中location ~\.php${}的作用是将以php结尾的文件转向FastCGI服务器。

	# vhost/basic.conf
	server {
	    listen       80 default;
	    server_name  localhost;
	    access_log  /usr/local/var/log/nginx/access.log;
	
	    charset utf8;
	
	    root   /Users/xx/Documents/Study/nginx/learn_conf;
	    location / {
	        index  index.php index.html index.htm;
	    }
	
	    error_page 404 404.php;
	    fastcgi_intercept_errors on;
	    # pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
	    location ~ \.php$ {
	        fastcgi_pass   localhost:9000;
	        fastcgi_index  index.php;
	        include        fastcgi.conf;
	    }
	}

## 加密HTTPS虚拟主机配置

加密HTTPS与普通HTTP虚拟主机的区别在于：
1）端口号后加 ssl
2）加入ssl\_certificate   ssl\_certificate_key两个指令

其他方面二者相同：比如端口号改变，server_name更改

	server {
	    listen 443 ssl;
	    server_name learn.ssl1;
	
	    ssl_certificate /Users/Documents/GitLibrary/nginx/nginx_conf/vhost/33iq.crt;
	    ssl_certificate_key /Users/Documents/GitLibrary/nginx/nginx_conf/vhost/33iq_nopass.key;
	
	    root /Users/Documents/Study/nginx/learn_ssl2;
	    location / {
	        index index.php index.html index.htm;
	    }
	
	    error_page 404 404.php;
	    fastcgi_intercept_errors on;
	    # pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
	    location ~ \.php$ {
	        fastcgi_pass   localhost:9000;
	        fastcgi_index  index.php;
	        include        fastcgi.conf;
	    }
	}
