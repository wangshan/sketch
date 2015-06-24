#ifndef LEXICAL_CACHE_H_INCLUDED
#define LEXICAL_CACHE_H_INCLUDED

#include "hash_functions.h"

#include <sparsehash/dense_hash_map>

#include <unordered_map>
#include <map>
#include <string>
#include <type_traits>
#include <chrono>
#include <tuple>
#include <array>
#include <iostream>
#include <assert.h>

// TODO: sort by time, so I can kick out the oldest one
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
// a bespoke structure
// std::array<struct { string, double, time }>
// do a binary search of either string or double when reading,
// when overflow, heaptify by time and then evict heap root
// the two caches have to be the same, unless use 2 container
// q1: do i have cache line efficiency here:
// not sure, double and time will be fixed size, but string can be of
// any size because of small string optimization, so have to use char[]
// q2: what about thread safety? not much gain here
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

inline timestamp_type updateTimestamp(timestamp_type& t)
{
    return ++t;
}

enum CacheType {
    String2Real = 0,
    Real2String,
    Both,
};

struct CstrHash {                                                                  
    inline size_t operator()(const char *s) const {                             
        size_t hash = 1;                                                        
        if (!s)                                                                 
            return 0;                                                           
        for (; *s; ++s)                                                         
            hash = hash * 5 + *s;                                               
        return hash;                                                            
    }                                                                           
    inline bool operator()(const char *s1, const char *s2) const {              
        if (!s1 || !s2)                                                         
            return s1 == s2;                                                    
        return strcmp(s1, s2) == 0;                                             
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
            memset(m_str, 0, 128);
        }
        char m_str[128];
        real_type m_real;
        timestamp_type m_time;
    };

    struct CachedReal
    {
        CachedReal()
            : m_real(NAN)
            , m_time(0)
        {
        }

        CachedReal(real_type r, timestamp_type t)
            : m_real(r)
            , m_time(t)
        {
        }

        real_type       m_real;
        timestamp_type  m_time;
    };

    struct CachedString
    {
        CachedString()
            : m_str("")
            , m_time(0)
        {
        }

        CachedString(const std::string& s, timestamp_type t)
            : m_str(s)
            , m_time(t)
        {
        }

        std::string     m_str;
        timestamp_type  m_time;
    };

    Cache()
    {
    }

    ~Cache() = default;

    // all copy and move operations using default

    // a const function may not be a good idea, it means the cache won't be
    // updated
    real_type castToReal(const std::string& str);
    std::string castToStr(const real_type& real);

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
//        os << "Real cached: \n";
//        for (const auto& r : cache.m_reals) {
//            os << "real: " << std::get<0>(r)
//               << ", timestamp: " << std::get<1>(r)
//               << ", string: \"" << std::get<2>(r) << "\""
//               << "\n";
//        }
//        os << "String2Real index: \n";
//        for (const auto& r : cache.m_strToReal) {
//            os << "string: \"" << r.first << "\""
//               << ", index: " << r.second
//               << "\n";
//        }
        if (cache.m_enableStats) {
            os << "cache miss ratio: " << cache.missRatio()
               << "%\n";
        }
        return os;
    }

protected:
    // only called when str is not in internal cache
    void updateCache(const std::string& str, const real_type& fp);
    void updateCache2(const real_type& fp, const std::string& str);
    real_type updateCache3(const std::string& str);

private:
    using ValueCache = std::array<CachedItem, cache_size_N>;

    ValueCache                            m_reals;
    ValueCache                            m_strings;

    // test shows searching in unordered map is faster than a sorted array
    //google::dense_hash_map<std::string, CachedReal> m_strToReal;

    std::unordered_map<const char*, int, CstrHash, CstrHash>  m_strToReal;
    //std::unordered_map<std::string, CachedReal>  m_strToReal;
    std::unordered_map<real_type, CachedString>  m_realToStr;

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

    /*
    // TODO: can move the is_same check to a bool parameter of the function,
    // then let overload chose stod/l to call
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
    */

    // kick out the oldest cache
    return this->updateCache3(str);
    //return fp;
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
std::string
Cache<real_type, cache_size_N, enable>::castToStr(const real_type& real)
{
    auto existing = m_realToStr.find(real);
    if (existing != m_realToStr.end()) {
        ++m_cacheHit;
        return existing->second.m_str;
    }

    auto str = std::to_string(real);

    // kick out the oldest cache
    this->updateCache2(real, str);
    return str;
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
void
Cache<real_type, cache_size_N, enable>::updateCache(
        const std::string& str,
        const real_type& fp)
{
    ++m_cacheMiss;
//    static bool firstTime = true;
//    if (!firstTime)
//        return;
    if (m_strToReal.size() >= cache_size_N) {
        auto oldest = std::min_element(
                m_strToReal.begin(),
                m_strToReal.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.second.m_time < rhs.second.m_time; }
                );
        //assert(oldest != m_reals.end());
        
        m_strToReal.erase(oldest);
//        firstTime = false;
    }

    // when I know there's no such element, is [] more efficient or emplace?
    m_strToReal.emplace(
            str, CachedReal(fp, updateTimestamp(m_latestTime))
            );
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
void
Cache<real_type, cache_size_N, enable>::updateCache2(
        const real_type& fp,
        const std::string& str)
{
    ++m_cacheMiss;
    if (m_realToStr.size() >= cache_size_N) {
        auto oldest = std::min_element(
                m_realToStr.begin(),
                m_realToStr.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.second.m_time < rhs.second.m_time; }
                );
        
        m_realToStr.erase(oldest);
    }

    m_realToStr.emplace(
            fp, CachedString(str, updateTimestamp(m_latestTime))
            );
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
real_type
Cache<real_type, cache_size_N, enable>::updateCache3(
        const std::string& str)
{
    ++m_cacheMiss;
//    static bool firstTime = true;
//    if (!firstTime)
//        return;

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
        m_strToReal.erase(oldest->m_str);
//        firstTime = false;
    }
    else {
        index = m_strToReal.size();
    }

    strcpy(m_reals[index].m_str, str.c_str());
    m_reals[index].m_real = std::stod(str);
    m_reals[index].m_time = updateTimestamp(m_latestTime);

    m_strToReal.emplace(
            str.c_str(), index
            );

    return m_reals[index].m_real;
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
