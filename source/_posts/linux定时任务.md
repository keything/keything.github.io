---
title: linux定时任务
date: 2016-08-17 16:11:49
tags: [linux,基础知识]
---

## 34.1. Cron

Cron是一个守护进程，根据分时日月周的组合来执行任务。
Cron假设系统是可用的。如果系统不可用，那么定时任务不会执行。为了安排one-time tasks，可以参考 at and batch.

为了执行Cron服务，``crond``服务必须执行。
可以使用命令``rpm -q vixie-cron``命令查看是否安装。可以使用命令 ``service crond status``查看状态


### 34.1.1 配置定时任务


	SHELL=/bin/bash 
	PATH=/sbin:/bin:/usr/sbin:/usr/bin 
	MAILTO=root HOME=/  
	# run-parts 
	01 * * * * root run-parts /etc/cron.hourly 
	02 4 * * * root run-parts /etc/cron.daily 
	22 4 * * 0 root run-parts /etc/cron.weekly 
	42 4 1 * * root run-parts /etc/cron.monthly
	
	
前四行用于配置定时任务执行时的环境变量。``SHELL``变量告诉系统使用哪个shell，``PATH``变量定义了执行命令的路径。``MAILTO``定义了定时任务的输出结果被email给谁，如果MAILTO=""，则不发送邮件。``HOME``变量用于home目录。

在``/etc/crontab/``文件中任务的格式如下：
	`` minute hour day month dayofweek command``
+ minute -- 从0到59的任何数字
+ hour   -- 从0到23的任何数字
+ day    -- 从0到31的任何一天
+ month  -- 从1到12的任何数字（或者月份的缩写，如jan或feb）
+ dayofweek -- 从0到7的数字，其中0或7代表星期天（或者缩写，如mon或sun）
+ command -- 要执行的命令

几种取值情况

+ ``*``：代表任何有效值
+ ``-``：指定一个数字的范围，如1-3代表整数1，2，3。
+ ``,``: 一个以逗号分隔的列表，如3,4,5,8代表四个特定的数字。
+ ``/``: 用于指定步进的值（step value）。 The value of an integer can be skipped within a range by following the range with /<integer>. For example, 0-59/2 can be used to define every other minute in the minute field. Step values can also be used with an asterisk. For instance, the value */3 can be used in the month field to run the task every third month.
	
正如在/etc/crontab文件中，run-parts脚本执行/etc/cron.hourly/，/etc/cron.daily/, /etc/cron.weekly/, and /etc/cron.monthly/目录下的文件。在这些目录下面必须是shell脚本。

如果定时任务不是每小时，每天，每周，每月，那么定时任务可以加到/ect/cron.d/目录下，这个目录下文件的格式与/etc/crontab是一样的。


除了root以外的用户，可以使用crontab工具配置定时任务。所有用户定义的crontab都存在/var/spool/cron/目录下，并且使用创建他们的用户名来执行。使用crontab -e可以编辑。


cron守护进程每分钟会检查/etc/crontab文件，/ect/cron.d/目录的变化，如果有变化则加载到内存。因此如果crontab文件改变了不需要重启守护进程。

### 34.1.2 控制访问cron权限

/etc/cron.allow和/etc/cron.deny文件用于严格控制cron进程的访问。文件格式：一行一个用户名，不需要出现空格。如果访问控制文件修改了，crond不需要重启。

root用户什么时候都可以使用cron，而不用考虑访问控制文件中的用户名
如果cron.allow文件存在，只有users列表中的用户允许访问；忽略cron.deny文件
如果cron.allow文件不存在，只有cron.deny中用户不允许访问。


### 34.1.3 开始和停止服务

使用service crond start 开启。
使用service crond stop 关闭。



## 34.2 At和Batch
cron用于规划重复的任务，at命令用于规划一次性任务（one-time task)，batch用于在系统负载小于0.8的时候执行一次性任务。


## 参考文章

1. [cron](https://www.centos.org/docs/5/html/Deployment_Guide-en-US/s1-autotasks-at-batch.html)
2. [at and batch](https://www.centos.org/docs/5/html/Deployment_Guide-en-US/s1-autotasks-at-batch.html)
