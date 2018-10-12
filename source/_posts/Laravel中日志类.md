---
title: Laravel中日志类
date: 2016-10-03 16:19:37
tags: Laravel
---

## 问题描述及改进方法
1. Laravel中原有的日志门面Log

    在Laravel中大家都知道Log，其用法是

        use Log;
        Log::info();

    但是这个存在一个问题，通过Log门面打印的日志是打印到一个Log文件中，不利于日志监控。

2. 改进的日志门面KLog

    通过添加一个自定义的日志门面KLog，将日志打印到不同的文件中，并且自动实现日志按天切割。

## 实现日志门面KLog

所谓门面facade，简单概括为以静态语法的方式调用底层类。门面的详细解释可以参考[核心概念 —— 门面（Facades）](http://laravelacademy.org/post/5820.html)

在 Laravel 应用中，门面就是一个为容器中对象提供访问方式的类。该机制原理由 Facade 类实现。Laravel 自带的门面，以及我们创建的自定义门面，都会继承自 Illuminate\Support\Facades\Facade 基类。

门面类只需要实现一个方法：getFacadeAccessor。正是 getFacadeAccessor 方法定义了从容器中解析什么，然后Facade 基类使用魔术方法 __callStatic() 从你的门面中调用解析对象。

因此门面类KLog的实现如下

    <?php
    // filepath:app/Http/Facades/KLog.php
    namespace App\Http\Facades;

    use Illuminate\Support\Facades\Facade;


    /**
     * @see \APP\Http\Logic\KLogLogic
     */
    class KLog extends Facade
    {
        protected static function getFacadeAccessor()
        {
           return 'KLog';//该方法的工作就是返回服务容器绑定类的别名
        }
    }

KLog门面继承 Facade 基类并定义了 getFacadeAccessor 方法，该方法的工作就是返回服务容器绑定类的别名，当用户引用 KLog类的任何静态方法时，Laravel 从服务容器中解析 KLog绑定，然后在解析出的对象上调用所有请求方法。`getFacadeAccessor`返回的是在config/app.php中aliases的别名。

前面提到`` 门面就是一个为容器中对象提供访问方式的类``，那么服务容器是什么呢？可以参考[核心概念-服务容器](http://laravelacademy.org/post/5805.html) 。既然提到服务容器，就必然提到[服务提供者](http://laravelacademy.org/post/5809.html)。服务提供者是Laravel应用启动的中心，你自己的应用以及所有Laravel的核心服务都是通过服务提供者启动。

KLog类的服务提供者实现如下:

    <?php
    // filepath:app/Providers/KLogProvider.php
    namespace App\Providers;

    use Illuminate\Support\ServiceProvider;

    class KLogProvider extends ServiceProvider
    {
        public function register()
        {
            $this->app->singleton('KLog', function () {
                return new \App\Http\Logic\KLogLogic();
            });
        }
    }

在register方法中，你唯一要做的事情就是绑事物到服务容器。

既然服务提供者和门面类都已经创建好，那么接下来我们需要进行配置，在``config/app.php``中进行如下配置：

a. aliases中增加KLog的配置，
b. providers中增加KLogProvider

    'aliases' => [
        ...
        'View' => Illuminate\Support\Facades\View::class,
        'KLog' => App\Http\Facades\KLog::class,

    ],
    'providers' => [
        ...
        App\Providers\RouteServiceProvider::class,
        App\Providers\KLogProvider::class,
    ],

当然我们忘了一件重要的事情，那就是KLogLogic类的实现，这才是真正门面KLog调用的底层类。

    <?php
    namespace App\Http\Logic;

    use Monolog\Logger;
    use Monolog\Handler\StreamHandler;
    use Illuminate\Log\Writer;

    class KLogLogic 
    {
        private static $writers = null;
        const MAX_FILES = 5; // 最多保留MAX_FILES天的日志

        public function getBizProc()
        {
            return $this->getLogger('biz_proc');
        }

        public function getBizProcErr()
        {
            return $this->getLogger('biz_proc_err');
        }

        private function getLogger($channel_name)
        {
            if (isset(self::$writers[$channel_name])) {
                return self::$writers[$channel_name];
            }

            $new_writer = new Writer(new Logger($channel_name));
            $log_file_path = storage_path() . '/logs/' . $channel_name . '.log';
            $new_writer->useDailyFiles($log_file_path, self::MAX_FILES);

            self::$writers[$channel_name] = $new_writer;
            return self::$writers[$channel_name];
        }
    }

搞定以后，我们如何使用KLog门面呢？很简单，notice和info后面的参数与门面Log一样，因为都是调用的Writer类的接口

    use KLog;

    KLog::getBizProcErr()->info($msg);
    KLog::getBizProcErr()->notice($msg);

如果要想添加新的日志，那么在KLogLogic中添加新的方法即可。
        



