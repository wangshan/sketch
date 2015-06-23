#include <TestUtils.h>

#include <lexical_cache/lexical_cache.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <algorithm>

using namespace ::testing;

namespace lexical_cache {

using range_type = std::pair<double, double>;

static range_type testRanges[] = {
    {-9999.9999, 9999.9999},
//    {0.0, 0.0001},
//    {std::numeric_limits<double>::min(), std::numeric_limits<double>::max()},
};


constexpr int g_cacheSize = 64;

class StringToRealPerfTest : public ::testing::TestWithParam<range_type>
{
public:
    StringToRealPerfTest()
    {
        for (int i = 0; i < g_cacheSize; ++i) {
            m_testPairs.push_back(
                    randomStringRealPair(
                        GetParam().first,
                        GetParam().second)
                    );
            //std::cout<<m_testPairs[i].first<<":"<<m_testPairs[i].second
            //<<std::endl;
        }

        // populate the cache
        for (const auto& p : m_testPairs) {
            auto d = m_cache.castToReal(p.first);
            //EXPECT_FLOAT_EQ(d, p.second);
        }

        EXPECT_EQ(g_cacheSize, m_cache.size());
        EXPECT_EQ(g_cacheSize, m_cache.size(String2Real));
    }

protected:
    Cache<double, g_cacheSize>                    m_cache;
    std::vector< std::pair<std::string, double> > m_testPairs;

    std::vector<std::string> generateTestSequence(int iteration, double hitRatio)
    {
        auto numOfHits = std::min(iteration,
                static_cast<int>(iteration * hitRatio));

        // choose a random test sequence
        std::vector<std::string> testSequence;
        testSequence.reserve(iteration);
        for (int i = 0; i < numOfHits; ++i) {
            auto chosen = randomInt(0, g_cacheSize-1);
            testSequence.push_back(m_testPairs[chosen].first);
        }
        // note: 
        // this can possibly generate numbers that's already in the cache, we
        // ignore this case for simplicity purpose, this should be fine as long
        // as the range is big
        for (int i = 0; i < iteration - numOfHits; ++i) {
            testSequence.push_back(
                    randomString(GetParam().first, GetParam().second));
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(testSequence.begin(), testSequence.end(), g);

        assert(testSequence.size() == iteration);

        return testSequence;
    }

    std::vector<double> generateTestSequence2(int iteration, double hitRatio)
    {
        auto numOfHits = std::min(iteration,
                static_cast<int>(iteration * hitRatio));

        // choose a random test sequence
        std::vector<double> testSequence;
        testSequence.reserve(iteration);
        for (int i = 0; i < numOfHits; ++i) {
            auto chosen = randomInt(0, g_cacheSize-1);
            testSequence.push_back(m_testPairs[chosen].second);
        }
        // note: 
        // this can possibly generate numbers that's already in the cache, we
        // ignore this case for simplicity purpose, this should be fine as long
        // as the range is big
        for (int i = 0; i < iteration - numOfHits; ++i) {
            testSequence.push_back(
                    randomReal(GetParam().first, GetParam().second));
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(testSequence.begin(), testSequence.end(), g);

        assert(testSequence.size() == iteration);

        return testSequence;
    }


    void testWithCache(const std::vector<std::string>& testSequence,
            int iteration)
    {
        using namespace std::chrono;
        auto start = std::chrono::system_clock::now();
        for (int i = 0; i < iteration; ++i) {
            auto d = m_cache.castToReal(testSequence[i]);
        }
        auto finish = std::chrono::system_clock::now();
        auto duration = finish - start;
        std::cout << "with cache, mean latency: "
            << duration_cast<nanoseconds>(duration).count() / iteration
            << " ns, cache miss ratio: "
            << m_cache.missRatio()<<"%"<<std::endl;
    }

    void testWithoutCache(const std::vector<std::string>& testSequence,
            int iteration)
    {
        using namespace std::chrono;
        auto start = std::chrono::system_clock::now();
        for (int i = 0; i < iteration; ++i) {
            auto d = std::stod(testSequence[i]);
        }
        auto finish = std::chrono::system_clock::now();
        auto duration = finish - start;
        std::cout << "no cache, mean latency: "
            << duration_cast<nanoseconds>(duration).count() / iteration
            << " ns"
            << std::endl;
    }

    void testWithCache2(const std::vector<double>& testSequence,
            int iteration)
    {
        using namespace std::chrono;
        auto start = std::chrono::system_clock::now();
        for (int i = 0; i < iteration; ++i) {
            auto d = m_cache.castToStr(testSequence[i]);
        }
        auto finish = std::chrono::system_clock::now();
        auto duration = finish - start;
        std::cout << "with cache, mean latency: "
            << duration_cast<nanoseconds>(duration).count() / iteration
            << " ns, cache miss ratio: "
            << m_cache.missRatio()<<"%"<<std::endl;
    }

    void testWithoutCache2(const std::vector<double>& testSequence,
            int iteration)
    {
        using namespace std::chrono;
        auto start = std::chrono::system_clock::now();
        for (int i = 0; i < iteration; ++i) {
            auto d = std::to_string(testSequence[i]);
        }
        auto finish = std::chrono::system_clock::now();
        auto duration = finish - start;
        std::cout << "no cache, mean latency: "
            << duration_cast<nanoseconds>(duration).count() / iteration
            << " ns"
            << std::endl;
    }
};

INSTANTIATE_TEST_CASE_P(PerfTest,
        StringToRealPerfTest,
        ::testing::ValuesIn(testRanges));

TEST_P(StringToRealPerfTest, DISABLED_testCacheHitPerformance)
{
    constexpr int iteration = 1000*1000;

    auto cache_hit_ratio = 1.0;

    auto testSequence = this->generateTestSequence(iteration, cache_hit_ratio);
    m_cache.resetStats();

    this->testWithCache(testSequence, iteration);
    this->testWithoutCache(testSequence, iteration);
}

TEST_P(StringToRealPerfTest, testCacheMissPerformance)
{
    constexpr int iteration = 1000*1000;

    // note: the final miss ratio is much higher due to the shuffle
    auto cache_hit_ratio = 0.8;

    auto testSequence = this->generateTestSequence(iteration, cache_hit_ratio);
    m_cache.resetStats();

    this->testWithCache(testSequence, iteration);
    this->testWithoutCache(testSequence, iteration);
}

TEST_P(StringToRealPerfTest, testCastRealToString)
{
    constexpr int iteration = 1000*1000;

    // note: the final miss ratio is much higher due to the shuffle
    auto cache_hit_ratio = 1.0;

    auto testSequence = this->generateTestSequence2(iteration, cache_hit_ratio);
    m_cache.resetStats();

    this->testWithCache2(testSequence, iteration);
    this->testWithoutCache2(testSequence, iteration);
}

}
