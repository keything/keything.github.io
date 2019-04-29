---
title: golang包导入 
date: 2019-03-20 19:52:36
tags: [golang]
---

## golang包导入
go源代码是按package方式组织，再通过import引入使用。

### 工作目录

在Go中代码保持在称之为workspace的系统文件夹中。这个工作目录有三个根目录：

+ bin：包含可执行文件
+ pkg：包含不同操作系统架构的包二进制文件。
+ src：包含按包方式组织的源代码


其中bin和pkg文件夹是在调用go命令安装和编译源代码时自动生成。

必须让Go知道工作目录的位置，这样才能知道包的具体位置。 通过设置环境变量GOPATH来指定。 


### 导入包

1. $GOPATH/src/importpackage/lib/lib.go

        package lib
    
        import "fmt"
    
        func SayHello() {
        	fmt.Println("Hello,I'm in myLib :) ")
        }

2. $GOPATH/src/importpackage/app/main.go

        package main

        import "importpackage/lib"

        func main() {
        	lib.SayHello()
        }

3. 目录结构：

        .
        └── src
            └── importpackage
                ├── app
                │   └── main.go
                └── lib
                    └── lib.go

    go build -o main src/importpackage/app/main.go             

### 导入包的多种方式

+ 代码统一存储在工作目录下
+ 工作目录里面有多个包，不同包按目录组织，包下面由多个代码文件组成。
+ 导入包时按包的唯一路径进行导入，导入的包默认是必须要使用，如果不使用则编译失败，需要移除，减少不必要代码的引入，当然还有其他使用场景。默认情况下，我们使用文件名做为包名，方便理解。不同包组织不同的功能实现，方便理解。



