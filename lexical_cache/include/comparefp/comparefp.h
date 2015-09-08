#ifndef USEFUL_COMPAREFP_H_INCLUDED
#define USEFUL_COMPAREFP_H_INCLUDED

#include <cfloat>
#include <iostream>

// floating point comparison, see the below links for detail
// https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
// https://code.google.com/p/googletest/source/browse/trunk/include/gtest/internal/gtest-internal.h

namespace useful {

template <size_t size>
struct TypeWithSize
{
    using Int = void;
};

template <>
struct TypeWithSize<4>
{
    using Int = int;
};

template <>
struct TypeWithSize<8>
{
    using Int = long long;
};

template <typename raw_type>
class FloatingPoint
{
public:
    using IntegerSize = typename TypeWithSize<sizeof(raw_type)>::Int;

    static const int MAX_ULPS = 4;

    // allow implicit conversion

    FloatingPoint(const raw_type& raw)
    {
        m_u.f = raw;
    }

    bool negative() const
    {
        return (m_u.i & m_signBitMask) != 0;
    }

    bool almostEqual(const FloatingPoint& rhs, int maxUlpsDiff=MAX_ULPS)
    {
        // Different signs means they do not match.

        if (this->negative() != rhs.negative()) {
            return false;
        }

        // Find the difference in ULPs.
        IntegerSize ulpsDiff = std::abs(m_u.i - rhs.m_u.i);
        //std::cout<<"ulpsDiff="<<ulpsDiff<<", lhs="<<m_u.i<<", rhs="<<rhs.m_u.i
        //    <<", actual diff="<<m_u.i-rhs.m_u.i<<std::endl;
        if (ulpsDiff <= maxUlpsDiff) {
            return true;
        }

        return false;
    }

private:
    union FloatingPointUnion
    {
        FloatingPointUnion(raw_type num = 0.0) : f(num) {}
        // Portable extraction of components.
        IntegerSize i;
        raw_type f;
    };

    static const IntegerSize
        m_signBitMask{static_cast<IntegerSize>(1) << (8*sizeof(raw_type)-1)};

    FloatingPointUnion m_u;
};

// FLT_EPSILON = 1.19209e-07
// DBL_EPSILON = 2.22045e-16
inline bool almostEqual(double a, double b, double maxDiff=FLT_EPSILON)
{
    if (std::isnan(a) || std::isnan(b)) {
        return false;
    }

    // if two numbers are really close, consider them equal, useful when
    // comparing numbers close to 0, because the ulp of those numbers can be
    // enourmous, but this means if maxDiff is too big, the ulp comparison will
    // never be invoked
    double absDiff = std::fabs(a - b);
    if (absDiff <= maxDiff) {
        return true;
    }

    return FloatingPoint<double>(a).almostEqual(b);
}

inline bool almostEqual(float a, float b, float maxDiff=FLT_EPSILON)
{
    if (std::isnan(a) || std::isnan(b)) {
        return false;
    }

    // if two numbers are really close, consider them equal, useful when
    // comparing numbers close to 0, because the ulp of those numbers can be
    // enourmous, but this means if maxDiff is too big, the ulp comparison will
    // never be invoked
    float absDiff = std::fabs(a - b);
    if (absDiff <= maxDiff) {
        return true;
    }

    return FloatingPoint<float>(a).almostEqual(b);
}

}

#endif
