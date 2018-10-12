---
title: 3. PHP中哈希表实现与数组的顺序遍历
date: 2016-09-20 19:57:23
tags: [php, php源码学习]
---
## PHP中哈希表实现

哈希表的相关概念可以看[TIPI中哈希表介绍](http://www.php-internals.com/book/?p=chapt03/03-01-01-hashtable)，也可以直接看算法导论中哈希表的介绍。

### 哈希表

PHP中的哈希表实现在Zend/zend_hash.c中。HashTable结构体用于保存整个哈希表需要的基本信息，Bucket结构体用于保存具体的数据内容，如下：

	typedef struct _hashtable { 
	    uint nTableSize;        // hash Bucket的大小，最小为8，以2x增长。
	    uint nTableMask;        // nTableSize-1 ， 索引取值的优化
	    uint nNumOfElements;    // hash Bucket中当前存在的元素个数，count()函数会直接返回此值 
	    ulong nNextFreeElement; // 下一个数字索引的位置
	    Bucket *pInternalPointer;   // 当前遍历的指针（foreach比for快的原因之一）pInternalPointer指向当前的内部指针的位置, 在对数组进行顺序遍历的时候, 这个指针指明了当前的元素.当在线性(顺序)遍历的时候, 就会从pListHead开始, 顺着Bucket中的pListNext/pListLast, 根据移动pInternalPointer, 来实现对所有元素的线性遍历.


	    Bucket *pListHead;          // 存储数组头元素指针
	    Bucket *pListTail;          // 存储数组尾元素指针
	    Bucket **arBuckets;         // 存储hash数组
	    dtor_func_t pDestructor;    // 在删除元素时执行的回调函数，用于资源的释放
	    zend_bool persistent;       //指出了Bucket内存分配的方式。如果persisient为TRUE，则使用操作系统本身的内存分配函数为Bucket分配内存，否则使用PHP的内存分配函数。
	    unsigned char nApplyCount; // 标记当前hash Bucket被递归访问的次数（防止多次递归）
	    zend_bool bApplyProtection;// 标记当前hash桶允许不允许多次访问，不允许时，最多只能递归3次
	#if ZEND_DEBUG
	    int inconsistent;
	#endif
	} HashTable;

### 数据容器：槽位

	typedef struct bucket {
	    ulong h;            // 对char *key进行hash后的值，或者是用户指定的数字索引值
	    uint nKeyLength;    // hash关键字的长度，如果数组索引为数字，此值为0
	    void *pData;        // 指向value，一般是用户数据的副本，如果是指针数据，则指向pDataPtr
	    void *pDataPtr;     //如果是指针数据，此值会指向真正的value，同时上面pData会指向此值
	    struct bucket *pListNext;   // 整个hash表的下一元素
	    struct bucket *pListLast;   // 整个哈希表该元素的上一个元素
	    struct bucket *pNext;       // 存放在同一个hash Bucket内的下一个元素
	    struct bucket *pLast;       // 同一个哈希bucket的上一个元素
	   
	    char arKey[1];   // 保存当前值所对于的key字符串，这个字段只能定义在最后，实现变长结构体            
	} Bucket;
	
	
在PHP中可以使用字符串或者数字作为数组的索引，数字索引直接就可以作为哈希表的索引，数字无需进行哈希处理。h字段后面的nKeyLength字段是作为key长度的标示，如果索引是数字的话，则nKeyLength为0.在PHP数组中如果索引字符串可以被转换成数字也会被转换成数字索引。所以在php中'19' '20'这样的字符索引和数字索引19，20没有区别。

上面结构体的最后一个字段用来保存key的字符串，而这个字段却申明为只有一个字符的数组， 其实这里是一种长见的[变长结构体](http://stackoverflow.com/a/4690976/319672)，主要的目的是增加灵活性。

一张图来解释![Zend引擎哈希表结构和关系 来自tipi](http://www.php-internals.com/images/book/chapt03/03-01-02-zend_hashtable.png)

详细解释：
+ PHP的Zend引擎中哈希表处理冲突使用的是拉链
+ HashTable结构体中存储的是哈希表的整体信息，Bucket中存储的是真正的数据
+ Bucket结构体维护了两个双向链表，pNext和pLast指针分别指向本槽位所在的链表关系。比如相同hash值的两个key分别是k1,k2,先插入k1再插入k2，那么k2->pNext = k1; k1->pLast = k2;
+ pListHead和pListTail维护了整个哈希表的头元素指针和最后一个元素的指针。
+ 关于pListNext和pListLast
	+ 当在线性(顺序)遍历的时候, 就会从pListHead开始, 顺着Bucket中的pListNext/pListLast, 根据移动pInternalPointer, 来实现对所有元素的线性遍历。
	+ 数组添加元素的时候，依次添加元素。比如两个key，先添加k1再添加k2，则k1->pListNext=k2, k2->pListLast=k1。
	+ 也就是说PHP中遍历数组的顺序是和元素的添加先后顺序相关的。
	+ 哈希表的Bucket结构通过pListNext和pListLast维护了哈希表插入元素的先后顺序。
+ pListNext和pListLast指针指向的是整个哈希表所有的数据之间的链接关系。

参考文章：
1. [深入理解PHP之数组(遍历顺序)](http://www.laruence.com/2009/08/23/1065.html)
2. [深入理解PHP原理之foreach](http://www.laruence.com/2008/11/20/630.html)
3. [变量的结构和类型](http://www.php-internals.com/book/?p=chapt03/03-01-00-variables-structure)
4. [PHP的哈希表实现](http://www.php-internals.com/book/?p=chapt03/03-01-02-hashtable-in-php)
