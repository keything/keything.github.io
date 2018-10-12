---
title: free命令详解
date: 2017-10-08 13:15:29
tags: linux
---


在centos上使用free命令得到如下四行，其中行号1 2 3 4是我加上的

            1          total       used       free           shared    buffers     cached
            2 Mem:     132047948  121586656   10461292       2824      89148       106314244
            3 -/+ buffers/cache:   15183264  116864684
            4 Swap:      7812092      97712    7714380

第一行解释：

    total：除系统外，可以使用的内存总量；
    used：已经使用的内存总量
    free：空闲的内存总量
    shared：共享内存使用总量
    buffers:被OS buffer住的内存
    cached：被OS cached住的内存
    buffers和cached的区别是：

    + A buffer is something that has yet to be "written" to disk. 
    + A cache is something that has been "read" from the disk and stored for later use.
    + 也就是说buffer是用于存放要输出到disk（块设备）的数据的，而cache是存放从disk上读出的数据。这二者是为了提高IO性能的，并由OS管理。

第二行解释：

    输出是从操作系统（OS）来看的。也就是说，从OS的角度来看，计算机上一共有:

    132047948（缺省时free的单位为KB）物理内存，即FO[2][1]；
    在这些物理内存中有121586656（即FO[2][2]）被使用了；
    还用10461292（即FO[2][3]）是可用的；
    这里得到第一个等式：

    FO[2][1] = FO[2][2] + FO[2][3]

    FO[2][5] 是OS buffer住的内存，FO[2][6]是OS cached住的内存。

    Linux和其他成熟的操作系统（例如windows），为了提高IO read的性能，总是要多cache一些数据，这也就是为什么FO[2][6]（cached memory）比较大，而FO[2][3]比较小的原因。

第三行解释：

    是从一个应用程序的角度看系统内存的使用情况。

    对于FO[3][2]，即FO[2][2]-buffers/cache，表示一个应用程序认为系统被用掉多少内存；
    对于FO[3][3]，即FO[2][3]+buffers/cache，表示一个应用程序认为系统还有多少内存；
    因为被系统cache和buffer占用的内存可以被快速回收，所以通常FO[3][3]比FO[2][3]会大很多

    这里还用两个等式：

    FO[3][2] = FO[2][2] - FO[2][5] - FO[2][6]
    FO[3][3] = FO[2][3] + FO[2][5] + FO[2][6]





