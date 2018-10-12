---
title: log4cplus介绍
date: 2017-01-02 22:57:32
tags: cpp
---

## 概述

最近在看C++，比较难懂的一个c++的库是log4cplus，一直没有找到合适的文档。元旦看了一下老大写的基于log4cplus的库，方才对log4cplus有了一个直观的印象。

首先说一下我的几个困惑：
1. 版本问题，目前[官方wiki](https://sourceforge.net/p/log4cplus/wiki/Home/), [例子](https://github.com/log4cplus/log4cplus/blob/master/docs/examples.md) 介绍的是版本2.0。而我们在使用多是1.2版本。 

2. 代码问题。在搜到的博客中，经常见到的是自己定义append,layout等等，代码非常长。老大写的代码只有很少的代码。
经过查证，log4cplus有两种方式，一种是代码中写appender,layout,一种是通过配置。

## 介绍

有关log4cplus的配置有一篇不错的文章。 [log4cplus介绍](http://blog.csdn.net/fksec/article/details/41546189)

log4cplus的组成：

+ 1. logger： 一个应用程序可以使用多个logger，logger是我们打印log的句柄

+ 2. appender：一个logger可以拥有多个appender，appender决定了log的输出方向

+ 3. filter：一个appender可以有多个filter，在配置文件中设置过滤条件

+ 4. layout：支持多种layout，一个appender一个layout



## 举例

demo.cpp 


    // demo.cpp
    #include <log4cplus/logger.h>
    #include <log4cplus/configurator.h>
    #include <log4cplus/helpers/loglog.h>
    #include <log4cplus/helpers/stringhelper.h>
    #include <log4cplus/helpers/sleep.h>
    #include <log4cplus/loggingmacros.h>
    #include <iostream>

    using namespace log4cplus;
    using namespace log4cplus::helpers;


    Logger log_1 =  Logger::getInstance(LOG4CPLUS_TEXT("test.log_1"));
    Logger log_2 =  Logger::getInstance(LOG4CPLUS_TEXT("test.log_2"));
    Logger log_3 =  Logger::getInstance(LOG4CPLUS_TEXT("test.log_3"));


    void
    printMsgs(Logger& logger)
    {
        LOG4CPLUS_TRACE_METHOD(logger, LOG4CPLUS_TEXT("printMsgs()"));
        //LOG4CPLUS_DEBUG(logger, "printMsgs()");
        LOG4CPLUS_INFO(logger, "printMsgs()");
        //LOG4CPLUS_WARN(logger, "printMsgs()");
        //LOG4CPLUS_ERROR(logger, "printMsgs()");
        LOG4CPLUS_ERROR_FMT(logger, "printMsgs(%s)", "helloworld");
    }



    int
    main()
    {
        std::cout<< LOG4CPLUS_TEXT("Entering main()...") << std::endl;
        log4cplus::initialize();
        LogLog::getLogLog()->setInternalDebugging(true);
        Logger root = Logger::getRoot();
        try 
        {
            ConfigureAndWatchThread configureThread(
                LOG4CPLUS_TEXT("log4cplus.properties"), 5 * 1000);

            LOG4CPLUS_WARN(root, "Testing....");

            for(int i=0; i<4; ++i) {
                //printMsgs(log_1);
                printMsgs(log_2);
                //printMsgs(log_3);
                log4cplus::helpers::sleep(1);
            }
        }
        catch(...) {
            std::cout<< LOG4CPLUS_TEXT("Exception...") << std::endl;
            LOG4CPLUS_FATAL(root, "Exception occured...");
        }

        std::cout<< LOG4CPLUS_TEXT("Exiting main()...") << std::endl;
        return 0;
    }

配置文件

    log4cplus.rootLogger= WARN, M
    log4cplus.logger.test.log_1=WARN
    log4cplus.logger.test.log_2=WARN
    log4cplus.logger.test.log_3=WARN

    log4cplus.appender.TT=log4cplus::ConsoleAppender
    log4cplus.appender.TT.layout=log4cplus::PatternLayout
    log4cplus.appender.TT.layout.ConversionPattern=%d{%m/%d/%y %H:%M:%S} [%t] %-5p %c{2} %%%x%% - %m [%l]%n

    log4cplus.appender.M=log4cplus::DailyRollingFileAppender
    log4cplus.appender.M.Schedule=MINUTELY
    log4cplus.appender.M.filters.1.LogLevelToMatch=DEBUG
    log4cplus.appender.M.filters.1.AcceptOnMatch=TRUE
    log4cplus.appender.M.filters.2=log4cplus::spi::DenyAllFilter
    log4cplus.appender.M.File=output.log
    #log4cplus.appender.M.MaxFileSize=5MB
    log4cplus.appender.M.MaxBackupIndex=1
    log4cplus.appender.M.layout=log4cplus::PatternLayout
    log4cplus.appender.M.layout.ConversionPattern=%d{%m/%d/%y %H:%M:%S} [%t] %-5p %c{2} %%%x%% - %m [%l]%n

编译

    g++ -llog4cplus -lpthread -o test log4cplus.demo.cpp

## 参考文章

[log4cplus学习速记](http://blog.csdn.net/cchd0001/article/details/49928421)
[log4cplush学习](http://blog.csdn.net/skyztttt/article/details/51492935)



安装

 ./configure --enable-static --prefix=/Users/keything/log4cplus_static CXXFLAGS="-std=c++0x"

因为如果需要log4cplus支持C++11，必须在g++编译的时候需要加选项“-std=c++0x”。

