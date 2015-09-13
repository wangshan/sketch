#ifndef LEXICAL_CACHE_H_INCLUDED
#define LEXICAL_CACHE_H_INCLUDED

#include "hash_functions.h"

#include <sparsehash/dense_hash_map>
#include <comparefp/comparefp.h>

#include <unordered_map>
#include <map>
#include <string>
#include <type_traits>
#include <chrono>
#include <tuple>
#include <array>
#include <iostream>
#include <assert.h>

// NOTES: sort by time, so I can kick out the oldest one
// only write need to know who's the oldest and only when cache is full
// both write and read will update time
// time can be a int sequence, every time a cache is read, it's time
// member will be assigned the global sequence, t increments
// monotonically, the one with smallest number is the oldest, need to deal
// with wrap
//
// solution A:
// map<string, struct{double, time}>
// map<doulbe, struct{string, time}>
// iterate through the map to find the oldest one, when N is small enough,
// should have little performance impact (however if N is too small, there
// will be more eviction)
// the two caches can be different 
//
// solution B:
// use multi-index container:
// struct { string, double, time }
// seems redundant, do i pay the time sorting penalty on every read?
// the two caches have to be the same, unless use 2 container
//
// solution C:
// std::array<struct { string, double, time }>
// search for either string or double when reading,
// when overflow, heaptify by time and then evict heap root
// the two caches have to be the same, unless use 2 containers
// q1: do i have cache line efficiency here:
// not sure, double and time will be fixed size, but string can be of
// any size because of small string optimization, so have to use char[]
//
// futher potential improvements:
// 1. do i have to construct a new string every time? can I reuse the cached 
//    memory?
// 2. thread safety, should provide a mutex policy, what about thread local?
//
// 3. should collect stats about cost of string <-> real conversion with mutex
//    locking, if the cost outweights the benefit, then simply return a
//    conversion everytime.
//
namespace lexical_cache
{

using timestamp_type = int;

// TODO: handle wrap
inline timestamp_type updateTimestamp(timestamp_type& t)
{
    return ++t;
}

enum CacheType {
    String2Real = 0,
    Real2String,
    Both,
};

struct CstrHash
{
    inline size_t operator() (const char* s) const {
        if (!s) {
            return 0;
        }
        size_t hash = 1;
        // the below loop is the equivalent functionality of the duffy device
//        for (; *s; ++s) {
//            hash = hash * 5 + *s;                                               
//        }
        auto count = strlen(s);
        auto n = (count + 3)/4;
        switch (count % 4) {
            case 0: do { hash = hash * 5 + *s; ++s;
            case 3: hash = hash * 5 + *s; ++s;
            case 2: hash = hash * 5 + *s; ++s;
            case 1: hash = hash * 5 + *s; ++s;
            } while (--n > 0);
        }
        return hash;
    }

    inline bool operator() (const char* s1, const char* s2) const {
        if (!s1 || !s2) {
            return s1 == s2;
        }
        return ::strcmp(s1, s2) == 0;
    }
};

template <
    typename real_type,
    int cache_size_N=10,
    typename enable=
        typename std::enable_if<std::is_floating_point<real_type>::value>::type
    >
class Cache
{
public:
    struct CachedItem
    {
        CachedItem()
            : m_real(NAN)
            , m_time(0)
        {
        }

        std::string m_str;
        real_type m_real;
        timestamp_type m_time;
    };

    Cache()
    {
    }

    ~Cache() = default;

    // all copy and move operations using default

    real_type castToReal(const std::string& str);
    const char* castToStr(const real_type& real);

    size_t size(const CacheType& t=Both) const;
    bool   empty(const CacheType& t=Both) const;
    void   clear(const CacheType& t=Both);

    double missRatio() const
    {
        return static_cast<double>(m_cacheMiss) / (m_cacheHit + m_cacheMiss)*100;
    }

    void resetStats()
    {
        m_cacheMiss = 0.0;
        m_cacheHit = 0.0;
    }

    friend std::ostream& operator << (std::ostream& os, const Cache& cache)
    {
        os << "Real cached: \n";
        for (const auto& r : cache.m_reals) {
            os << "real: " << r.m_real
               << ", timestamp: " << r.m_time 
               << ", string: \"" << r.m_str << "\""
               << "\n";
        }
        os << "String2Real index: \n";
        for (const auto& r : cache.m_strToReal) {
            os << "string: \"" << r.first << "\""
               << ", index: " << r.second
               << "\n";
        }
        os << "String cached: \n";
        for (const auto& r : cache.m_strings) {
            os << "real: " << r.m_real
               << ", timestamp: " << r.m_time 
               << ", string: \"" << r.m_str << "\""
               << "\n";
        }
        os << "String2Real index: \n";
        for (const auto& r : cache.m_realToStr) {
            os << "real: \"" << r.first << "\""
               << ", index: " << r.second
               << "\n";
        }
        if (cache.m_enableStats) {
            os << "cache miss ratio: " << cache.missRatio()
               << "%\n";
        }
        return os;
    }

protected:
    // only called when str is not in internal cache
    real_type updateStrCache(const std::string& str); //370ns
    const char* updateRealCache(const real_type& fp); //600ns

private:
    using ValueCache = std::array<CachedItem, cache_size_N>;

