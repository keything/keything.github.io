---
title: Laravel等框架压测
date: 2016-10-06 16:05:23
tags: laravel
---
在学习Laravel过程中，发现大家都讲到Laravel性能比较差，又了解到Laravel为了性能专门有一个Lumen框架，为了更加客观的知道各个框架的性能，现在进行测试。

1. 压测工具
    采用boom

    安装 pip install boom

2. 压测参数

    boom your.domain -c 2000 -n 100

3. 压测框架

    对CI2，YII2，Laravel5.3， Lument5.2进行压测

4. 代码及输出

    都是在TestController中输出json串
    
    {"hello":"world","hello2":"world2"}

5. 压测结果

    CI2>LUMENT5.2>YII2>LARAVEL5.3(优化过)>LARAVEL5.3(没有优化)
    开启OPCACHE以后性能有显著提高，如下图所示：

    ![PHP框架QPS对比.png](/images/PHP框架QPS对比.png)


附注：Laravel优化的语句是

    配置信息缓存 php artisan config:cache
    路由缓存 php artisan route:cache
    类映射加载优化 php artisan optimize
    自动加载优化 composer dumpautoload
