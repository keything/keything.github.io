---
title: PHP源码之用户代码的执行
date: 2016-09-18 13:45:57
tags: [php, php源码学习]
---
# 二.用户代码的执行

## 一切的开始：SAPI接口

SAPI（Server Application Programming Interface) 指的是PHP具体应用的编程接口。
脚本执行的开始都是以SAPI接口实现开始的。只是不同的SAPI接口实现会完成他们特定的工作。

## 开始和结束

### PHP开始执行以后会经过两个主要阶段：

+ 处理请求之前的开始阶段：又分为两个过程
	+ 第一个过程：模块初始化阶段（MINIT），在整个SAPI生命周期内只进行一次。比如``sudo /etc/init.d/php-fpm start``时。例如PHP在MINIT阶段会回调所有模块的MINIT函数，进行一些初始化工作，如注册常量，定义模块使用的类等等
	+ 第二个过程：模块激活阶段(RINIT)，该过程发生在请求阶段，在每个请求到来之前都会进行模块激活。其目的是请求到达以后PHP初始化执行脚本的基本环境，例如创建一个执行环境，包含保存PHP允许过程中变量名称和值内容的符号表，以及当前所有的函数和类的符号表。然后PHP调用所有模块的RINIT函数。
+ 处理请求之后的结束阶段
	+ 第一个过程：请求结束后停用模块（RSHUTDOWN，对应RINIT）
	+ 第二个过程：关闭模块，在SAPI生命周期结束（WEB服务器退出或者命令行脚本执行完毕退出）MSHUTDOWN，对应MINIT。

