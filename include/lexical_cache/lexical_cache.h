#ifndef LEXICAL_CACHE_H_INCLUDED
#define LEXICAL_CACHE_H_INCLUDED

#include <unordered_map>
#include <string>
#include <type_traits>

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
// do i have cache line efficiency here:
// not sure, double and time will be fixed size, but string can be of
// any size because of small string optimization
//
namespace lexical_cache
{

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
    real_type cast(const std::string& str) const;
//    std::string cast(const real_type& real);
//    std::string cast(const real_type& real) const;

private:
    
    // step 1: do one way cache first
    std::unordered_map<std::string, real_type> m_strToFp;
    std::unordered_map<real_type, std::string> m_fpToStr;
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
        return existing->second;
    }

    // TODO: can move the is_same check to a bool parameter of the function,
    // then let overload chose which stox to call
    real_type fp(0.0);
    if (std::is_same<float,
            typename std::remove_cv<real_type>::type>::value) {
        real_type fp = std::stof(str);
    }
    else if (std::is_same<double,
            typename std::remove_cv<real_type>::type>::value) {
        real_type fp = std::stod(str);
    }
    else {
        real_type fp = std::stold(str);
    }

    //TODO: kick out the oldest cache
    m_strToFp.insert(std::make_pair(str, fp));
    return fp;
}

template <
    typename real_type,
    int cache_size_N,
    typename enable
    >
real_type
Cache<real_type, cache_size_N, enable>::cast(const std::string& str) const
{
    const auto existing = m_strToFp.find(str);
    if (existing != m_strToFp.end()) {
        return existing->second;
    }

    return std::stod(str);
}

}

#endif
