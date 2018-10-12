---
title: PHP函数之匿名函数
date: 2016-09-21 20:06:54
tags: [php, php源码学习]
---
## 匿名函数与闭包的关系

匿名函数是一类不需要指定指示符，而又可以被调用的函数或子例程，匿名函数可以方便的作为参数传递给其他函数， 最常见应用是作为回调函数。

闭包是词法闭包（Lexical Closure）的简称，是引用了自由变量的函数， 这个被引用的自由变量将和这个函数一同存在，即使离开了创建它的环境也一样，所以闭包也可认为是有函数和与其相关引用组合而成的实体。 

闭包和匿名函数很容易混用，但其实这是两个不同的概念。

当然在PHP中，一个观点就是`Anonymous Functions are Closures without a name`，即匿名函数是没有名字的闭包。



## PHP中匿名函数基本用法
在PHP中匿名函数(Anonymous functions)也叫作闭包函数(closures)，允许临时创建一个没有指定名称的函数。

1. 使用use关键字从父作用域继承变量。继承的变量的取值是在函数定义时的值，而不是被调用的时候的值。因此下面的代码输出是`hello hello`。

		<?php
		$helo = 'hello';
		
		$func = function() use ($helo) {
		    echo $helo . "\n";
		};
		$func();
		
		$helo = 'world';
		$func();
		
2. 当把use后面的变量改为引用以后，那么就可以改变了，输出 `hello world`

		<?php
		$helo = 'hello';
		
		$func = function() use (&$helo) {
		    echo $helo . "\n";
		};
		$func();
		
		$helo = 'world';
		$func();

	


## PHP中匿名函数
### create_function

PHP5.3中才开始正式支持匿名函数，在此之前可以通过create_function函数来创建`匿名函数`。

通过create_function函数创建的`匿名函数`并不是真的没有名字，而是生成了一个`\0lambda_xxx`的函数名，并且会在`EG(function_table)`中注册。因为函数名的第一个字符是`\0`，这样的函数名用户是不会出现的。

		#define LAMBDA_TEMP_FUNCNAME    "__lambda_func"
		 
		ZEND_FUNCTION(create_function)
		{
		    // ... 省去无关代码
		    function_name = (char *) emalloc(sizeof("0lambda_")+MAX_LENGTH_OF_LONG);
		    function_name[0] = '\0';  // <--- 这里
		    do {
		        function_name_length = 1 + sprintf(function_name + 1, "lambda_%d", ++EG(lambda_count));
		    } while (zend_hash_add(EG(function_table), function_name, function_name_length+1, &new_function, sizeof(zend_function), NULL)==FAILURE);
		    zend_hash_del(EG(function_table), LAMBDA_TEMP_FUNCNAME, sizeof(LAMBDA_TEMP_FUNCNAME));
		    RETURN_STRINGL(function_name, function_name_length, 0);
		}

通过create_function创建`匿名函数`有一些缺点：

1. 函数的定义是通过将函数拼接为一个字符串来处理的。[深入理解php之匿名函数解释](http://www.laruence.com/2010/06/20/1602.html)，这样的话就无法进行基本的语法检查
2. 这种函数与普通函数没有本质区别，无法实现闭包的效果

### 真正的匿名函数

1. ``__invoke``魔术方法
	这个魔术方法被调用的时机是：当一个对象当做函数调用的时候。通过`__invoke`方法可以把一个对象当做函数调用。
	
2. 匿名函数的实现

	匿名函数的实现就是通过``__invoke``方式实现的。匿名函数是一个普通的类，类名是[Closure](http://php.net/manual/zh/class.closure.php)。

3. 闭包的使用

	PHP使用闭包（Closure）来实现匿名函数。匿名函数在定义的时候如果需要使用作用域外的变量需要使用关键字`use`。

	特别注意的是，在PHP中是通过`拷贝`的方式将上层变量传入匿名函数的。如果需要改变上层变量的值则需要通过引用的方式传递。


	通过下面的代码看一下func的内容

		<?php
		$b = 1;
		$func = function($a) use($b) {
		    $b = $b+1;
		    echo $b . "\n";
		};
		var_dump($func);
	可以看到

		object(Closure)#1 (2) {
		  ["static"]=>
		  array(1) {
		    ["b"]=>
		    int(1)
		  }
		  ["parameter"]=>
		  array(1) {
		    ["$a"]=>
		    string(10) "<required>"
		  }
		}
	
	将代码中的`use ($b)` 改为`use (&$b)`，可以看到输出是：

		object(Closure)#1 (2) {
		  ["static"]=>
		  array(1) {
		    ["b"]=>
		    &int(1)
		  }
		  ["parameter"]=>
		  array(1) {
		    ["$a"]=>
		    string(10) "<required>"
		  }
		}
	
4. 闭包的实现

	匿名函数是通过闭包来实现的，现在我们来看看闭包（类）是怎么实现的。匿名函数和普通函数除了是否有变量名以外并没有区别，闭包的实现代码在`Zend/zend_closure.c`中。

	使用关键字use引入的父作用域的变量会作为闭包类的静态变量，从上面的var_dump中也可以看出。

	在``ZEND_DECLARE_LAMBDA_FUNCTION_SPEC_CONST_CONST_HANDLER``中调用`zend_create_closure`创建一个闭包对象。在`zend_create_closure`中会将`zval_copy_static_var`作为回调函数传递给`zend_hash_apply_with_arguments`。每次读取到hash表中的值以后都会由这个函数进行处理，而这个函数对所有use语句定义的变量值赋值给这个匿名函数的静态变量， 这样匿名函数就能访问到use的变量了。


##参考文章

1. [Anonymous Functions VS Closures](http://dhorrigan.com/post/29209695084/anonymous-functions-vs-closures)
2. [匿名函数及闭包](http://www.php-internals.com/book/?p=chapt04/04-04-anonymous-function)
3. [深入理解PHP之匿名函数](http://www.laruence.com/2010/06/20/1602.html)
4. [PHP匿名函数基本用法](http://php.net/manual/zh/functions.anonymous.php)
