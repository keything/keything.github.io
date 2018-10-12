---
title: linux下CPU核数查看
date: 2017-10-08 14:01:54
tags: linux
---

## 概念

1. 物理cpu

 插槽上的CPU个数，物理cpu数量等于不同physical id的个数。

 cat /proc/cpuinfo | grep 'pysical id' | sort | uniq | wc -l

2. cpu核数

一颗CPU上面能处理数据的芯片组的数量

cat /proc/cpuinfo| grep "cpu cores"| uniq

3. 逻辑cpu核数

一般来说，物理CPU个数 X cpu cores = 逻辑CPU的个数。
如果不相等的话，则表示服务器的CPU支持超线程技术。

cat /proc/cpuinfo | grep 'processor' | wc -l


4. 超线程技术
cat /proc/cpuinfo| grep "siblings"| uniq

超线程的数量 是 "siblings"/"cpu cores"

## 查看方式
$  cat /proc/cpuinfo| grep "physical id"| sort| uniq| wc -l
2
$  cat /proc/cpuinfo| grep "cpu cores"| uniq
cpu cores    : 12
$  cat /proc/cpuinfo| grep "siblings"| uniq
siblings    : 24
$ cat /proc/cpuinfo| grep "processor"| wc -l
48

## cpu核数取决因素

cpu的个数只与processor 、physical id、siblings 、cpu cores 四个参数有关，其他参数值可以不用考虑




