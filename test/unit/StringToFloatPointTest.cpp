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
    EXPECT_FLOAT_EQ(1.0, cache.cast(x));

    const auto d = randomReal();
    const std::string y = realToString(d);
    // TODO: use almostEqual
    EXPECT_FLOAT_EQ(d, cache.cast(y));
}

TEST(StringToRealTest, testCacheFull)
{
}

}
