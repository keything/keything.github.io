---
title:  "Mac 安装storm环境"
date:   2016-01-03 17:46:01 +0800
tags: mac
---

## 1. 安装
1. 获取storm-starter源代码

    git clone https://github.com/nathanmarz/storm-starter.git

2. cd storm_starter

3. mvn package

4. 使用 Intellij
    1. Open File > Import Project... and navigate to the storm-starter directory of your storm clone (e.g. ~/git/storm/examples/storm-starter).
    2. Select Import project from external model, select "Maven", and click Next.
    In the following screen, enable the checkbox Import Maven projects automatically. Leave all other values at their defaults. Click Next.
    3. Click Next on the following screen about selecting Maven projects to import.
    Select the JDK to be used by IDEA for storm-starter, then click Next.
    At the time of this writing you should use JDK 6.
    It is strongly recommended to use Sun/Oracle JDK 6 rather than OpenJDK 6.
    4. You may now optionally change the name of the project in IDEA. The default name suggested by IDEA is "storm-starter". Click Finish once you are done

## 可能出现的问题
1. org.apache.zookeeper.ClientCnxn  - Session 0x0 for server null, unexpected error, closing socket connection and attempting reconnect.
 解决方案：
  http://blog.csdn.net/myemail_sl/article/details/11074017
  设置执行参数。在Deault VM Arguments中设置-Djava.net.preferIPv4Stack=true
2. 如果出现缺少module.xml 那么需要重新导入一次项目即可。
3. java.lang.NoClassDefFoundError: backtype/storm/topology/IRichSpout....
http://blog.jobbole.com/88203/ 这篇文章给出了解释。
解决方案：在pom.xml将storm的scope由provided修改为compile即可。
