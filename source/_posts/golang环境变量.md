---
title: golang环境变量
date: 2018-01-23 00:08:32
tags: golang
---

## GOROOT

GOROOT 是golang的安装路径


## GOPATH

go install/go get和 go的工具等会用到GOPATH环境变量.

GOPATH是作为编译后二进制的存放目的地和import包时的搜索路径。其实也是你的工作目录, 你可以在src下创建你自己的go源文件, 然后开始工作。

+ GOPATH之下主要包含三个目录: bin、pkg、src
+ bin目录主要存放可执行文件; pkg目录存放编译好的库文件, 主要是.a文件; src目录下主要存放go的源文件

## 注意

不要把GOPATH设置成go的安装路径

## 参考文章

http://blog.csdn.net/Alsmile/article/details/48290223

