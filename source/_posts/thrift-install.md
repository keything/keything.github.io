---
title: thrift install
date: 2017-01-16 10:37:45
tags: thrift
---

## Centos6.5上安装Apache Thrift

1. 按照[Building Apache Thrift on CentOS6.5](http://thrift.apache.org/docs/install/centos)操作
2. 可能还需要安装libevent
3. 最后安装thrift的时候，我是下载了thrift-0.9.3.tar.gz 

    wget http://archive.apache.org/dist/thrift/0.9.3/thrift-0.9.3.tar.gz
    tar zxvf thrift-0.9.3.tar.gz
    cd thrift-0.9.3
    ./configure
    make
    sudo make install
4. 如果出现"error while loading shared libraries: libthrift-0.9.3.so" 错误的原因和解决办法

    sudo ldconfig
