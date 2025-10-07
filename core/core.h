#pragma once
#include <xutility>
#include <boost/container_hash/hash.hpp>
namespace raum {

inline size_t hashRange(const uint8_t* ptr, size_t size) {
    return boost::hash_range(ptr, ptr + size);
};

inline void hashRange(size_t& seed, const uint8_t* ptr, size_t size) {
    boost::hash_range(seed, ptr, ptr + size);
}

template <typename T>
struct equalTo {
    bool operator()(const T& lhs, const T& rhs) const {
        return std::equal_to<T>{}(lhs, rhs);
    }
};

}