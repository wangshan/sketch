#ifndef LEXICAL_CACHE_H_INCLUDED
#define LEXICAL_CACHE_H_INCLUDED

#include <unordered_map>
#include <string>
#include <type_traits>
#include <chrono>
#include <tuple>
#include <array>
#include <assert.h>

// TODO: sort by time, so I can kick out the oldest one
// only write need to know who's the oldest and only when cache is full
// both write and read will update time
// time can be a int sequence, every time a cache is read, it's time
// member will be assigned the global sequence, which increments
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

    real_type cast(const std::string& str);
    // a const function may not be a good idea, it means the cache won't be
    // updated
//    std::string cast(const real_type& real);

    void clear();

protected:
    // only called when str is not in internal cache
    void updateCache(const std::string& str, const real_type& fp);

private:
    using ValueCache
        = std::array<std::tuple<real_type, timestamp_type, std::string>,
                     cache_size_N>;
    ValueCache m_reals;
    std::unordered_map<std::string, int> m_strToFp;

    timestamp_type m_latestTime = 0;
    
    // step 1: do one way cache first
//    std::unordered_map<std::string, real_type> m_strToFp;
//    std::unordered_map<real_type, std::string> m_fpToStr;
};

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
real_type
Cache<real_type, cache_size_N, enable>::cast(const std::string& str)
{
    auto existing = m_strToFp.find(str);
    if (existing != m_strToFp.end()) {
        return std::get<0>(m_reals[existing->second]);
    }

    // TODO: can move the is_same check to a bool parameter of the function,
    // then let overload chose which stox to call
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
void
Cache<real_type, cache_size_N, enable>::updateCache(
        const std::string& str,
        const real_type& fp)
{
    if (m_strToFp.size() >= cache_size_N) {
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
        const auto& oldStr = std::get<2>(*oldest);
        std::get<0>(*oldest) = fp;
        std::get<1>(*oldest) = updateTimestamp(m_latestTime);
        std::get<2>(*oldest) = str;

        m_strToFp.erase(oldStr);
        m_strToFp[str] = oldest - m_reals.begin();
    }
    else {
        m_strToFp.emplace(str, m_reals.size());
    }
}


}

#endif