    ValueCache                            m_reals;
    ValueCache                            m_strings;

    // test shows searching in unordered map is faster than a sorted array
    std::unordered_map<const char*, int,
        CstrHash, CstrHash>               m_strToReal;
    std::unordered_map<real_type, int>    m_realToStr;

    timestamp_type                        m_latestTime = 100;
    bool                                  m_enableStats = true;
    long                                  m_cacheHit = 0;
    long                                  m_cacheMiss = 0;
};

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
real_type
Cache<real_type, cache_size_N, enable>::castToReal(const std::string& str)
{
    // not much advantage compared with stod, even with 100% cache hit, which
    // means I need a faster hash map
    // need to test with boost::lexical_cast
    auto existing = m_strToReal.find(str.c_str());
    if (existing != m_strToReal.end()) {
        ++m_cacheHit;
        return m_reals[existing->second].m_real;
    }

    return this->updateStrCache(str);
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
const char*
Cache<real_type, cache_size_N, enable>::castToStr(const real_type& real)
{
    auto existing = std::find_if(m_realToStr.begin(), m_realToStr.end(),
            [&real](const auto& item) {
                return useful::almostEqual(item.first, real);
            });
    if (existing != m_realToStr.end()) {
        ++m_cacheHit;
        return m_strings[existing->second].m_str.c_str();
    }

    return this->updateRealCache(real);
}


template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
real_type
Cache<real_type, cache_size_N, enable>::updateStrCache(const std::string& str)
{
    ++m_cacheMiss;

    auto index = 0;
    if (m_strToReal.size() >= cache_size_N) {
        auto oldest = std::min_element(
                m_reals.begin(),
                m_reals.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.m_time < rhs.m_time; }
                );
        assert(oldest != m_reals.end());
        
        index = oldest - m_reals.begin();
        m_strToReal.erase(oldest->m_str.c_str());
    }
    else {
        index = m_strToReal.size();
    }

    real_type fp(0.0);
    if (std::is_same<float,
            typename std::remove_cv<real_type>::type>::value) {
        fp = std::stof(str);
    }
    else if (std::is_same<double,
            typename std::remove_cv<real_type>::type>::value) {
        fp = std::stod(str);
    }
    else {
        fp = std::stold(str);
    }

    m_reals[index].m_str = str;
    m_reals[index].m_real = fp;
    m_reals[index].m_time = updateTimestamp(m_latestTime);

    m_strToReal.emplace(
            m_reals[index].m_str.c_str(), index
            );

    return fp;
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
const char*
Cache<real_type, cache_size_N, enable>::updateRealCache(const real_type& fp)
{
    ++m_cacheMiss;

    auto index = 0;
    if (m_realToStr.size() >= cache_size_N) {
        auto oldest = std::min_element(
                m_strings.begin(),
                m_strings.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.m_time < rhs.m_time; }
                );
        
        index = oldest - m_strings.begin();
        m_realToStr.erase(oldest->m_real);
    }
    else {
        index = m_realToStr.size();
    }

    m_strings[index].m_str = std::to_string(fp);
    m_strings[index].m_real = fp;
    m_strings[index].m_time = updateTimestamp(m_latestTime);

    m_realToStr.emplace(fp, index);

    return m_strings[index].m_str.c_str();
}


template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
size_t Cache<real_type, cache_size_N, enable>::size(const CacheType& t) const
{
    if (t == String2Real) {
        return m_strToReal.size();
    }
    else if (t == Real2String) {
        return m_realToStr.size();
    }
    else {
        return m_strToReal.size() + m_realToStr.size();
    }
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
bool Cache<real_type, cache_size_N, enable>::empty(const CacheType& t) const
{
    if (t == String2Real) {
        return m_strToReal.empty();
    }
    else if (t == Real2String) {
        return m_realToStr.empty();
    }
    else {
        return m_strToReal.empty() && m_realToStr.empty();
    }
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
void Cache<real_type, cache_size_N, enable>::clear(const CacheType& t)
{
    if (t == String2Real || t == Both) {
        m_strToReal.clear();
        m_reals.clear();
    }

    if (t == Real2String || t == Both) {
        m_realToStr.clear();
        m_strings.clear();
    }
}

}

#endif
