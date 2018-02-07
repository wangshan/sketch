#include "badlad.h"

BadLad::BadLad() {
    if (!sCount) {
        sCount.reset(new int(1));
        std::cout<<"BadLad, reset count to, "<<*sCount<<std::endl;
    }
    else {
        // comment out this line triggers segfault
        ++*sCount;
        std::cout<<"BadLad, "<<*sCount<<std::endl;
    }
}

BadLad::~BadLad() {
    if (sCount && --*sCount == 0) {
        std::cout<<"~BadLad, deleting count "<<*sCount<<std::endl;
        delete(sCount.release());
    }
    else {
        std::cout<<"~BadLad, "<<*sCount<<std::endl;
    }
}

