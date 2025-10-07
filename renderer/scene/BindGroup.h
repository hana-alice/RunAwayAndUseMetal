#pragma once
#include <boost/container/flat_map.hpp>
#include "RHIDescriptorSetLayout.h"
#include "RHIDevice.h"

namespace raum::scene {
using SlotMap = boost::container::flat_map<std::string_view, uint32_t>;
class BindGroup {
public:
    BindGroup() = delete;
    BindGroup(const SlotMap& bindings,
              rhi::DescriptorSetLayoutPtr layout,
              rhi::DevicePtr device);

    rhi::DescriptorSetPtr descriptorSet() const;

    bool contains(std::string_view slotName) const;

    void bindBuffer(std::string_view name,
                    uint32_t index,
                    rhi::BufferPtr buffer);
    void bindBuffer(std::string_view name,
                    uint32_t index,
                    uint32_t offset,
                    uint32_t size,
                    rhi::BufferPtr buffer);
    void bindBuffer(std::string_view name,
                uint32_t index,
                rhi::BufferViewPtr buffer);

    void bindImage(std::string_view name,
                   uint32_t index,
                   rhi::ImageViewPtr imgView,
                   rhi::ImageLayout layout);

    void bindSampler(std::string_view name,
                     uint32_t index,
                     const rhi::SamplerInfo& samplerInfo);

    void bindTexelBuffer(std::string_view name,
                         uint32_t index,
                         rhi::BufferViewPtr bufferView);

    void update();

private:
    SlotMap _bindingMap;
    rhi::DescriptorPoolPtr _descriptorSetPool;
    rhi::DescriptorSetPtr _descriptorSet;
    rhi::DescriptorSetLayoutPtr _descriptorSetLayout;
    rhi::BindingInfo _currentBinding;
    rhi::BindingInfo _updateInfo;
    std::vector<uint32_t> _updateIndices;

    rhi::DevicePtr _device;
};

using BindGroupPtr = std::shared_ptr<BindGroup>;

} // namespace raum::scene