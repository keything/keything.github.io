---
title: 内核参数 tcp_syncookies-- 默认开启tcp_syncookies
date: 2016-07-09 21:05:09
tags: tcp
---


提问：
基于下面的信息，是否可以默认开启``/proc/sys/net/ipv4/tcp_syncookies``。缺点呢？



一种流行的服务攻击是给你的服务器发送许多（可能伪造的）的SYN包，但从不通过三次握手完成TCP请求。这样很快用尽内核中半打开队列（unp中叫 incompelete connection queue), 阻止了合法连接的完成。因为一个连接不需要完成，那么被攻击机器上就不需要资源，那么这就很容易处理和维护了。


如果设置了``tcp_syncookies``变量（需要内核使用``CONFIG_SYNCOOKIES``编译），那么在队列满的时候，内核也能够处理TCP SYN包了，这就是SYN cookie功能的关键所在。


SYN cookie完全不使用SYN队列就能工作。正常的情况，内核会发送SYN|ACK作为对SYN包的响应，但是，SYN cookie是发送一个特定的TCP 序号，这个序号是对``源地址``，``目的地址``，``源端口``，``目的端口``和``发送包的时间``的编码。一个执行SYN 洪水的攻击者拿不到这个包，也就没法响应。一个合法的连接将会包含这个序号来发送三次握手中的第三个包，服务器将会确认这个包是对合法SYN cookie的响应，并允许建立连接，即使在SYN队列中没有对应的位置了。


开启SYN cookie是一种抵御SYN洪水攻击的简单方式，只是多了一点建立cookie和确认cookie的CPU时间。相比拒绝所有到来的连接，开始SYNcookie是一种明智的选择。


## 参考文章

https://www.redhat.com/archives/rhl-devel-list/2005-January/msg00447.html
https://ckdake.com/content/2007/disadvantages-of-tcp-syn-cookies.html
http://coolshell.cn/articles/11564.html
