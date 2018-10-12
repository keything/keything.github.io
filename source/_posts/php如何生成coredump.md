---
title: php如何生成coredump
date: 2016-09-07 15:33:49
tags: php
---
## PHP如何生成GDB的backtrace

使用lnmp环境时，可能会出现502。502的原因很多，其中一种是php-fpm出现了段错误，可以通过fpm日志（在php-fpm.conf中的error_log设置）查看。

### 重要
要想使用backtrace获得正确消息，编译php的时候使用参数 ``--enable-debug``

### 如何设置，才能生成coredump文件

1. 第一步设置linux，使linux能够生成core

		$ su -
		$ echo '/tmp/core-%e.%p' > /proc/sys/kernel/core_pattern
		$ echo 0 > /proc/sys/kernel/core_uses_pid
		$ ulimit -c unlimited
2. 第二步设置php-fpm，使php-fpm生成core

		$ vim /usr/local/php/etc/php-fpm.conf
	修改配置
		``rlimit_core = unlimited```
	
	重启php-fpm ``sudo /etc/init.d/php-fpm restart``

3. 确认coredump
	
	如果在php-fpm的error_log日志中看到类似下面的日志就代表生成了coredump，那么在/tmp/目录下就会有coredump文件
	``[05-Jun-2014 06:21:12] WARNING: [pool www] child 631273 exited on signal 11 (SIGSEGV - core dumped) after 20.263546 seconds from start``
	
4. 读取backtrace
	
    使用下面语法来运行gdb
	
		``gdb /usr/local/php/sbin/php-fpm /tmp/core-php-fpm.1230``
	可以运行``bt``命令获得更加详细的输出
		
		``(gdb) bt``


### 参考文章
[Generating a gdb backtrace](https://bugs.php.net/bugs-generating-backtrace.php)
[geberating core-dump for php5-fpm](https://easyengine.io/tutorials/php/core-dump-php5-fpm/)
[如何调试PHP的Core之获取基本信息](http://www.laruence.com/2011/06/23/2057.html)
