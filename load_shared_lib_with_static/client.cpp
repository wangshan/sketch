#include <dlfcn.h>
#include <iostream>
#include <memory>
#include "interface.h"


int main() {
    std::cout<<"main"<<std::endl;

    std::cout<<"--"<<std::endl;
    void* dll1 = dlopen("./libplugin.so", RTLD_LAZY);
    std::cout<<"loaded plugin"<<std::endl;
    dlclose(dll1);

    std::cout<<"--"<<std::endl;
    void* dll2 = dlopen("./libplugin.so", RTLD_LAZY);
    std::cout<<"loaded plugin again"<<std::endl;
    dlclose(dll2);

    std::cout<<"--"<<std::endl;
    return 0;
}
