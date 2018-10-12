---
title: 1. PHP变量的存储结构
date: 2016-09-19 19:52:36
tags: [php, php源码学习]
---

## 基础介绍
变量具有三个基本组成部分：
+ 名称
+ 类型
+ 值内容

数据类型：
从类型的维度来看，编程语言可以分为三大类：
+ 静态类型语言：比如C/Java等，类型的检查是在编译期（compile-time）确定的。
+ 动态语言类型：比如PHP，pythone等各种脚本语言，这类语言的类型在运行时确定的。
+ 无类型语言：比如汇编语言，汇编语言操作的是底层存储。

## 变量的结构
在官方的PHP实现内部，所有变量使用同一种数据结构zval来保存，而这个结构同时表示PHP中各种数据类型。它不仅仅包含变量的值，也包含变量的类型。这就是PHP弱类型的核心。

### php变量类型及存储结构

PHP是弱类型语言，但这并不表示PHP没有类型，在PHP中，存在8种变量类型，可以分为三类：
标量类型: boolean, integer, float(double), string
复合类型: array, object,
特殊类型: resource, NULL

### 变量存储结构	
	
	// Zend/zend.h
	typedef struct _zval_struct zval;
	...
	struct _zval_struct {
	    /* Variable information */
	    zvalue_value value;     /* value */
	    zend_uint refcount__gc;
	    zend_uchar type;    /* active type */
	    zend_uchar is_ref__gc;
	};
	
### 变量类型
type的值可以是IS_NULL, IS_BOOL, iS_LONG, IS_DOUBLE, IS_STRING, IS_ARRAY, IS_OBJECT, IS_RESOURCE之一。

### 变量的值存储
前面提到变量的值存储在zvalue_value结构体中，结构体定义如下：之所以是联合体是因为一个变量同时只能属于一种类型。
	
	typedef union _zvalue_value {
	    long lval;                  /* long value */
	    double dval;                /* double value */
	    struct {
	        char *val;
	        int len;
	    } str;
	    HashTable *ht;              /* hash table value */
	    zend_object_value obj;
	} zvalue_value;
	
#### 字符串string
字符串的类型标示和其他数据类型一样，不过在存储字符串时多了一个字符串长度的字段。

	struct {
		char *val;
		int len;
	}str;

#### 数组Array

数组是PHP中最常用的，也是最强大的变量类型，可以存储其他类型的数据，而且提供各种内置操作函数。数组的值存储在zvalue_value.ht字段中，是一个HashTable类型的数据。

#### 对象object

在面向对象语言中，我们能自己定义自己需要的数据类型，包括类的属性，方法等数据。而对象则是类的一个具体实现。对象有自身的状态和所能完成的操作。
PHP的对象是一种复合型的数据，使用一种zend_object_value的结构体来存放。其定义如下：

	typedef struct _zend_object_value {
    	zend_object_handle handle;  //  unsigned int类型，EG(objects_store).object_buckets的索引
    	zend_object_handlers *handlers;
	} zend_object_value;
	
PHP的对象只有在运行时才会被创建，前面的章节介绍了EG宏，这是一个全局结构体用于保存在运行时的数据。 其中就包括了用来保存所有被创建的对象的对象池，EG(objects_store)即全局变量executor_globals.objects_store，而object对象值内容的zend_object_handle域就是当前 对象在对象池中所在的索引，handlers字段则是将对象进行操作时的处理函数保存起来。 这个结构体及对象相关的类的结构_zend_class_entry，将在第五章作详细介绍。

	#define EG(v) (executor_globals.v)
	extern ZEND_API zend_executor_globals executor_globals;
	


参考文章：
1. [变量的结构和类型](http://www.php-internals.com/book/?p=chapt03/03-01-00-variables-structure)

