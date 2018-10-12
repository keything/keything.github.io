---
title: Thrift：可扩展的跨语言的服务实现[论文翻译]
date: 2016-11-12 16:26:52
tags: thrift
---

本文翻译[Thrift: Scalable Cross-Language Services Implementation](http://thrift.apache.org/static/files/thrift-20070401.pdf)

有一些地方没有翻译到，就纯纯的用英文直接读咯.


## 概要

Thrift是什么呢？Thrift由Facebook开发；Thrift是一个软件库，一个代码生成工具的集合；通过这些工具可以加快高效可扩展的后端服务的开发和实现。Thrift的主要目标是：实现跨语言的高效可靠的通信。实现方式：将各个语言中需要的定制化抽象为一个通用的库。这个库是由各个语言实现的一个库。 特别地，Thrift允许开发者以一个中立语言（language-neutral)文件定义数据类型和服务接口，并且生成构建RPC客户端和服务器端的所需的所有必要的代码。 


这篇论文详细介绍了Thrift中的动机和设计思想。 这并不是作为一个研究，而是详细阐述了我们做了什么以及为什么这么做。 

## 1. 说明

一句话，单纯的LAMP框架不能够满足Facebook的需求，Facebook的文化就是选择最好的工具和实现来达到最好的性能。那么带来的问题就是需要在各个语言之间建立一个透明和高性能的桥。我们发现大多数可选系统要么限制太多，要么有性能问题。


我们的实现方案：将一个中立语言软件栈与一个代码生成引擎进行组装。
中立语言软件栈可以由许多编程语言实现。
代码生成引擎可以可以将简单接口和数据定义语言转换为客户端和服务端调用的远程库。

在一个动态系统上选择一个静态代码生成，允许我们可以创建有效的代码；这些代码不需要进行实时的类型检查。对于开发者而言，可以在一个简短的文件中定义复杂服务所需要的数据类型和接口。

令人惊讶的是竟然一个健壮的开源的解决方案，因此我们将Thrift的实现进行了开源。

评估了网络环境下，跨语言交互的挑战后，我们明确了一些关键组件：

+ types：必须有一个通用的类型系统；跨语言的时候，应用开发者不需要使用自定义的thrift数据类型或者写自己的序列化代码。也就是说，一个C++程序员能够透明的将一个STL map转换为python的字字典。不需要程序员写应用层以下的代码。 
+ transport。每个语言都需要一个通用的双向数据传输的接口。服务开发者（service developer）不需要关心特定传输是如何实现的。相同的应用代码可以跑在不同的传输上，比如TCP流，内存中数据，或者硬盘上的文件。
+ protocol：数据类型必须有某种使用传输层的方式来编解码自己。当然，应用开发者不需要关心协议这一层的实现。对于应用层代码而言，使用XML或二进制协议都是一样的。所有要确定的就是，必须以始终一样的确定的协议进行传输
+ versioning。对于健壮的服务，所包含的数据类型必须提供一个版本化自己的机制。特别的，能够在不破坏服务的情况下增加或删除对象(或者函数列表)中的字段。Section 5 details Thrift’s versioning system.
+ Processors：最后，我们生成能够处理数据流的代码来完成远程进程调用。Section 6 details the generated code and TProcessor paradigm。
+ Section 7 discusses implementation details, and Section 8 describes our conclusions.

## 2. Types

Thrift类型系统是为了：不管使用什么编程语言，都使用完全原生的类型进行开发。设计上，Thrift类型系统不会引入任何特别的动态类型或者wrapper objects。并且，不需要开发者写任何对象序列化或传输的代码。Thrift IDL（Interface Definition Language）文件以最简单的方式告诉代码生成工具，如何安全的跨语言的传输对象。

### 2.1 base types

类型系统依赖一些基础类型。考虑支持哪些类型时，我们目标是在所有编程语言中都可用的数据类型。
在Thrift中支持的基础类型是

+ bool
+ byte
+ i16: a 16-bit signed integer
+ i32: a 32-bit signed integer
+ i64: a 64-bit signed integer
+ double: a 64-bit floating point 
+ string: 一个不知道编码的文本或者二进制文件。

特别注意的是unsigned integer类型的缺失。因为在很多语言中这个类型不能明确的转换为基础类型。并且，在一些语言如python中，应用开发者会给整形变量赋值一个负值。从设计的角度来说，我们发现unsigned integers在算术运算中很少用，实际中大都用作key或者标识符。这种情况下，sign就无关紧要了。signed integers同样可以做，并且可以在需要的时候安全的转换为unsigned。

