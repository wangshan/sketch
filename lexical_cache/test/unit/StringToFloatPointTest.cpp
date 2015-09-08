#include "TestUtils.h"

#include <lexical_cache/lexical_cache.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace lexical_cache {

TEST(StringToRealTest, testCast)
{
    //Cache<int> wrong_cache;
    //Cache<float> cache1;
    Cache<double> cache;
    const auto x = "1.0";
    EXPECT_FLOAT_EQ(1.0, cache.castToReal(x));

    const auto d = randomReal();
    const std::string y = realToString(d);
    EXPECT_NEAR(d, cache.castToReal(y), 0.001);
}


TEST(StringToRealTest, testCacheFull)
{
    constexpr int cacheSize = 4;
    Cache<double, cacheSize> cache;

    const std::vector< std::pair<std::string, double> > testPairs = {
        {"1.0", 1.0},
        {"2.0", 2.0},
        {"3.0", 3.0},
        {"4.0", 4.0},
    };

    for (const auto& p : testPairs) {
        auto d = cache.castToReal(p.first);
        EXPECT_FLOAT_EQ(d, p.second);
    }

    EXPECT_EQ(cacheSize, cache.size());
    EXPECT_EQ(cacheSize, cache.size(String2Real));

    // cache hit
    cache.castToReal("4.0");
    EXPECT_EQ(cacheSize, cache.size(String2Real)) << cache;

    // cache miss 
    cache.castToReal("5.2");
    EXPECT_EQ(cacheSize, cache.size(String2Real)) << cache;
}

}
