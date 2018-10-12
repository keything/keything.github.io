---
title: 转义字符
date:   2016-01-04 22:00:01 +0800
tags: 基础知识
---
## 0.概述

转义字符标志着在一个字符序列中出现在它之后的后续几个字符是一种替代解释。

判断某字符是否为转义字符由上下文确定。转义字符标志着转义序列escape sequence开始的那个字符。

转义字符原本是指ASCII的十进制27，16进制1B，8进制033所定义的字符，对应标准键盘的ESC键，老式键盘"Ctrl+["。

后来将狭义的转义字符的含义引申开来，在各种计算机语言和协议中，标志着一个转义序列开始的那个字符，都叫做Escape character.

        在C语言中 \是转义字符。
        在Shell语言中 \033是转义字符
        在URL协议中 %是转义字符

## 1. 定义
转义序列通常有两种功能。第一个是编码一个句法上的实体，如设备命令或者无法被字母表直接表示的特殊数据。第二种功能，也叫字符引用，用于表示无法在当前上下文中被键盘录入的字符（如字符串中的回车符），或者在当前上下文中会有不期望的含义的字符（如C语言字符串中的双引号字符"，不能直接出现，必须用转义序列表示

控制字符与转义字符是两个不同的概念。转义字符不属于控制字符；控制字符也不属于转义字符。
大多数ASCII控制字符单独都具有控制功能，因此它们不是转义字符


## 2. 举例
1. shell 输出红色hello

        echo -e "\e[0;31mhello\e[0m"

2. shell 将出现WARN这一行绿色字体显示；出现ERR这一行加上红色背景色

        echo "ERRhello" > aa.txt
        echo "WARNdfajo" >> aa.txt
        cat aa.txt | sed -e 's/.*WARN.*/\x1b[0;36m&\x1b[0m/i' -e 's/.*ERR.*/\x1b[93;41m&\x1b[0m/i'

3. url 中转义字符是%。其中%后面跟一个ASCII码，可以在http://www.searchtb.com/wp-content/uploads/2012/04/ascii0.jpg 图中找到该ASCII码对应的字符

        http://www.baidu.com/helo=fw fwe&str=sfa
        该urlurlencode后对应的是
        http%3A%2F%2Fwww.baidu.com%2Fhelo%3Dfw+fwe%26str%3Dsfa
