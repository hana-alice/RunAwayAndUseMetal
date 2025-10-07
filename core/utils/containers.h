#pragma once
#include "core/core.h"
#include <boost/container/flat_map.hpp>
#include <boost/container/pmr/flat_map.hpp>
#include <boost/container/pmr/flat_set.hpp>
#include <map>
#include <memory_resource>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "log.h"
#include "renderer/rhi/base/RHIDefine.h"
namespace raum {

    using memory_resource = std::pmr::memory_resource;
    using synchronized_pool_resource = std::pmr::synchronized_pool_resource;
    using unsynchronized_pool_resource = std::pmr::unsynchronized_pool_resource;
    using monotonic_resource = std::pmr::monotonic_buffer_resource;

    using Pred = std::less<>;

    template <typename T>
    using PmrVector = std::pmr::vector<T>;

    using PmrString = std::pmr::string;

    template <typename K, typename V, typename P = Pred>
    using PmrMap = std::pmr::map<K, V, P>;

    template <typename K, typename V, typename P = Pred>
    using PmrUnorderedMap = std::pmr::unordered_map<K, V, P>;

    template <typename K, typename V>
    using RaumMap = std::pmr::unordered_map<K, V, rhi::RHIHash<K>, equalTo<K>>;

    template <typename T, typename P = Pred>
    using PmrSet = std::pmr::set<T, P>;

    template <typename T>
    using PmrUnorderedSet = std::pmr::unordered_set<T>;

    template <typename K, typename V, typename P = Pred>
    using FlatMap = boost::container::flat_map<K, V, P>;

    template <typename K, typename V, typename P = Pred>
    using PmrFlatMap = boost::container::pmr::flat_map<K, V, P>;

class TrackedResource : public std::pmr::memory_resource {
public:
    TrackedResource(const std::string& name, std::pmr::memory_resource* resource)
        :_name(name), _resource(resource) {}
    void* do_allocate(size_t bytes, size_t alignment) override {
        void* p = _resource->allocate(bytes, alignment);
        raum_info("[Alloc] {0}: {1} bytes at {2}", _name, bytes, p);
        return p;
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) override {
        raum_info("[Dealloc] {0}: {1} bytes at {2}", _name, bytes,  p);
        _resource->deallocate(p, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

private:
    std::string _name;
    std::pmr::memory_resource* _resource = nullptr;
};

TrackedResource* getGlobalTrackedResource();

auto* DefaultResource = std::pmr::get_default_resource();
// inline auto* DefaultResource = getGlobalTrackedResource();

}