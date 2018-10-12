---
title: awk使用技巧
date: 2018-02-23 23:09:12
tags: linux
---

1. shell 变量传递给 awk


举例：将shell的变量shellVarm传递给awk，在awk中变量名叫awkVarm

该shell语句的目的是：如果第一列等于a的话，则打印

    shellVarm=a; echo "a b\na d\nc m" | awk -v awkVarm=${shellVarm} '$1==awkVarm {print $2}'


