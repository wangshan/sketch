#ifndef LEXICAL_CACHE_TESTUTILS_H_INCLUDED
#define LEXICAL_CACHE_TESTUTILS_H_INCLUDED

#include <string>
#include <random>
#include <algorithm>
#include <sstream>

namespace lexical_cache {

inline double randomReal(double min=0.0, double max=1000.0)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}

inline std::string realToString(double d)
{
    std::ostringstream oss;
    oss << d;
    return oss.str();
}

inline std::string randomString(double min=0.0, double max=1000.0)
{
    return realToString(randomReal(min, max));
}

}

#endif