### 2.2 structs



一个Thrift的struct定义了一个对象，可以在不同语言中使用。本质上，结构体等同于面向对象语言的类。定义Thrfit结构体的基本语法类似C的结构体定义。字段可能会用一个整形来进行注释，并且有一个默认值。如果忽略的话，会有一个默认值；不过强烈建议有取值，这对于稍后的版本有很大用处。 


### 2.3 Containers

Thrift中的container可以映射到很多语言中的container。可以用C++模板的样式进行标记。有三种可选的类型：

+ list<type>:元素的有序列表。直接转换为STL vector，Java ArrayList, 或JS的native array
+ set<type>:元素的无序集合。转为了STL set， Java HashSet，python的set，或PHP中的字典
+ map<type1, type2>:key到value的映射。转为STL map，Java HashMap，PHP的关联数组

当提供默认值时，类型的映射并不是固定的。Container中的元素可以是任何有效的Thrift类型，包含其他结构体的containers。

    struct Example {
        1:i32 number = 10,
        2:i64 bigNumber,
        3:double decimals,
        4:string name="thrifty"
    }

在目标语言中，每个定义都会生成两种方法的类型，read和write，用于执行序列化和使用Thrift TProtocol对象传输。

### 2.4 Exception
异常在语法和功能上等同于结构体，除了使用exception关键字而不是使用struct关键字。
生成的对象继承自某个异常基类。目的是：用一种应用层开发者熟悉的代码风格。 

### 2.5 Services

使用Thrift类型定义服务service。在语义上，service的定义等同于接口的定义（或者一个纯虚抽象类）。Thrift编译器生成实现这些接口的全部功能的client和server stubs。service定义如下：

    service <name> {
        <returntype> <name>(<arguments>)
            [throw (<exceptions>)]
    }

一个例子：

    service StringCache {
        void set(1:i32 key 2:string value),
        string get(1:i32 key) throws (1:KeyNotFound knf),
        void delete(1:i32 key)
    }
    
    
注意到void是函数返回的有效类型，当然也可以返回其他已经定义的thrift类型。另外，`async`关键字可以加到void函数上，其生成的代码不需要等待服务器的响应。对于一个纯净的void函数，只有完成服务器端的操作以后才会给客户端返回响应。使用async方法调用个，客户端只保证传输层的请求成功。
也需要注意到参数列表和异常列表也可以实现为thrift structs。在notation和行为上，三种的构造是相同的。 

## 3. Transport

生成的代码使用传输层加速数据传输

### 3.1 接口

在Thrift设计中一个关键的设计选择是：从代码生成层中解耦传输层。Though Thrift is typically used on top of the TCP/IP stack with streaming sockets as the base layer of communication, there was no compelling reason to build that constraint into the system。与真实的I/O操作相比，抽象I/O层带来的性能损耗是无关紧要的。

根本上，生成的Thrift代码只需要知道如何读取和写入数据。数据的起始是无关紧要的；它可能是一个socket，一个共享内存的片段，或者本地磁盘的一个文件。Thrift传输接口支持下面的方法：

+ open： 打开传输
+ close：关闭传输
+ isOpen：标识传输是否打开
+ read：读取
+ write：写入
+ flush：

有一些接口没有列出来，这些接口用于批量读取 and optionally signaling the comple- tion of a read or write operation from the generated code.

除了上面的TTransport接口，还有一个TServerTransport接口用于接收或创建原始的transport objects。其接口如下：

+ open
+ listen
+ accept
+ close

### 3.2 实现

在任何语言中很容易实现传输层接口。如果需要的话，开发者可以实现新的传输层机制。 

#### 3.2.1 TSocket

在所有目标语言中，实现TSocket类。提供了一个TCP/IP stream socket的通用的简单的接口。

#### 3.2.2 TFileTransport

TFileTransport是硬盘文件数据流的抽象。可以将到来的thrift请求写入到硬盘文件中。硬盘数据可以从日志中重放，用于post-process或reproduction 或者过去事件的同步。

#### 3.2.3 Utilities

The Transport interface is designed to support easy extension us- ing common OOP techniques, such as composition. Some sim- ple utilites include the TBufferedTransport, which buffers the writes and reads on an underlying transport, the TFramedTransport, which transmits data with frame size headers for chunking op- timization or nonblocking operation, and the TMemoryBuffer, which allows reading and writing directly from the heap or stack memory owned by the process


## 4. Protocol

