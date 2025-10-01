#pragma once
#include <memory_resource>
#include <boost/container/flat_map.hpp>
namespace raum {
    auto* DefaultResource = std::pmr::get_default_resource();

    using memory_resource = std::pmr::memory_resource;
    using synchronized_pool_resource = std::pmr::synchronized_pool_resource;
    using unsynchronized_pool_resource = std::pmr::unsynchronized_pool_resource;
    using monotonic_resource = std::pmr::monotonic_buffer_resource;

    template <typename T>
    using PmrVector = std::pmr::vector<T>;

    using PmrString = std::pmr::string;

    template <typename K, typename V>
    using PmrMap = std::pmr::map<K, V>;

    template <typename K, typename V>
    using PmrUnorderedMap = std::pmr::unordered_map<K, V>;

    template <typename T>
    using PmrSet = std::pmr::set<T>;

    template <typename T>
    using PmrUnorderedSet = std::pmr::unordered_set<T>;

    template <typename K, typename V>
    using FlatMap = boost::container::flat_map<K, V>;


}