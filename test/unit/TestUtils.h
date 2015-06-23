#ifndef LEXICAL_CACHE_TESTUTILS_H_INCLUDED
#define LEXICAL_CACHE_TESTUTILS_H_INCLUDED

#include <string>
#include <random>
#include <algorithm>
#include <sstream>

namespace lexical_cache {

// TODO: make the below templates
inline int randomInt(int min, int max)
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

inline double randomReal(double min=0.0, double max=1000.0)
{
    std::random_device rd;
    std::mt19937_64 generator(rd());
    std::uniform_real_distribution<double> distribution(min, max);
    return distribution(generator);
}

//inline double randomReal(double min=0.0, double max=1000.0)
//{
//    std::random_device rd;
//    std::mt19937 generator(rd());
//    std::exponential_distribution<double> distribution(0.3);
//    return distribution(generator);
//}
//
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

inline std::pair<std::string, double> randomStringRealPair(double min=0.0, double max=1000.0)
{
    auto real = randomReal(min, max);
    return std::make_pair(realToString(real), real);
}

}

#endif