+ 处理流程图（引用TIPI图）
	![图片](http://www.php-internals.com/images/book/chapt02/02-01-01-cgi-lift-cycle.png) 

## fastcgi详细执行流程	
### 初始化：即 /etc/init.d/php-fpm start
1. 入口函数：在`sapi/fpm/fpm/fpm_main.c`中的main函数
2. 调用`cgi_sapi_module.startup`(函数指针startup的取值是`php_cgi_startup`），``php_cgi_startup``做了以下几件事情：

	+ 初始化若干全局变量。如``zuf.printf_function = php_printf``，``PHP_VERSION``等
	+ 初始化zend引擎和核心组件。php_module_startup会调用`zend_startup`(zend/zend.c)函数。``zend_startup``函数的作用就是初始化zend引擎。这里的初始化操作包括内存管理初始化、 全局使用的函数指针初始化（如前面所说的`zend_printf`等），对PHP源文件进行词法分析、语法分析、 中间代码执行的函数指针的赋值，初始化若干HashTable（比如函数表，常量表等等），为ini文件解析做准备， 为PHP源文件解析做准备，注册内置函数（如strlen、define等），注册标准常量（如`E_ALL`、TRUE、NULL等）、注册GLOBALS全局变量(`zend_register_auto_global("GLOBALS",..., php_auto_globals_create_globals)`)等。
		+ `zend_startup`->`zend_startup_builtin_functions`->`zend_register_module_ex`注册core模块，core模块的`module_number`是0。core模块中的函数分别是`zend_version`, `func_num_args`...`gc_disable`。
		+ 注册`E_ERROR`，`E_WARNING`这些常量。`zend_startup->zend_register_standard_constants`
	+ 解析php.ini：`php_init_config`函数的作用就是读取php.ini文件，设置配置参数，加载zend扩展并注册php扩展函数。
	+ 全局操作函数的初始化：
		+ `php_startup_auto_globals`(`main/php_variables.c`)函数会初始化在用户空间使用频率很高的一些全局变量，如``$_GET,$_POST,$_FILES等``。其调用的`zend_register_auto_global`函数会将这些变量名添加到`CG(auto_globals)`这个变量表。
		+ `php_startup_sapi_content_types`函数用来初始化SAPI对于不同类型内容的处理函数。这里的处理函数包括POST数据默认处理函数
	+ 初始化静态构建的模块和共享模块MINIT
		+ `php_register_internal_extensions_func`函数用来注册静态构建的模块，也就是默认加载的模块。
		+ 模块初始化会执行两个操作：1. 将这些模块注册到已注册的模块列表(`module_registry`) 2. 将每个模块中包含的注册到函数表CG(`function_table`).可以看到各个模块是按照首字母排序的（date模块的`module_number`是2， ereg模块是3，libxml是4，xsl是46，zip是47。特别是cgi-fcgi是48）
		+ 在所有的模块都注册有，PHP会马上执行模块初始化操作（`zend_startup_modules`）。它的整个过程就是依次遍历每个模块，调用每个模块的模块初始化函数（`PHP_MINIT_FUNCTION`)。
	+ 禁用函数和类 `php_disable_functions`和`php_disable_classes`

3. 补充一段 进程初始化部分
    + fpm_init
    + fpm_run中会调用fpm_children_create_initial(wp)进行worker进程（子进程）的初始化，
        + fpm_children_create_initial函数返回：0表示是子进程、1表示父进程、2表示错误。
        + fpm_run函数对于父进程不返回，执行fpm_event_loop(0); 永远进行事件循环
        + fpm_run函数对于子进程返回listen_fd
4. 此时调用`fcgi_accept_request`(fpm/fpm/fastcgi.c)。因为没有请求，因此listen_socket加锁，卡在accept处，等待请求的到来

5. 当有请求到来时，会从accept后续开始执行。

### 接受请求

在处理了文件相关的内容，PHP会调用`php_request_startup`做请求初始化操作。 请求初始化操作，除了图中显示的调用每个模块的请求初始化函数外，还做了较多的其它工作，其主要内容如下
1. 用户空间中需要用到的一些环境变化的初始化，包括服务器环境、请求数据环境等。实际用到的就是`$_POST, $_GET, $_COOKIE, $_SERVER, $_ENV, $_FILES`。
	+ 在 `php_request_startup`->`php_hash_environment`，调用`CG(auto_globals)`中的回调来处理。到了这里说个题外话， 就是在php.ini中， 可以使用variables_order来控制PHP是否生成某个大变量，已经大变量的生成顺序。关于顺序，就是说， 如果打开了auto_register_globals的情况下， 如果先处理p，后处理g，那么$_GET['a'],就会覆盖$_POST['a'];
    + php.ini设置中variables_order string
    设置了EGPCS(Environment, Get, Post, Cookie and Server)变量的解析顺序。比如，variables_order顺序设置为SP，那么PHP将会创建超级全局变量$_SERVER和$_POST，而不会创建$_ENV, $_GET, $_COOKIE。
    
	+ 和`sapi_module.default_post_reader`一样，`sapi_module.treat_data`的值也是在模块初始化时， 通过`php_startup_sapi_content_types`函数注册了默认数据处理函数为`main/php_variables.c`文件中`php_default_treat_data`函数。
	+ 以$_COOKIE为例，`php_default_treat_data`函数会对依据分隔符，将所有的cookie拆分并赋值给对应的变量。

2. 模块请求的初始化。PHP通过zend_activate_modules函数实现模块请求的初始化。会调用各个模块的RINIT函数。这儿的调用并没有按照字母排序的顺序调用模块。比如libxml,zlib,intl,mbstring...
3. 运行。
	+ php_execute_script函数包含了运行php脚本的全部过程。当一个php文件需要解析执行时，它可能会需要执行三个文件，其中包括一个前置执行文件、当前需要执行文件和一个后置文件。非当前的两个文件可以在php.ini文件通过auto_prepend_file参数和auto_append_file参数设置。如果这两个参数设置为空，则禁用对应执行文件。
	+ 对于需要解析执行的文件，通过zend_compile_file（compile_file函数）做词法分析、语法分析和中间代码生成操作，返回此文件的所有中间代码。 如果解析的文件有生成有效的中间代码，则调用zend_execute（execute函数）执行中间代码。 如果在执行过程中出现异常并且用户有定义对这些异常的处理，则调用这些异常处理函数。 在所有的操作都处理完后，PHP通过EG(return_value_ptr_ptr)返回结果

### PHP关闭请求
PHP关闭请求的过程是一个若干个关闭操作的集合，这个集合存在于php_request_shutdown函数中。 这个集合包括如下内容：

+ 调用所有通过register_shutdown_function()注册的函数。这些在关闭时调用的函数是在用户空间添加加来的。我们可以在脚本出错时调用一个统一的函数，给用户一个友好一些的页面，这个有点类似于网页中的404页面。
+ 执行所有可用的__destruct函数。 这里的析构函数包括在对象池（EG(objects_store）中的所有对象的析构函数以及EG(symbol_table)中各个元素的析构方法。
+ 将所有的输出刷出去
+ 发送HTTP应答头。也是一个输出字符串的过程，只是这个字符串可能符合某些规范
+ 遍历每个模块的关闭请求方法，执行模块的请求关闭操作，即RSHUTDOWN
+ 销毁全局变量表（PG(http_globals)）的变量。
+ 通过zend_deactivate函数，关闭词法分析器、语法分析器和中间代码执行器
+ 调用每个扩展的post-RSHUTDOWN函数。只是基本每个扩展的post_deactivate_func函数指针都是NULL
+ 关闭SAPI，通过sapi_deactivate销毁SG(sapi_headers)、SG(request_info)等的内容。
+ 关闭流的包装器、关闭流的过滤器
+ 关闭内存管理
+ 重新设置最大执行时间

### 结束

1. flush

	sapi_flush将最后的内容刷回去。调用的是sapi_module.flush。
2. 关闭zend引擎

	此时对应图中的流程，我们应该是执行每个模块的关闭模块操作。 在这里只有一个zend_hash_graceful_reverse_destroy函数将module_registry销毁了。 当然，它最终也是调用了关闭模块的方法的，其根源在于在初始化module_registry时就设置了这个hash表析构时调用ZEND_MODULE_DTOR宏。 而ZEND_MODULE_DTOR宏对应的是module_destructor函数。 在此函数中会调用模块的module_shutdown_func方法，即PHP_RSHUTDOWN_FUNCTION宏产生的那个函数。

	在关闭所有的模块后，PHP继续销毁全局函数表，销毁全局类表、销售全局变量表等。 通过zend_shutdown_extensions遍历zend_extensions所有元素，调用每个扩展的shutdown函数。

【todo】目前需要看的地方有： 

1. redismemcache的持久化连接是如何处理的 
2. 全局变量表这些如何做的呢 
3. 最后关闭这儿是如何处理的呢，还没有详细看代码
4. opcode：一个代码构造出来的完整opcode是什么样子呢
【todo】opcode这部分还是没有理解请求
 

	

# 2. 深入理解PHP内核之SAPI概述（讲述FASTCGI）

在各个服务器抽象层之间遵守着相同的约定，这里我们称之为SAPI接口。 每个SAPI实现都是一个_sapi_module_struct结构体变量。（SAPI接口）。 在PHP的源码中，当需要调用服务器相关信息时，全部通过SAPI接口中对应方法调用实现， 而这对应的方法在各个服务器抽象层实现时都会有各自的实现。如下图所示，为SAPI的简单示意图（引用TIPI图）

![SAPI简单示意图](http://www.php-internals.com/images/book/chapt02/02-02-01-sapi.png)

因为平时使用fastcgi，因此这儿参考TIPI中讲述apache2的方式来讲讲``fastcgi``。
它的启动方法如下：

	cgi_sapi_module.startup(&cgi_sapi_module) // fastcgi模式 sapi/fpm/fpm/fpm_main.c
	
这儿的cgi_sapi_module.startup是sapi_module_struct结构体的静态变量。静态变量的详细解释如下（引用TIPI内容）：

	struct _sapi_module_struct {
    char *name;         //  名字（标识用）
    char *pretty_name;  //  更好理解的名字（自己翻译的）
 
    int (*startup)(struct _sapi_module_struct *sapi_module);    //  启动函数
    int (*shutdown)(struct _sapi_module_struct *sapi_module);   //  关闭方法
 
    int (*activate)(TSRMLS_D);  // 激活
    int (*deactivate)(TSRMLS_D);    //  停用
 
    int (*ub_write)(const char *str, unsigned int str_length TSRMLS_DC);
     //  不缓存的写操作(unbuffered write)
    void (*flush)(void *server_context);    //  flush
    struct stat *(*get_stat)(TSRMLS_D);     //  get uid
    char *(*getenv)(char *name, size_t name_len TSRMLS_DC); //  getenv
 
    void (*sapi_error)(int type, const char *error_msg, ...);   /* error handler */
 
    int (*header_handler)(sapi_header_struct *sapi_header, sapi_header_op_enum op,
        sapi_headers_struct *sapi_headers TSRMLS_DC);   /* header handler */
 
     /* send headers handler */
    int (*send_headers)(sapi_headers_struct *sapi_headers TSRMLS_DC);
 
    void (*send_header)(sapi_header_struct *sapi_header,
            void *server_context TSRMLS_DC);   /* send header handler */
 
    int (*read_post)(char *buffer, uint count_bytes TSRMLS_DC); /* read POST data */
    char *(*read_cookies)(TSRMLS_D);    /* read Cookies */
 
    /* register server variables */
    void (*register_server_variables)(zval *track_vars_array TSRMLS_DC);
 
    void (*log_message)(char *message);     /* Log message */
    time_t (*get_request_time)(TSRMLS_D);   /* Request Time */
    void (*terminate_process)(TSRMLS_D);    /* Child Terminate */
 
    char *php_ini_path_override;    //  覆盖的ini路径
 
    ...
    ...
	};
其中一些函数指针的说明如下：

+ startup：当sapi初始化时，首先会调用该函数。 startup函数只在父进程中创建一次，在其fork的子进程中不会调用。在fastcgi的
+ activate：在每个请求开始时调用，他会再次初始化每个请求前的数据结构。


其中fastcgi的sapi_module_struct的定义在fpm/fpm/fpm_main.c中，定义如下：

	static sapi_module_struct cgi_sapi_module = {
	"fpm-fcgi",						/* name */
	"FPM/FastCGI",					/* pretty name */

	php_cgi_startup,				/* startup */
	php_module_shutdown_wrapper,	/* shutdown */

	sapi_cgi_activate,				/* activate */
	sapi_cgi_deactivate,			/* deactivate */

	sapi_cgibin_ub_write,			/* unbuffered write */
	sapi_cgibin_flush,				/* flush */
	NULL,							/* get uid */
	sapi_cgibin_getenv,				/* getenv */

	php_error,						/* error handler */

	NULL,							/* header handler */
	sapi_cgi_send_headers,			/* send headers handler */
	NULL,							/* send header handler */

	sapi_cgi_read_post,				/* read POST data */
	sapi_cgi_read_cookies,			/* read Cookies */

	sapi_cgi_register_variables,	/* register server variables */
	sapi_cgi_log_message,			/* Log message */
	NULL,							/* Get request time */
	NULL,							/* Child terminate */

	STANDARD_SAPI_MODULE_PROPERTIES
	};


## 参考文章 

[SAPI概述](http://www.php-internals.com/book/?p=chapt02/02-02-00-overview)

[fastcgi概述](https://mengkang.net/668.html)


