---
title: composer概述
date: 2017-05-02 15:16:50
tags: [composer,php]
---
#Composer 概述

## composer介绍
1. composer是php中的依赖管理器。通过composer声明项目所依赖的库，并通过composer安装依赖的库。
2. 依赖的声明
	
	在项目中使用composer，需要使用一个composer.json文件。该文件是Json格式。
	
	一个重要的key是require，告诉composer该项目依赖哪些package。
	
	举例：
		
		{
	    	"require": {
	        	"monolog/monolog": "1.2.*"
	    	}
		}
	
3. 系统需要

	composer需要PHP5.3.2以上的PHP版本。
	
4. 如何使用composer

	1. 在根目录创建一个composer.json文件。可以通过人工创建，也可以通过命令创建

			composer init
	2. 使用composer install进行依赖库的安装

			composer install
			
5. 自动加载Autoloading

	执行composer install以后，composer不仅会下载库，还会生成一个autoload文件，可以自动加载composer下载的库中的所有的类。为了使用它， 可以在代码中加入
	
		require __DIR__ . '/vender/autoload.php';
		
		
## 基本用法
https://getcomposer.org/doc/01-basic-usage.md

1. 安装

	使用composer.json文件进行项目的安装。这个文件描述了项目的依赖关系和可能包含的其他元信息。
	
	
	举例：
	
		{
	    	"require": {
	        	"monolog/monolog": "1.2.*"
	    	}
		}
	
	关于版本号的详细信息可以到该网址中进行查看。
	
2. 依赖安装

		composer install

3. composer.lock 锁文件

	安装完依赖后，composer会将已经安装的准确的版本列表写入composer.lock文件。
	需要将composer.lock and composer.json文件一起进行版本控制。
	
	如果没有composer.lock文件可能会造成库的版本不一致。
	
	更新
	
		composer update
		
	更新某个依赖
	
		composer update monolog/monolog [...]
4. 	Packagist

	packagist是一个主要的Composer仓库。一个composer库是一个基本的package源：一个可以获取package的地方。
	
5. 自动加载 Autoloading

	Composer生成一个vendor/autoload.php。
	
		require 'vendor/autoload.php';
		
	你还可以添加自己的代码到autoloader，通过在composer.json中添加autoload字段。如下：
	
		{
		    "autoload": {
		        "psr-4": {"Acme\\": "src/"}
		    }
		}
	
	Composer将会为Acme命名空间注册一个PSR-4的autoloader。
	
	### 这儿还需要详细阅读文档
	
## Libraries 库
https://getcomposer.org/doc/02-libraries.md

1. 该章目的：使你自己的库可以通过Composer进行安装
2. 每个项目都是package
    
    只要你有一个 composer.json 文件在目录中，那么整个目录就是一个包。当你添加一个 require 到项目中，你就是在创建一个依赖于其它库的包。你的项目和库之间唯一的区别是，你的项目是一个没有名字的包。

3. 平台软件包

    Composer 将那些已经安装在系统上，但并不是由 Composer 安装的包视为一个虚拟的平台软件包。这包括PHP本身，PHP扩展和一些系统库。

    1. php 表示用户的 PHP 版本要求，你可以对其做出限制。例如 >=5.4.0。如果需要64位版本的 PHP，你可以使用 php-64bit 进行限制
    2. hhvm 
    3. ext-<name> 可以帮你指定需要的 PHP 扩展（包括核心扩展）
    4. lib-<name> 允许对 PHP 库的版本进行限制。
    以下是可供使用的名称：curl、iconv、icu、libxml、openssl、pcre、uuid、xsl




## 命令行
https://getcomposer.org/doc/03-cli.md


## composer.json的策略
https://getcomposer.org/doc/04-schema.md

该章解释了在composer.json中用到的所有可选字段。
https://getcomposer.org/doc/04-schema.md

1. 如何更改composer的repository

	其实就是添加其中的repositories字段
	
		{
		    "name": "tmp/test",
		    "description": "hello",
		    "require": {
		        "monolog/monolog": "^1.13"
		    },
		    "authors": [
		        {
		            "name": "yankai",
		            "email": "yankai0219@126.com"
		        }
		    ],
		    "repositories": [
		        {
		            "packagist": false
		        },
		        {
		            "type": "composer",
		            "url": "http://packagist.phpcomposer.com"
		        }
		    ]

		}

## Repositories 仓库
https://getcomposer.org/doc/05-repositories.md

该章解释package和repository的概念，哪些repository是可选的，以及他们如何工作。

1. package



## 社区
https://getcomposer.org/doc/06-community.md



	
