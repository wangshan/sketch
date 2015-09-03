#ifndef LEXICAL_CACHE_HASH_FUNCTIONS_H_INCLUDED
#define LEXICAL_CACHE_HASH_FUNCTIONS_H_INCLUDED

#include <string>

namespace lexical_cache
{

    struct BKDRHash
    {
        size_t operator() (const std::string& str) const
        {
            unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
            unsigned int hash = 0;

            for(std::size_t i = 0; i < str.length(); i++)
            {
                hash = (hash * seed) + str[i];
            }

            return hash;
        }
    };

    // worse than others
    struct RSHash 
    {
        size_t operator() (const std::string& str) const
        {
            unsigned int b    = 378551;
            unsigned int a    = 63689;
            unsigned int hash = 0;

            for(std::size_t i = 0; i < str.length(); i++)
            {
                hash = hash * a + str[i];
                a    = a * b;
            }

            return hash;
        }
    };

    struct JSHash
    {
        size_t operator() (const std::string& str) const
        {
            unsigned int hash = 1315423911;

            for(std::size_t i = 0; i < str.length(); i++)
            {
                hash ^= ((hash << 5) + str[i] + (hash >> 2));
            }

            return hash;
        }
    };


    // worse
    struct PJWHash
    {
        size_t operator() (const std::string& str) const
        {
            unsigned int BitsInUnsignedInt = (unsigned int)(sizeof(unsigned int) * 8);
            unsigned int ThreeQuarters     = (unsigned int)((BitsInUnsignedInt  * 3) / 4);
            unsigned int OneEighth         = (unsigned int)(BitsInUnsignedInt / 8);
            unsigned int HighBits          = (unsigned int)(0xFFFFFFFF) << (BitsInUnsignedInt - OneEighth);
            unsigned int hash              = 0;
            unsigned int test              = 0;

            for(std::size_t i = 0; i < str.length(); i++)
            {
                hash = (hash << OneEighth) + str[i];

                if((test = hash & HighBits)  != 0)
                {
                    hash = (( hash ^ (test >> ThreeQuarters)) & (~HighBits));
                }
            }

            return hash;
        }
    };

//    unsigned int ELFHash(const std::string& str)
//    {
//        unsigned int hash = 0;
//        unsigned int x    = 0;
//
//        for(std::size_t i = 0; i < str.length(); i++)
//        {
//            hash = (hash << 4) + str[i];
//            if((x = hash & 0xF0000000L) != 0)
//            {
//                hash ^= (x >> 24);
//            }
//            hash &= ~x;
//        }
//
//        return hash;
//    }


//    unsigned int SDBMHash(const std::string& str)
//    {
//        unsigned int hash = 0;
//
//        for(std::size_t i = 0; i < str.length(); i++)
//        {
//            hash = str[i] + (hash << 6) + (hash << 16) - hash;
//        }
//
//        return hash;
//    }


//    unsigned int DJBHash(const std::string& str)
//    {
//        unsigned int hash = 5381;
//
//        for(std::size_t i = 0; i < str.length(); i++)
//        {
//            hash = ((hash << 5) + hash) + str[i];
//        }
//
//        return hash;
//    }


    struct DEKHash
    {
        size_t operator() (const std::string& str) const
        {
            unsigned int hash = static_cast<unsigned int>(str.length());

            for(std::size_t i = 0; i < str.length(); i++)
            {
                hash = ((hash << 5) ^ (hash >> 27)) ^ str[i];
            }

            return hash;
        }
    };


//    unsigned int BPHash(const std::string& str)
//    {
//        unsigned int hash = 0;
//        for(std::size_t i = 0; i < str.length(); i++)
//        {
//            hash = hash << 7 ^ str[i];
//        }
//
//        return hash;
//    }


    struct FNVHash
    {
        size_t operator ()(const std::string& str) const
        {
            const unsigned int fnv_prime = 0x811C9DC5;
            unsigned int hash = 0;
            for(std::size_t i = 0; i < str.length(); i++)
            {
                hash *= fnv_prime;
                hash ^= str[i];
            }

            return hash;
        }
    };


    struct APHash
    {
        size_t operator() (const std::string& str) const
        {
            unsigned int hash = 0xAAAAAAAA;

            for(std::size_t i = 0; i < str.length(); i++)
            {
                hash ^= ((i & 1) == 0) ? (  (hash <<  7) ^ str[i] * (hash >> 3)) :
                    (~((hash << 11) + (str[i] ^ (hash >> 5))));
            }

            return hash;
        }
    };

}

#endif