Thrift中的第二个重要的抽象是：数据结构和传输层表示的分离。 当传输数据时，Thrift加强了（enforce）一种消息结构，但是对于正在用的协议编码是不可知的。也就是，不需要关心数据是编码为XML，还是ASCII，还是二进制；只要数据支持固定的操作集合就行。


### 4.1 接口

Thrift的Protocol接口很直接。根本上它支持两件事：1. 双向的顺序消息 2. base type，containers，structs的编码。



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



Note that every write function has exactly one read counter- part, with the exception of writeFieldStop(). This is a special method that signals the end of a struct. The procedure for reading a struct is to readFieldBegin() until the stop field is encountered, and then to readStructEnd(). The generated code relies upon this call sequence to ensure that everything written by a protocol encoder can be read by a matching protocol decoder. Further note that this set of functions is by design more robust than necessary. For example, writeStructEnd() is not strictly necessary, as the end of a struct may be implied by the stop field. This method is a convenience for verbose protocols in which it is cleaner to separate these calls (e.g. a closing </struct> tag in XML).


### 4.2 structure

Thrift structures are designed to support encoding into a streaming protocol. The implementation should never need to frame or com- pute the entire data length of a structure prior to encoding it. This is critical to performance in many scenarios. Consider a long list of relatively large strings. If the protocol interface required reading or writing a list to be an atomic operation, then the implementation would need to perform a linear pass over the entire list before en- coding any data. However, if the list can be written as iteration is performed, the corresponding read may begin in parallel, theoreti- cally offering an end-to-end speedup of (kN − C), where N is the size of the list, k the cost factor associated with serializing a sin- gle element, and C is fixed offset for the delay between data being written and becoming available to read.Similarly, structs do not encode their data lengths a priori. Instead, they are encoded as a sequence of fields, with each field having a type specifier and a unique field identifier. Note that the inclusion of type specifiers allows the protocol to be safely parsed and decoded without any generated code or access to the original IDL file. Structs are terminated by a field header with a special STOP type. Because all the basic types can be read deterministically, all structs (even those containing other structs) can be read deterministically. The Thrift protocol is self-delimiting without any framing and regardless of the encoding format.In situations where streaming is unnecessary or framing is advan- tageous, it can be very simply added into the transport layer, using the TFramedTransport abstraction.



### 4.3 实现

Facebook has implemented and deployed a space-efficient binary protocol which is used by most backend services. Essentially, it writes all data in a flat binary format. Integer types are converted to network byte order, strings are prepended with their byte length, and all message and field headers are written using the primitive integer serialization constructs. String names for fields are omitted - when using generated code, field identifiers are sufficient.We decided against some extreme storage optimizations (i.e. pack- ing small integers into ASCII or using a 7-bit continuation for- mat) for the sake of simplicity and clarity in the code. These alter- ations can easily be made if and when we encounter a performance- critical use case that demands them.



## 5. Versioning

面对版本和数据定义改变，Thrift是健壮的。对于已经部署的服务，改变的阶段性健壮是必须的。系统必须支持 1. 从日志中读取旧数据 2. 接收来自过期客户端到新服务器的请求等等

### 5.1 Field Identifiers

Thrift中的版本是通过字段标识符来实现的。 最好的编程实践是清晰的制定字段标识符。 

    struct Example {        1:i32 number=10,        2:i64 bigNumber,                        
        3:double decimals,
        4:string name="thrifty"
    }



### 5.2 Isset

### 5.3 Case Analysis

### 5.4 Protocol/Transport Versioning

## 6. RPC实现

### 6.1 TProcessor

Thrift设计中最后一个核心接口是TProcessor，可能是最简单的构造了。接口如下：

    interface TProcessor {        bool process(TProtocol in, TProtocol out)            throws TException    }

这儿关键的设计观点是：我们创建的复杂系统从根本上可以看做是处理输入和输出的服务。大多数情况下，确实是一个需要处理的输入和输出系统。

### 6.2 Generated Code
当定义一个service时，我们生成一个能够处理RPC请求的TProcessor实例。基本结构如下：

     Service.thrift    => Service.cpp    interface ServiceIf    class ServiceClient : virtual ServiceIf
        TProtocol in        TProtocol out    class ServiceProcessor : TProcessor        ServiceIf handler


    ServiceHandler.cpp        class ServiceHandler : virtual ServiceIf
        
    TServer.cpp        TServer(TProcessor processor,
            TServerTransport transport,
            TTransportFactory tfactory,
            TProtocolFactory pfactory)
        serve()
        
