---
title:  "Unix/Linux文件系统--目录，inode，硬链接"
date:   2016-04-28 18:00:01 +0800
tags: 基础知识
---

## 1. 简介：文件和Inodes

在Unix/Linux中，文件是没有结构的一个字节序列。文件中控制数据的程序（比如mysql）会添加任何所需要的结构。linux自己并不需要知道数据库文件的内部结构，对于linux而言只需要返回字节（bytes）
### 1.1 甚至硬件设备都有文件名
Unix/Linux尽可能的将每个设备都当做一系列字节。因此包括网卡、硬盘驱动、键盘、打印机、文件在内，一切都被当做文件。

	你的电脑内存是 /dev/mem
	你的第一块硬盘是 /dev/sda
	你的终端是 /dev/tty1
	...
举例

	$ ls -li /dev/mem /dev/sda /dev/tty1
	5792 crw-r----- 1 root kmem 1, 1 Oct 13 02:30 /dev/mem
	 888 brw-rw---- 1 root disk 8, 0 Oct 13 02:30 /dev/sda
	5808 crw-rw---- 1 root tty  4, 1 Oct 13 02:31 /dev/tty1
	
在Linux中大多数的输入输出设备和目录都被看做文件。如果你有足够的权限，你可以使用它们的文件系统的名字来读取这些设备。
### 1.2 Index Nodes = inodes

文件系统的内容都不是以名字的形式存储，而是以数字的形式存储。Linux是以序号化结构来存储每个硬盘对象（disk object, 如文件或目录）的数据和信息。这个序号化的结构称之为index node或inode。

每个inode都是由唯一的inode number来标识，可以使用ls -i选项查看。

	$ ls -l -i /usr/bin/perl*
	266327 -rwxr-xr-x 2 root root 10376 Mar 18  2013 /usr/bin/perl
	266327 -rwxr-xr-x 2 root root 10376 Mar 18  2013 /usr/bin/perl5.14.2
	266331 -rwxr-xr-x 2 root root 45183 Mar 18  2013 /usr/bin/perlbug
	266328 -rwxr-xr-x 1 root root   224 Mar 18  2013 /usr/bin/perldoc

Unix的目录将文件系统的名字与inode number进行关联。正如上面例子，/usr/bin/perl与inode number 266327关联。当你到达perl程序的时候，系统将会在目录中找到perl这个名字，与perl名字相关联的inode number是266327，这个inode存有真正的数据，系统通过inode找到真正的数据。文件数据与inode number关联，而不是与名字关联。

在目录中，每个文件有自己名字和与之关联的inode number。每个文件名字可以映射到一个inode number，但是一个inode number可以有多个名字。

Inode numbers are specific to a file system inside a disk partition. Every file on a file system (in that partition) has a unique inode number. Numbering is done separately for each file system, so different disk partitions may have file system objects with the same inode numbers.

每个文件系统初始化的时候会有大量可用的inodes，可以使用```df -i```列出可用的inodes。


## 2. 文件系统示意图

大多数关于文件系统的示意图都是错误的，下面是真相。
inodes的名字（如文件、目录、设备等的名字）是在目录中存储的。目录中只存储了名字和inodenumber的关联关系。文件中数据的真正存储的硬盘空间是在inode中，而不是在目录中。目录中保存的是名字和inode number，名字不会与数据有关联。

目录中，每个名字有一个inode number，inode number指出了存储数据的硬盘空间的位置。通过```ls -i```看到名字和inode对应关系。

	$ ls -i /usr/bin/perl*
	266327 /usr/bin/perl        266329 /usr/bin/perldoc.stub
	266327 /usr/bin/perl5.14.2  266330 /usr/bin/perlivp
	266331 /usr/bin/perlbug     266331 /usr/bin/perlthanks
	266328 /usr/bin/perldoc


最重要的事情是知道名字和数据真正存储的位置是分开的。大多数示意图都会把ROOT的名字搞成命名的。这会造成对unix/linux文件和目录的误解。


	WRONG - names on things      RIGHT - names above things
	=======================      ==========================
	                                                      
	    R O O T            --->         [etc,bin,home]   <-- ROOT directory
	   /   |   \                         /    |      \
	etc   bin   home       --->  [passwd]  [ls,rm]  [abcd0001]
	 |   /   \    \                 |      /    \       |
	 |  ls   rm  abcd0001  --->     |  <data>  <data>  [.bashrc]
	 |               |              |                   |
	passwd       .bashrc   --->  <data>                <data>
	
目录是名字和数字的列表，右侧示意图以中括号的方式列出，在示意图中忽略了innode number。目录中每一个对象（文件、目录、特殊文件等）的名字与真正的存储空间是分开的。这就允许一个inode可以有多个名字和多个目录。使用相同的inode number，所有的名字指向相同的存储空间。

右侧示意图中，树中目录有自己的名字。右侧最顶层目录是ROOT目录的inode，包含了etc,bin,home..的名字列表。因为在ROOT目录上面没有目录，因此ROOT目录是没有名字的。

ROOT目录下的名字bin与之关联的inodenumber是一个目录的inodenumber，包含了在bin目录下的names的列表，names列表包含名字ls和rm。从bin目录下的ls往下走就是文件/bin/ls的数据节点了。数据节点中不会包含文件的名字，名字在文件的目录中保存。

ROOT节点没有名字，因为ROOT节点上面没有目录了。其他目录有名字是因为在这些目录上面还有目录，上层目录包含了其名字。

## 3 Inodes Manage disk blocks

每个Uninx文件或目录储存在硬盘的真正数据是由序号化的硬盘数据结构进行管理的，序号化的硬盘数据结构称为Inodes。每个文件和每个目录都分配一个inode。

Unix节点管理每个文件或目录的硬盘存储空间。inode中包含了一系列指针，指向属于那个文件或目录的硬盘块（disk blocks）。文件或目录越大，inode中需要的硬盘块指针就越多。inode还存储文件或目录的属性（权限、所有者、group、size，access/modify times, etc.) ，唯独没有文件或目录的名字。Inodes只有数字、属性和硬盘块--没有名字。名字分开存储，在目录中进行保存。

## 4. 目录节点保存所有的名字

文件有硬盘块用于包含文件数据，目录同样有硬盘块，保存的是名字和inode number的对应关系。

像大多数其他的节点，目录节点也包含了节点的属性信息（权限、所有者等等）和一个或多个硬盘块指针用来存储数据。但是，目录的硬盘块存储的不是文件数据而是目录的数据：名字和inode numbers。

## 5. 多个名字--硬连接
因为

	1. 一个唯一数字的inode管理一个文件
	2. inode中不会保存文件的名字
	3. 目录中保存文件的名字和inodenumber对应关系

因此
	
	一个unix文件可以在一个或多个目录中有多个name-and-inode对，从而有多个名字。
	
### 5.1 Link Counts count names

硬链接hardlinks：相同inode有多个名字。
```ln```命令为目录中已经存在的节点创建一个新的名字。系统在每个inode中维持一个```link count```的字段，用于对inode指向的names进行计数。```rm```命令从目录中删除一个名字（hark link），并减少link count。当一个inode的link count减为0以后，inode就没有名字了，inode就会被回收，所有与inode关联的存储和数据都被释放了。

rm命令不是删除文件，而是删除文件的名字。当所有的名字都没有了，系统会删除文件并且释放空间

## 6. Tracing Inodes in Pathnames


## 参考文章

http://teaching.idallen.com/cst8207/13w/notes/450_file_system.html
