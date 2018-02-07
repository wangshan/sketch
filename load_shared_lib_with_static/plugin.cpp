#include "interface.h"
#include "badlad.h"

#include <iostream>


class Foo : public Interface {
public:
    virtual void print() override { std::cout<<"Foo"<<std::endl; }
};

//extern "C" {
//Interface* create() {
//    return new Foo;
//}
//}
//
//extern "C" {
//void destroy(Interface* p) {
//    delete p;
//}
//}

