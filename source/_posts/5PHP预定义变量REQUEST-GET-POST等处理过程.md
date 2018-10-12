---
title: 5. PHP预定义变量REQUEST/GET/POST等处理过程
date: 2016-09-20 19:22:50
tags: [php, php源码学习]
---



## 1. 基础环境
+ php版本：php.5.6.25
+ php.ini中一个配置 `variables_order = "GPCS"`

## 2. 处理流程

### a. 初始化

1. 对于`_GET,_POST,_COOKIE,_SERVER, _ENV, _REQUEST, _FILES`处理
    fpm启动时会对这些预定义常量进行处理初始化，设置其回调函数。

    `php_cgi_startup`(sapi/fpm/fpm/fpm_main.c)->`php_startup_auto_globals`中进行初始化

        void php_startup_auto_globals(TSRMLS_D)
        {
            zend_register_auto_global(ZEND_STRL("_GET"), 0, php_auto_globals_create_get TSRMLS_CC);
            zend_register_auto_global(ZEND_STRL("_POST"), 0, php_auto_globals_create_post TSRMLS_CC);
            zend_register_auto_global(ZEND_STRL("_COOKIE"), 0, php_auto_globals_create_cookie TSRMLS_CC);
            zend_register_auto_global(ZEND_STRL("_SERVER"), PG(auto_globals_jit), php_auto_globals_create_server TSRMLS_CC);
            zend_register_auto_global(ZEND_STRL("_ENV"), PG(auto_globals_jit), php_auto_globals_create_env TSRMLS_CC);
            zend_register_auto_global(ZEND_STRL("_REQUEST"), PG(auto_globals_jit), php_auto_globals_create_request TSRMLS_CC);
            zend_register_auto_global(ZEND_STRL("_FILES"), 0, php_auto_globals_create_files TSRMLS_CC);
        }
        
2. 对于`GLOBALS`处理
    在`php_cgi_startup->php_module_startup->zend_startup`中会对`$GLOBALS`的处理进行初始化

    `zend_register_auto_global("GLOBALS", sizeof("GLOBALS") - 1, 1, php_auto_globals_create_globals TSRMLS_CC);`

3. `zend_register_auto_global`解释
    该函数在`Zend/zend_compile.c`中定义，主要是将这些添加到CG(auto_globals)这个全局的hash表中。


### b. 请求到来时

`php_request_startup->php_hash_environment-> zend_activate_auto_globals ->zend_hash_apply`其中`zend_activate_auto_globals `的核心是调用

	zend_hash_apply(CG(auto_globals),(apply_func_t) zend_auto_global_init TSRMLS_CC)

在``zend_auto_global_init``中会对CG(auto_globals)进行处理，执行初始化时设置的回调函数。

#### 对于回调函数的解释

首先介绍php.ini中的配置[variables_order](http://php.net/manual/en/ini.core.php#ini.variables-order)。`variables_order`目的是设置the EGPCS (Environment, Get, Post, Cookie, and Server)变量的解析顺序。比如，variables_order设置为PG，那么只会设置全局变量`$POST`和`$_GET`，并且对于`$_REQUEST`，如果`$POST['a']` 和`$_GET['a']`，那么在`$_REQUEST`中`$_GET['a']`会覆盖`$_POST['a']`的值。

明白了`variables_order`的含义，接下来就看一下回调函数。

+ 数据处理：对于GET，POST，COOKIE三者的回调函数都是`sapi_module.treate_data`。对于SERVER,ENV,FILES,REQUEST则走的其他逻辑。
+ 数据保存：得到数据以后，会经过`zend_hash_update(&EG(symbol_table), 变量名..)`存入符号表中。
	
一些细节的地方：

+ 其中`sapi_module.treate_data`的初始化是在`php_startup_sapi_content_types`中设置的， `sapi_module.treate_data = php_default_treate_data`。大多数sapi都是使用的默认的处理函数`php_default_treate_data`。

+ 在`php_default_treat_data`中，对于变量，都调用`php_register_variable_safe`来注册变量， 而`php_register_variable_safe`最终会调用`php_register_variable_ex`:

	
	  
		  PHPAPI void php_register_variable_ex(char *var, zval *val, zval *track_vars_array TSRMLS_DC)
		  {
		  		... 省略
		    for (p = var; *p; p++) {
	        if (*p == ' ' || *p == '.') {
	            *p='_';
	        } else if (*p == '[') {
	            is_array = 1;
	            ip = p;
	            *p = 0;
	            break;
	        }
	  			....以下省略
		  }
就是在`php_register_variable`的时候，会将(.)转换成（_)

## 3. 预定义变量的获取

在某个局部函数中使用类似于`$GLOBALS`变量这样的预定义变量， 如果在此函数中有改变的它们的值的话，这些变量在其它局部函数调用时会发现也会同步变化。 为什么呢？是否是这些变量存放在一个集中存储的地方？ 从PHP中间代码的执行来看，这些变量是存储在一个集中的地方：EG(symbol_table)。



在通过$获取变量时，PHP内核都会通过这些变量名区分是否为全局变量（`ZEND_FETCH_GLOBAL`）， 其调用的判断函数为`zend_is_auto_global`，这个过程是在生成中间代码过程中实现的。 如果是`ZEND_FETCH_GLOBAL`或`ZEND_FETCH_GLOBAL_LOCK`(global语句后的效果)， 则在获取获取变量表时(`zend_get_target_symbol_table`)， 直接返回EG(`symbol_table`)。则这些变量的所有操作都会在全局变量表进行。



## 4. 参考文章

说明：
	在参考文章中使用的php版本应该是5.3.x版本。我特意下载了php-5.3.0看了一下`php_hash_environment`的实现，确实是下面两篇文章所介绍的。而在php-5.6中，采用的则是先初始化`CG(auto_globals)`的方式。

1. [预定义常量](http://www.php-internals.com/book/?p=chapt03/03-03-pre-defined-variable)
2. [PHP的GET/POST等大变量生成过程](http://www.laruence.com/2008/11/07/581.html)
