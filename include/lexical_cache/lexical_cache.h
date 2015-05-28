#ifndef LEXICAL_CACHE_H_INCLUDED
#define LEXICAL_CACHE_H_INCLUDED

#include <unordered_map>
#include <string>

namespace lexical_cache
{

// TODO: only allow real number type
template <typename float_point_type, int cache_size_N=10>
class Cache
{
public:
    ~Cache() = default;

    float_point_type cast(const std::string& str);
    float_point_type cast(const std::string& str) const;

private:
    // TODO: sort by time, so I can kick out the oldest one
    // only write need to know who's the oldest and only when cache is full
    // both write and read will update time
    // time can be a int sequence, every time a cache is read, it's time
    // member will be assigned the global sequence, which increments
    // monotonically, the one with smallest number is the oldest
    //
    // solution A:
    // map<string, struct{double, time}>
    // map<doulbe, struct{string, time}>
    // iterate through the map to find the oldest one, when N is small enough,
    // should have little performance impact
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
    
    // step 1: do one way cache first
    std::unordered_map<std::string, float_point_type> m_strToFp;
};

template <typename float_point_type, int cache_size_N>
float_point_type
Cache<float_point_type, cache_size_N>::cast(const std::string& str)
{
    auto existing = m_strToFp.find(str);
    if (existing != m_strToFp.end()) {
        return existing->second;
    }

    // TODO: chose different function based on float_point_type
    float_point_type fp = std::stod(str);
    m_strToFp.insert(std::make_pair(str, fp));
    return fp;
}

template <typename float_point_type, int cache_size_N>
float_point_type
Cache<float_point_type, cache_size_N>::cast(const std::string& str) const
{
    const auto existing = m_strToFp.find(str);
    if (existing != m_strToFp.end()) {
        return existing->second;
    }

    return std::stod(str);
}

}

#endif
