---
title: laravel到lumen
date: 2016-10-07 17:55:38
tags: laravel
---
在Lumen中，默认不会加载Facades，默认不会注册Provider，可以在bootstrap/app.php中看到。

1. 关于Facades
    如果想使用Facades，那么修改bootstrap/app.php

        $app->withFacades();

    如果你自己定义了Facades的话，那么需要使用如下方式加载:

        $user_facades_alias = [
            App\Http\Facades\KLog::class => 'KLog',
        ];
        $app->withFacades(true, $user_facades_alias);

    当然不能忘了加载Provider，

        $app->register(App\Providers\KLogProvider::class);

2. 关于加载配置文件

    在Laravel中，我们已经习惯`config('config_file.config_item')`，但是在Lumen中服务都是按需绑定并加载的。为了与Laravel中代码尽可能保持一致，方便二者代码的迁移，因此依然在bootstrap/app.php中修改，修改如下：

        $cfg_files = [
            'database_split',
            'database',
            'didicfg',
            'errno',
            'timeout',
        ];

        foreach ($cfg_files as $cfg_file) {
            $app->configure($cfg_file);
        }

3. 关于异常处理

    我们知道在app/Exception/Handler.php中可以对特定的异常进行特别处理，对于render返回return []，laravel会自动转为json输出，而对于Lumen必须return json_encode([])才可以。

4. 目前我知道的需要改变的地方是这些，以后有的话会继续补充
