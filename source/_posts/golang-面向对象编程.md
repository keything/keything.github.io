---
title: golang-面向对象编程
date: 2019-03-02 15:08:34
tags: golang
---

理解golang面向对象


面向对象编程的三个核心是：封装、继承、多态。



1. 封装(encapsulation)

    封装就是将抽象得到的数据和行为（或功能）相结合，形成一个有机的整体，也就是将数据与操作数据的源代码进行有机的结合，形成“类”，其中数据和函数都是类的成员。

    封装的目的是增强安全性和简化编程，使用者不必了解具体的实现细节，而只是要通过 外部接口，一特定的访问权限来使用类的成员。
    即不直接暴露数据，而暴露的是接口.

    go封装是package层面的。小写开头的Names只能在包内可见。在一个private package可以隐藏所有东西，并只暴露特定类型、接口、工厂函数。


2. 继承

    继承是指一个对象直接使用另一对象的属性和方法。事实上，我们遇到的很多实体都有继承的含义。例如，若把汽车看成一个实体，它可以分成多个子实体，如：卡车、公共汽车等。这些子实体都具有汽车的特性，因此，汽车是它们的“父亲”，而这些子实体则是汽车的“孩子”。


    现代语言认为实现继承更好的方式是组合(composition)。go采用这种理念，并且没有任何等级内容(hierarchy)。 这允许你使用组合来共享实现的细节。go是通过嵌入(embedding)的方式来实现匿名组合的(anonymous composition)。 

    通过嵌入一个匿名类型的组合实现了继承。 嵌入的结构体等同于基类(base class)。当然了也可以嵌入一个接口，但是必须注意，嵌入一个接口时，该结构体必须实现这个接口的方法，不然会报runtime error。

    报错：panic: runtime error: invalid memory address or nil pointer dereference



3. 多态（polymorphism）

    多态是允许你将父对象设置成为和一个或更多的子对象相等的技术。赋值会后，父对象就可以根据当前赋值给它的子对象以不同的方式运作。简单来说，允许将子类类型的指针赋值给父类类型的指针。 在C++中都是通过虚函数(Virtual Function)实现的。golang允许接口的子类的多态，但不允许子类替换为父类




### 实际例子
在我们实际项目中用到多态的地方很多，举一个例子 获取下游服务的节点列表：
需求：

+ 希望支持多种方式获取节点列表，比如配置文件、服务发现、http请求等；
+ 希望通过配置获取顺序的方式来实现优先级，比如配置是`get_type=服务发现,配置文件,http请求`。那么当服务发现获取节点成功时，则使用服务发现；反之如果服务发现获取节点列表失败，则需要使用配置文件的方式。
    
抽象：

+ 定义一个接口IConfObj：
+ check函数：为了检查配置文件的配置项是否完备，因为不通获取方式，配置文件不一样；
        + run函数：执行节点获取，并执行 InterfaceAction来执行节点更新。

    + 定义一个获取节点后动作的func，目的就是在获取节点后，通过该func进行操作。         
       
                type IConfObj interface {
                	check() error
                	run() (bool, error)
                }
                
                type InterfaceAction func(hosts []string, is_high_node, is_primary bool) (bool, error)
    
    + 实现节点列表获取配置化
        + 不同的获取获取方式实现这个接口，并进行注册。
        + 根据注册先后顺序执行run，如果run成功则结束遍历，反之继续执行直至成功。



参考文章：

https://code.tutsplus.com/tutorials/lets-go-object-oriented-programming-in-golang--cms-26540
