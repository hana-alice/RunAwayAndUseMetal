#pragma once
#include <map>
#include "RHIDefine.h"
#include "RHIDescriptorSetLayout.h"
namespace raum::rhi {

inline const DescriptorPoolInfo makeDescriptorPoolInfo(const std::vector<RHIDescriptorSetLayout*>& layouts) {
    DescriptorPoolInfo info{};
    info.maxSets = static_cast<uint32_t>(layouts.size());
    std::map<DescriptorType, uint32_t> dict;
    for (auto* layout : layouts) {
        for (const auto& bindingInfo : layout->info().descriptorBindings) {
            ++dict[bindingInfo.type];
        }
    }
    std::vector<DescriptorPoolSize> poolSize;
    poolSize.reserve(dict.size());
    for (const auto& [type, count] : dict) {
        poolSize.emplace_back(DescriptorPoolSize{type, count});
    }
    info.pools = std::move(poolSize);
    return info;
}



} // namespace raum::rhi