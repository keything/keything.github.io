---
title: linux硬盘是否是ssd判断
date: 2017-10-08 14:52:56
tags: linux
---


## 方法一
$ cat /sys/block/*/queue/rotational

判断cat /sys/block/*/queue/rotational的返回值（其中*为你的硬盘设备名称，例如sda等等），如果返回1则表示磁盘可旋转，那么就是HDD了；反之，如果返回0，则表示磁盘不可以旋转，那么就有可能是SSD了

这种方法有个问题，那就是/sys/block/下面不只有硬盘，还可能有别的块设备，它们都在干扰你的判断。



## 方法二

使用lsblk命令进行判断，参数-d表示显示设备名称，参数-o表示仅显示特定的列。
这种方法的优势在于它只列出了你要看的内容，结果比较简洁明了。还是那个规则，ROTA是1的表示可以旋转，反之则不能旋

$ lsblk -d -o name,rota

NAME ROTA
sda     1
sdb     1

