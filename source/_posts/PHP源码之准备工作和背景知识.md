---
title: PHP源码之准备工作和背景知识
date: 2016-09-18 13:37:16
tags: [php, php源码学习]
---
深入理解PHP内核--学习版
说明：[深入理解PHP内核](http://www.php-internals.com/)是非常非常好的书，当然书中内容只有你详细尝试过，走一遍代码才能理解的更深刻，这几篇文章，当做自己的一个学习笔记。

## 一、学习环境搭建 
1. php源代码： 我下载的是php5.6.25。
2. 编译环境：由于会涉及到nginx，php，mysql等等，为了快速搭建lnmp的环境，建议使用[lnmp](http://lnmp.org/)先安装所需要的各种软件包。
3. 编译语句：
	因为我是在一个全新的centos虚拟机上进行学习，因此安装目录是/usr/local/php。
	
	``'./configure'  '--prefix=/usr/local/php' '--with-config-file-path=/usr/local/php/etc' '--enable-fpm' '--with-fpm-user=www' '--with-fpm-group=www' '--with-mysql=mysqlnd' '--with-mysqli=mysqlnd' '--with-pdo-mysql=mysqlnd' '--with-iconv-dir=libiconv' '--with-freetype-dir=/usr/local/freetype' '--with-jpeg-dir' '--with-png-dir' '--with-zlib' '--with-libxml-dir=/usr' '--enable-xml' '--disable-rpath' '--enable-bcmath' '--enable-shmop' '--enable-sysvsem' '--enable-inline-optimization' '--with-curl' '--enable-mbregex' '--enable-mbstring' '--with-mcrypt' '--enable-ftp' '--with-openssl' '--with-mhash' '--enable-pcntl' '--enable-sockets' '--enable-zip' '--enable-soap' '--with-gettext' '--disable-fileinfo' '--enable-opcache' '--enable-intl' '--with-xsl' ``
	
	此时试一下php是否可以运行
	
4. php的源码目录结构

	+ build：放置一些和源码编译相关的一些文件。比如开始构建之前的buildconf脚本等文件
	+ ext 官方扩展目录，包括了绝大多数PHP的函数定义和实现，如array系列，pdo系列，spl系列等函数实现
	+ main：这里存放的是PHP最为核心的文件了，主要实现php的基本设施。
	+ zend：zend引擎的实现目录，比如脚本的词法语法解析，opcode的执行和扩展机制的实现等等
	+ pear：php扩展与应用仓库
	+ sapi：包含了各种服务器抽象层的代码，如apache的mod_php，cgi，fastcgi以及fpm等接口
	+ TSRM：
	+ tests
	+ win32


5. php源码阅读工具

	使用vim+ctags阅读
	
	+ 安装ctags: ``yum install ctags``
	+ 生成tags: `cd /your/php/source/directory  && ctags -R `
	+ 在.vimrc中添加ctags路径：``set tags+=/your/php/source/directory/tags``
	+ 使用：“使用 Ctrl+] 就可以自动跳转至定义，Ctrl+t 可以返回上一次查看位置。这样就可以快速的在代码之间“游动”了。


6. PHP源码中的常用代码

	+ 双井号(##)："##"被称为 连接符（concatenator），它是一种预处理运算符， 用来把两个语言符号(Token)组合成单个语言符号。 这里的语言符号不一定是宏的变量。并且双井号不能作为第一个或最后一个元素存在
	+ 单井号(#):"#"是一种预处理运算符，它的功能是将其后面的宏参数进行 字符串化操作 ， 简单说就是在对它所引用的宏变量通过替换后在其左右各加上一个双引号， 用比较官方的话说就是将语言符号(Token)转化为字符串
	+ 宏定义中的do-while循环
	+ `#line`预处理:用于改变当前的行号（__LINE__）和文件名（__FILE__）
	+ PHP中的全局变量宏，如PG()， EG()之类的函数，他们都是PHP中定义的宏，这系列宏主要的作用是解决线程安全所写的全局变量包裹宏。在PHP代码的其他地方也存在很多类似的宏，这些宏和PG宏一样，都是为了将线程安全进行封装，同时通过约定的 G 命名来表明这是全局的， 一般都是个缩写，因为这些全局变量在代码的各处都会使用到，这也算是减少了键盘输入。


## 参考文章

[第一章 准备工作和背景知识](http://www.php-internals.com/book/?p=chapt01/01-00-prepare-and-background)
	
