#include <TestUtils.h>

#include <lexical_cache/lexical_cache.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

namespace lexical_cache {

TEST(StringToRealPerfTest, testCacheHitPerformance)
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

    int iteration = 1000*1000;
    auto start = std::chrono::system_clock::now();
    for (int i = 0; i < iteration; ++i) {
        for (const auto& p : testPairs) {
            auto d = cache.castToReal(p.first);
            //EXPECT_FLOAT_EQ(d, p.second);
        }
    }
    auto finish = std::chrono::system_clock::now();
    auto duration = finish - start;

    // cache miss 
//    cache.castToReal("5.2");
//    EXPECT_EQ(cacheSize, cache.size(String2Real)) << cache;
}

}
