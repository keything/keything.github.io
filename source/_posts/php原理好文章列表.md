---
title: php原理-鸟哥的文章列表
tags: php
date: 2016-09-10 08:24:56
---


0. 概述

    [深入浅出PHP](http://www.laruence.com/2008/08/11/147.html)

1. 变量

    [深入理解PHP原理之变量](http://www.laruence.com/2008/08/22/412.html)
    [深入理解PHP原理之变量作用域(Scope in PHP) ](http://www.laruence.com/2008/08/26/463.html)
    [深入理解PHP原理之变量分离/引用](http://www.laruence.com/2008/09/19/520.html)
    [深入理解php内核之写时复制](http://www.php-internals.com/book/?p=chapt06/06-06-copy-on-write)
    到了引用这儿还是有点懵逼
    [PHP的GET/POST等大变量生成过程](http://www.laruence.com/2008/11/07/581.html)
    [深入理解PHP原理之变量生命期(一)](http://www.laruence.com/2009/12/26/1198.html)
    [Array dereferencing](http://www.laruence.com/2011/10/10/2212.html)
    [如何获取一个变量的名字](http://www.laruence.com/2010/12/08/1716.html)
    如何获取一个变量的名字中提到了``活动符号表``
    而在PHP中, 所有的变量都存储在称为”符号表”的HastTable结构中. 在解析执行的过程中, 依旧保留着着”符号”信息, 所以, 肯定是可以获取到的.

    [变量的使用](http://php.net/manual/zh/internals2.variables.php)

    而在PHP中, 符号的作用域是和活动符号表相关联的. 同一时间, 只有一个活动符号表.

    那么怎么理解活动符号表和符号表呢?

    对于PHP来说, 当前活动的符号表是保存在全局变量EG(active_symbol_table)中的, 而于此同时, 还有个全局符号表保存在EG(symbol_table)中, 在进入一个函数调用的执行体之前, 会生成一个新的active_symbol_table, 并且会保持一个调用栈式样的符号表栈:EG(symtable_cache), 以便在退出函数调用的时候, 恢复之前的活动符号表(作用域).

    同时在PHP中, 不能实现作用域继承, 也就是不能直接访问作用域外层的符号(需要加上golbal声明), 而如果加上global的声明的话, 也会在当前的活动作用域生成一个copy, 也就是说, 不存在在当前作用域可见的符号是保存在全局符号表的

2. 函数

    [深入理解PHP原理之函数(Introspecting PHP Function)](http://www.laruence.com/2008/08/12/164.html)
    函数分为两种zend_internal_function(对应结构体_zend_internal_function)和用户自定义函数(对应结构体_zend_op_array)。还有一个结构体zend_function.
    首先你要理解他的设计目标： zend_internal_function, zend_function,zend_op_array可以安全的互相转换(The are not identical structs, but all the elements that are in “common” they hold in common, thus the can safely be casted to each other);
    具体来说，当在op code中通过ZEND_DO_FCALL调用一个函数的时候，ZE会在函数表中，根据名字（其实是lowercase的函数名字，这也就是为什么PHP的函数名是大小写不敏感的)查找函数， 如果找到，返回一个zend_function结构的指针(仔细看这个上面的zend_function结构), 然后判断type,如果是ZEND_INTERNAL_FUNCTION， 那么ZE就调用zend_execute_internal,通过zend_internal_function.handler来执行这个函数， 如果不是，就调用zend_execute来执行这个函数包含的zend_op_array.

    【todo】现在不明白的是zend_op_array里面到底存的是什么?
    [函数类型提示(Callable typehint) ](http://www.laruence.com/2011/10/10/2229.html)
    [深入理解PHP之匿名函数](http://www.laruence.com/2010/06/20/1602.html)

3. opcode

    [深入理解PHP原理之opcodes](http://www.laruence.com/2008/06/18/221.html)
    [使用PHP embed sapi实现opcodes查看器](http://www.laruence.com/2008/09/23/539.html)
    [关于PHP的编译和执行分离](http://www.laruence.com/2012/08/16/2701.html)
    [深入理解PHP原理之异常机制](http://www.laruence.com/2010/08/03/1697.html)
    [PHP源码分析之Global关键字](http://www.laruence.com/2008/08/24/377.html)
    也就是说， 如果你global了一个变量，那么Zend就会去全局symbol_table去寻找，如果找不到，就会在全局symbol_table中分配相应的变量。通过这样的机制，实现了全局变量

4. 对象

    [深入理解PHP原理之对象(一)](http://www.laruence.com/2010/05/18/1482.html)
    [PHP5多重继承顺序的bug](http://www.laruence.com/2008/08/24/427.html)

5. 数组

    [关于一笔试题(Iterator模式)](http://www.laruence.com/2008/10/31/574.html)
    [深入理解PHP原理之foreach](http://www.laruence.com/2008/11/20/630.html)
    [PHP中的Hash算法](http://www.laruence.com/2009/07/23/994.html)
    [深入理解PHP之数组（遍历顺序)](http://www.laruence.com/2009/08/23/1065.html)

6. 扩展

    [扩展PHP(一)](http://www.laruence.com/2008/08/16/301.html)
    [关于PHP扩展开发的一些资源](http://www.laruence.com/2011/09/13/2139.html)
    [用C/C++扩展你的PHP](http://www.laruence.com/2009/04/28/719.html)
    [保证PHP扩展的依赖关系](http://www.laruence.com/2009/08/18/1042.html)
    [深入理解PHP原理之扩展载入过程](http://www.laruence.com/2009/06/14/945.html)

7. 内存管理

    [PHP原理之内存管理中难懂的几个点](http://www.laruence.com/2011/11/09/2277.html)
    [深入理解PHP内存管理之谁动了我的内存](http://www.laruence.com/2011/03/04/1894.html)

8. SAPI
    [深入理解ZendSAPIs](http://www.laruence.com/2008/08/12/180.html)
    [垃圾回收机制](http://docs.php.net/manual/zh/features.gc.php)
    [第二节 SAPI概述](http://www.php-internals.com/book/?p=chapt02/02-02-00-overview)
    [fastcgi](http://www.php-internals.com/book/?p=chapt02/02-02-03-fastcgi)
    [嵌入式sapi](http://www.php-internals.com/book/?p=chapt02/02-02-02-embedding-php)

9. 其他
    [深入理解PHP原理之错误抑制与内嵌HTML](http://www.laruence.com/2009/07/27/1020.html)
    [PHP Performance Optimization](http://www.laruence.com/2011/05/31/2018.html)
    [关于PHP浮点数你应该知道的](http://www.laruence.com/2011/12/19/2399.html)
    [PHP浮点数的一个场景问题的解答](http://www.laruence.com/2013/03/26/2884.html)
    [深入理解PHP原理之文件上传](http://www.laruence.com/2008/11/07/586.html)
    [PHP受locale影响的函数](http://www.laruence.com/2009/05/31/889.html)
    [字符编码详解](http://www.laruence.com/2009/08/22/1059.html)
    [isset与is_null的不同](http://www.laruence.com/2009/12/09/1180.html)
    isset是语句 is_null是函数，判断是否为null时可以使用===
    [深入理解ob_flush和flush的区别](http://www.laruence.com/2010/04/15/1414.html)
    [在php中使用协程实现多任务调度](http://www.laruence.com/2015/05/28/3038.html)
    [使用fsock实现异步调用php](http://www.laruence.com/2008/04/16/98.html)
    [更简单的重现PHP core调用栈](http://www.laruence.com/2011/12/06/2381.html)
    [Zend Signal in PHP5.4](http://www.laruence.com/2011/10/19/2247.html)
    [一些PHP Coding Tips](http://www.laruence.com/2011/03/24/858.html)
 
10. Dtrace
[Dtrace](http://php.net/manual/zh/features.dtrace.php)
[php骇客指南](http://php.net/manual/zh/internals2.preface.php)