+ 从Thrift定义文件，我们生成虚拟服务接口。
+ 生成的客户端类实现了接口，并使用两个TProtocol实例执行I/O操作。
+ 生成的processor实现了TProcessor接口
+ 生成的代码中有所有处理RPC请求的逻辑，通过process()调用来处理。
+ 用户提供一个应用接口的实现，这与生成的源码是独立的。

### 6.3 TServer

最后，Thrift 核心库提供了一个TServer的抽象。TServer对象通常如下工作：

+ 使用TServerTransport生成一个TTransport
+ 使用TTransportFactory将一个基本的transport转换为合适的应用层transport（典型的是使用TBufferedTransportFactory）
+ 使用TProtocolFactory 创建一个用于TTransport的input/output协议
+ 触发TProcessor对象的process()方法

这些层都是分开的，从而服务器代码不需要知道传输层、编码。server封装了连接处理、线程等逻辑。The only code written by the application developer lives in the definitional Thrift file and the interface implementation.

设计的TProcessor接口很通用。一个TServer没有带着一个生成的TProcessor对象。Thrift允许应用开发者写任何类型的server用于处理TProtocol对象。


## 7. 实现细节

### 7.1 Target Language

Thrift目前支持五种目标语言：C++, Java, Python, Ruby 和PHP。

### 7.2 Generated Structs
We made a conscious decision to make our generated structs as transparent as possible. All fields are publicly accessible; there are no set() and get() methods. Similarly, use of the isset object is not enforced. We do not include any FieldNotSetException construct. Developers have the option to use these fields to write more robust code, but the system is robust to the developer ignor- ing the isset construct entirely and will provide suitable default behavior in all cases.This choice was motivated by the desire to ease application devel- opment. Our stated goal is not to make developers learn a rich new library in their language of choice, but rather to generate code that allow them to work with the constructs that are most familiar in each language.We also made the read() and write() methods of the generated objects public so that the objects can be used outside of the con- text of RPC clients and servers. Thrift is a useful tool simply for generating objects that are easily serializable across programming languages.


### 7.3 RPC Method Identification

在RPC中是通过发送方法名来调用方法的。 这种方法的一个问题是需要更多的带宽。

在方法调用时，我们希望避免很多不必要的字符串比较。为了处理这个，我们生成字符串到函数指针的映射，因此通常情况下能够以常量时间完成方法调用。在C++中，我们使用一个相对深奥的语言构造：member function pointers。
    std::map<std::string,
        void (ExampleServiceProcessor::*)(int32_t,
        facebook::thrift::protocol::TProtocol*,
        facebook::thrift::protocol::TProtocol*)>
        processMap_;
        
使用这些技术，处理字符串的消耗被降到最低。



### 7.4 Servers and Multithreading


Thrift服务需要基本的多线程来处理并发请求。对于Python和Java的Thrift服务器逻辑的实现，语言自带的线程库提供了足够的支持。对于C++的实现，没有标准的多线程实时库。特别地，健壮的、轻量的、可移植的线程管理和timer类不存在。我们研究了已有的实现，有boost::thread, boost:threadpool, ACE_Thread_Manager, ACE_TImer。

boost::threads：提供了多线程的轻量、健壮的实现；但是没有提供线程管理或timer实现。
boost::threadpool:看起来不错但还是没有达到我们的目标。我们希望尽可能的限制第三方以来。因为boost::threadpool不是一个纯净的模板库，还需要实时库，因为目前还不是官方boost的一部分。


### 7.5 Thread Primitives

在命名空间facebook::thrift::concurrency中实现了Thrift线程库，有三个组件：
+ primitives
+ thread pool manager
+ timer manager

正如上面提到的，我们在由于要不要在thrift中引入其他依赖。我们决定使用boost::shared_ptr，因为这对于多线程应用是很有用的，不需要link-time或runtime库，并且它编程了C++0x标准的一部分。

我们实现了标准的Mutex和Condition类，和一个Monitor类。后者是一个mutex和condition变量的组合，有点类似Java Object的Monitor实现。

This is also sometimes referred to as a barrier. We provide a Synchronized guard class to allow Java-like synchronized blocks. This is just a bit of syntactic sugar, but, like its Java counterpart, clearly delimits critical sec- tions of code. Unlike its Java counterpart, we still have the ability to programmatically lock, unlock, block, and signal monitors.

xxxx
xxxx
xxxx



### 7.6 Thread, Runnable, and shared_ptr

### 7.7 ThreadManager

### 7.8 TimerManager

### 7.9 Nonblocking Operation

### 7.10 Compiler

### 7.11 TFileTransport





