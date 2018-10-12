---
title: cpp_libconfig
date: 2018-05-28 10:50:46
tags: cpp-lib
---


## 参考处：
https://hyperrealm.github.io/libconfig/


## 简单用法

libconfig::Config 的方法

    #include <libconfig.h++>
    #include <iostream>

    int main() {
        try {
            libconfig::Config config;
            config.readFile("config.ini");
            const libconfig::Setting& serverConf = config.lookup("server");
            int serverPort;
            bool flag = serverConf.lookupValue("service_port", serverPort);
            std::cout << "server port " << serverPort << std::endl;
        } catch (const libconfig::ConfigException& e) {
            std::cout << "exception:" << e.what() << std::endl;
        }
    }
