---
title: 4. PHP变量之常量
date: 2016-09-20 22:56:56
tags: [php, php源码学习]
---
首先看下常量与变量的区别，常量是在变量的zval结构的基础上添加了一额外的元素。如下所示为PHP中常量的内部结构。
常量的内部结构：

	typedef struct _zend_constant {
    zval value; /* zval结构，PHP内部变量的存储结构，在第一小节有说明 */
    int flags;  /* 常量的标记如 CONST_PERSISTENT | CONST_CS */
    char *name; /* 常量名称 */
    uint name_len;  
    int module_number;  /* 模块号 */
	} zend_constant;

在了解了常量的存储结构后，来看PHP常量的定义过程，一个例子：
	
	define('TIPI', 'Thinking In PHP Internal');

## define定义常量的过程
define是PHP的内置函数，在zend/zend_builtin_functions.c文件中定义了此函数的实现。最后会调用zend_register_constant。在zend_register_constant中将常量注册到EG(zend_constants)中。

常量zend_constant的标记flag可以有CONST_CS、CONST_PERSISTENT、CONST_CT_SUBST的取值。

CONST_PERSISTENT表示这个常量需要持久化。这里的持久化是指内存申请时的持久化。非持久常量会在请求结束RSHUTDOWN阶段时释放该常量。持久化常量只会在MSHUTDOWN阶段将内存释放。用户空间定义的常量都是非持久化的，通常扩展和内核定义的常量会设置为持久化。

CONST_ST_SUBST可以知道其表示Allocw compile-time substitution(在编译时可被替换)。在php内核中这些常量表示：TRUE、FALSE、NULL、ZEND_THREAD_SAFE和ZEND_DEBUG_BUILD五个。

## 标准常量的初始化

通过define()函数定义的常量的模块编号是PHP_USER_CONSTANT，是用户定义的常量。
对于错误报告级别E_ALL, E_WARNING等常量就不同了，是PHP内置定义的常量属于标准常量。调用是php_module_startup()->zend_startup()->zend_register_standard_constants()->zend_register_constant。

## zend_register_constant 

对于define定义常量和标准常量都会调用zend_register_constant。该函数的核心是调用了
    `zend_hash_quick_add(EG(zend_constants), name, c->name_len, chash, (void *) c, ...)`
其中EG(zend_constants)即全局变量execute_gloabls.zend_constants

即常量都是在execute_gloabls.zend_constants这个哈希表中

## 参考文章
[PHP变量之常量](http://www.php-internals.com/book/?p=chapt03/03-02-const-var)
