#pragma once
namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSet {
public:
    explicit RHIDescriptorSet(const DescriptorSetInfo&, RHIDevice*) {}
    virtual ~RHIDescriptorSet() = 0;
};
} // namespace raum::rhi