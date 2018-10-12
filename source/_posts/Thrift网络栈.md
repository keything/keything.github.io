layout: thrift
title: Thrift network stack
date: 2016-11-12 16:29:53
tags: thrift
---

### Thrift network stack

本文翻译自[Thrift network stack](http://thrift.apache.org/docs/concepts)

Apache Thrift网络栈的简要描述


      +-------------------------------------------+
      | Server                                    |
      | (single-threaded, event-driven etc)       |
      +-------------------------------------------+
      | Processor                                 |
      | (compiler generated)                      |
      +-------------------------------------------+
      | Protocol                                  |
      | (JSON, compact etc)                       |
      +-------------------------------------------+
      | Transport                                 |
      | (raw TCP, HTTP etc)                       |
      +-------------------------------------------+

### 传输

传输层提供了读写网络的简单抽象。这使得Thrift可以与系统的底层传输解耦（如序列化/反序列化）。
下面是传输层接口暴露的一些方法：

+ open
+ close
+ read
+ write
+ flush

除了上面的传输接口外，Thrift还使用`ServerTransport`接口来接受或创建原始的传输对象。正如名字所写的，服务器端端使用`ServerTransport`为到来的连接创建一个传输对象（Transport objects)。


+ open
+ listen
+ accept
+ close


大多数支持Thrift支持的语言的可选的一些传输如下：


+ file: read/write to/from a file on disk
+ http: as the name suggests


###协议
协议的抽象定义了将内存中结构映射为有线格式（wire-format）的机制。换句话说，协议指明了数据如何使用底层的传输来进行编解码。因此协议实现控制了编码格式，并负责序列化和反序列化。协议举例，如JSON,XML,plain text, compact binary 。

下面是协议接口：

    writeMessageBegin(name, type, seq)
    writeMessageEnd()
    writeStructBegin(name)
    writeStructEnd()
    writeFieldBegin(name, type, id)
    writeFieldEnd()
    writeFieldStop()
    writeMapBegin(ktype, vtype, size)
    writeMapEnd()
    writeListBegin(etype, size)
    writeListEnd()
    writeSetBegin(etype, size)
    writeSetEnd()
    writeBool(bool)
    writeByte(byte)
    writeI16(i16)
    writeI32(i32)
    writeI64(i64)
    writeDouble(double)
    writeString(string)
    
    name, type, seq = readMessageBegin()
                      readMessageEnd()
    name = readStructBegin()
           readStructEnd()
    name, type, id = readFieldBegin()
                     readFieldEnd()
    k, v, size = readMapBegin()
                 readMapEnd()
    etype, size = readListBegin()
                  readListEnd()
    etype, size = readSetBegin()
                  readSetEnd()
    bool = readBool()
    byte = readByte()
    i16 = readI16()
    i32 = readI32()
    i64 = readI64()
    double = readDouble()
    string = readString()
    


Thrift协议是面向流的。没有必要需要任何明确的框架。举例，开始序列化的时候，不需要知道字符串的长度或列表中的个数。大多数支持Thrift语言可选的协议：

+ binary：相当简单的二进制编码--字段的长度和类型都编码为字节
+ compact：在[THRIFT-100](https://issues.apache.org/jira/browse/THRIFT-110)中描述
+ json

###处理器（Processor）

一个处理器有两个能力：从输入流读取数据，将数据写入到输出流。协议对象（protocal objects）代表输入流和输出流。处理器接口可以非常简单：

    
    interface TProcessor {
        bool process(TProtocol in, TProtocol out) throws TException
    }

特定服务处理器(service-specific)的实现是由compiler生成的。本质上处理器processor使用输入协议从wire读取数据，将处理过程委托给用户自己实现的handler，然后使用输出协议将数据写入wire。


### 服务
一个server将上面描述的所有的不同的特性聚合在一起：

+ 创建一个传输 transport
+ 为传输创建一个input/output协议
+ 基于Input/output协议创建一个处理器processor
+ 等待输入连接，并将其交给processor



### 其他文章

[Thrift Feature](http://thrift.apache.org/docs/features)

[Thrift interface description language](http://thrift.apache.org/docs/idl)

[Thrift install](http://thrift.apache.org/docs/install/)

[Thrift types](http://thrift.apache.org/docs/types)


[](http://dongxicheng.org/search-engine/thrift-guide/)
