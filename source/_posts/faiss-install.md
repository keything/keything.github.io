---
title: faiss-install
date: 2018-04-10 14:48:07
tags: faiss
---

一、clone faiss项目

git clone https://github.com/facebookresearch/faiss.git

二、安装必要依赖和工具


+ conda install openblas

+ brew install llvm

+ 如果提示:dyld: Library not loaded: @rpath/libomp.dylib
    + ln -s $HOME/anaconda2/lib/libopenblas.dylib /usr/lib/libopenblas.dylib
    + export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:/usr/local/opt/llvm/lib/

+ make tests/test_blas



常见错误：

+ fatal error: 'stdio.h' file not found
	
	+ 解决：xcode-select --install

+ fatal error: 'malloc.h' file not found
	
	+ IndexScalarQuantizer.cpp 中 malloc.h 改成 sys/malloc.h


三、编译安装c++部分

+ make all
+ ./demos/demo_ivfpq_indexing


四、安装faiss的python部分

+ which python

	+ /usr/local/Cellar/anaconda2/bin/python
	+ 建议使用anaconda的python

+ make py

+ python -c "import faiss"



五、 centos按照

+ 安装依赖

    sudo yum install -y openblas swig

+ 获取 faiss 源代码
   
    git clone https://github.com/facebookresearch/faiss.git

+ 编译 faiss

    cd faiss
    cp example_makefiles/makefile.inc.Linux makefile.inc
    make all

+ 编译 python 接口

    make py

+ 设置优化选项

    export OMP_WAIT_POLICY=PASSIVE

+  运行 python 示例代码

    export PYTHONPATH=.
    python tutorial/python/1-Flat.py
    python tutorial/python/2-IVFFlat.py
    python tutorial/python/3-IVFPQ.py

五、参考文章

+ https://github.com/facebookresearch/faiss/blob/master/INSTALL.md
+ https://www.mobibrw.com/2017/6235
