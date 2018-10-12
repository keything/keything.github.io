---
title: php一些bug列表
date: 2016-09-07 19:46:33
tags: php
---

在读[Laruence](http://www.laruence.com/)的文章时，有很多好的文章。
有一些不会出现或者出现概率很低的文章，现在将读过的文章，自己记录一下，方便查阅

1. [PHP5.2x + APC的一个bug定位](http://www.laruence.com/2009/12/05/1172.html) 

    出现原因是：session模块和apc模块加载顺序和模块关闭顺序导致。
    模块载入顺序和模块关闭函数很有关系了. 总体来说, 就是PHP会根据模块载入的顺序的反序来在每次请求处理结束后依次调用各个扩展的请求关闭函数.

    因为我们环境的Session是静态编译进PHP的, 所以Session模块一定先于动态编译进PHP的APC被载入, 也就是说, 在请求关闭时期, APC的请求关闭函数, 一定会先于Session的请求关闭函数被调用.

    APC在模块请求关闭函数时期, 清空了执行全局标量中的类定义表EG(classs_table)，当Session的请求关闭函数调用的时候, 执行环境的Class Table已经为空, 当然也就会抛出类找不到的fatalerror了。

2. [一个低概率的PHP CoreDump](http://www.laruence.com/2008/12/31/647.html)

    出现原因：php正在出错处理函数中，这个时候php execute limit time信号到来被响应，再次载入php_error_cb函数，就会出现。

    自己尝试：在php5.5环境下复现
3. [PHP stream未能及时清理现场导致Core的bug](http://www.laruence.com/2010/09/27/1754.html)


        
