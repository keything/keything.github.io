---
title: C++编译错误汇总
date: 2017-04-04 16:38:24
tags:
---

1. 出现 undefined reference to `typeinfo for xxxx`

在定义抽象函数的时候 virtual函数的定义有问题。

You must either provide a definition for virtual functions in your base class or declare them pure：

    class Accel {
    public:
        virtual void initialize(void) = 0;        //either pure virtual
        virtual void measure(void) = 0; 
        virtual void calibrate(void) {};          //or implementation 
        virtual const int getFlightData(byte) {};
    };
