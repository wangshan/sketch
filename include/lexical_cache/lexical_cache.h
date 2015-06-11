#ifndef LEXICAL_CACHE_H_INCLUDED
#define LEXICAL_CACHE_H_INCLUDED

#include <unordered_map>
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
// any size because of small string optimization
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


template <
    typename real_type,
    int cache_size_N=10,
    typename enable=
        typename std::enable_if<std::is_floating_point<real_type>::value>::type
    >
class Cache
{
public:
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

    friend std::ostream& operator << (std::ostream& os, const Cache& cache)
    {
        os << "Real cached: \n";
        for (const auto& r : cache.m_reals) {
            os << "real: " << std::get<0>(r)
               << ", timestamp: " << std::get<1>(r)
               << ", string: \"" << std::get<2>(r) << "\""
               << "\n";
        }
        os << "String2Real index: \n";
        for (const auto& r : cache.m_strToReal) {
            os << "string: \"" << r.first << "\""
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
    void updateCache(const std::string& str, const real_type& fp);

private:
    // TODO: may be store a pointer to string is more efficient?
    using ValueCache
        = std::array<std::tuple<real_type, timestamp_type, std::string>,
                     cache_size_N>;

    ValueCache                            m_reals;
    ValueCache                            m_strings;

    std::unordered_map<std::string, int>  m_strToReal;
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
    auto existing = m_strToReal.find(str);
    if (existing != m_strToReal.end()) {
        ++m_cacheHit;
        return std::get<0>(m_reals[existing->second]);
    }

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

    // kick out the oldest cache
    this->updateCache(str, fp);
    return fp;
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
std::string
Cache<real_type, cache_size_N, enable>::castToStr(const real_type& real)
{
    return "";
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
    auto index = 0;
    if (m_strToReal.size() >= cache_size_N) {
        auto oldest = std::min_element(
                m_reals.begin(),
                m_reals.end(),
                [](const typename ValueCache::value_type& lhs,
                   const typename ValueCache::value_type& rhs) {
                    return std::get<1>(lhs) < std::get<1>(rhs);
                }
                );
        assert(oldest != m_reals.end());
        //auto temp = std::make_tuple(fp, updateTimestamp(m_latestTime), str);
        //std::swap(temp, *oldest);
        
        // copy the old str
        const auto oldStr = std::get<2>(*oldest);
        m_strToReal.erase(oldStr);

        const auto& oldTime = std::get<1>(*oldest);
        std::cout<< "oldest found: " << oldStr
            << ", time: "<<oldTime
            <<std::endl;

        // NOTE: will not work if container is not an arrary or vector
        index = oldest - m_reals.begin();
    }
    else {
        index = m_strToReal.size();
    }

    // when I know there's no such element, is [] more efficient or emplace?
    m_strToReal.emplace(str, index);
    std::get<0>(m_reals[index]) = fp;
    std::get<1>(m_reals[index]) = updateTimestamp(m_latestTime);
    std::get<2>(m_reals[index]) = str;
    ++m_cacheMiss;
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
