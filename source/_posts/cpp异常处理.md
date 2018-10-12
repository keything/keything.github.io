---
title: cpp异常处理
date: 2017-05-28 16:52:56
tags: cpp
---

在C++语言中，异常处理包括：

+ throw表达式（throw expression），异常检测部分使用throw表达式来表示它遇到了无法处理的问题；我们说throw引发了异常

+ try语句块（try block），异常处理部分使用try语句块处理异常。try语句块以关键字try开始，并以catch 子句结束；

+ 一套异常类（exception class），用于在throw表达式和相关的catch子句之间传递异常的具体信息；

标准异常

C++标准定义了一组类，用于报告标准库函数遇到的问题。这些异常类也可以在用户编写的程序中使用；分别定义在4个头文件中：

+ exception头文件定义了最通用的异常类 exception。它只报告异常的发生，不提供任何额外的信息；
+ stdexcept头文件定义了几种常见的异常类
    + exception 最常见的问题
    + runtime_error 只有在运行时才能检测中的问题
    + range_error 运行时错误，生成的结果超出了有意义的值域范围
    + ...

+ new 头文件定义了bad_alloc异常类型，将在12.1.2节介绍
+ type_info头文件中定义了bad_cast异常类型；将在19.2节介绍
