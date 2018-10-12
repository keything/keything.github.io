---
title: cpp_class_copy_vector
date: 2017-05-13 09:22:17
tags: cpp
---

C++类中vector成员变量的拷贝

问题描述：

在使用过程中，遇到指针的问题；简要概括为：初始化中的指针和使用的指针不同；

    #include <iostream>
    #include <boost/format.hpp>
    #include <vector>

    class A {
        typedef struct {
            std::vector<std::string> vec_;
        } PhaseVecAndIter;
    public:
        PhaseVecAndIter getPhaseVec() {
            return pv_;
        }
        void init() {
            std::vector<std::string> vec;
            vec.push_back("string1");
            vec.push_back("string2");
            pv_.vec_ = vec;
            std::cout << "init vector pointer:"<<&(*(pv_.vec_.begin())) << std::endl;
        }
    private:
        PhaseVecAndIter pv_;
    };
    class B {
    public:
        void setA(boost::shared_ptr<A> a) {
            a_ = a;
        }
        boost::shared_ptr<A> getA() {
            return a_;
        }
    private:
        boost::shared_ptr<A> a_;
    };
    int main()
    {
        boost::shared_ptr<A> a(new A());
        a->init();
        std::cout << "1:vector pointer:"<<&(*(a->getPhaseVec().vec_.begin())) << std::endl;

        B* b = new B();
        b->setA(a);

        boost::shared_ptr<A> c = b->getA();
        std::cout << "2:vector pointer:"<<&(*(c->getPhaseVec().vec_.begin())) << std::endl;
    }
