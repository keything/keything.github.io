---
title: golang的import介绍
date: 2018-01-21 00:01:53
tags: golang
---

Golang使用包（package）这种语法元素来组织源码，所有语法可见性均定义在package这个级别。

### import 路径中的最后一个元素到底代表的是 目录还是包名?


### 编译时使用的是包源码还是.a

我们知道一个非main包在编译后会生成一个.a文件（在临时目录下生成，除非使用go install安装到$GOROOT或$GOPATH下，否则你看不到.a），用于后续可执行程序链接使用。


