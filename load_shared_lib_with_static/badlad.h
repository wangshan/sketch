#pragma once

#include <iostream>
#include <memory>
#include <unordered_map>

class BadLad {
public:
    BadLad();
    ~BadLad(); 
};

static std::unique_ptr<int> sCount;
static BadLad sBadLad;
