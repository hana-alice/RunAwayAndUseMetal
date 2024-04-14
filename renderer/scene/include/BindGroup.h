#pragma once
#include <boost/container/flat_map.hpp>
#include "RHIDescriptorSetLayout.h"
#include "RHIDevice.h"

namespace raum::scene {

class BindGroup {
public:
    BindGroup() = delete;
    BindGroup(const boost::container::flat_map<std::string_view, uint32_t>& bindings,
              rhi::DescriptorSetLayoutPtr layout,
              rhi::DevicePtr device);

    rhi::DescriptorSetPtr descriptorSet();

    void bindBuffer(std::string_view name,
                    uint32_t index,
                    rhi::BufferPtr buffer);
    void bindBuffer(std::string_view name,
                    uint32_t index,
                    uint32_t offset,
                    uint32_t size,
                    rhi::BufferPtr buffer);

    void bindImage(std::string_view name,
                   uint32_t index,
                   rhi::ImageViewPtr imgView,
                   rhi::ImageLayout layout);
//
//    void bindImage(std::string_view name,
//                   uint32_t index,
//                   rhi::RHIImageView* imgView,
//                   rhi::ImageLayout layout);

    void bindSampler(std::string_view name,
                     uint32_t index,
                     rhi::SamplerPtr sampler);

    void bindTexelBuffer(std::string_view name,
                         uint32_t index,
                         rhi::BufferViewPtr bufferView);

    void update();

private:
    boost::container::flat_map<std::string, uint32_t> _bindingMap;
    rhi::DescriptorPoolPtr _descriptorSetPool;
    rhi::DescriptorSetPtr _descriptorSet;
    rhi::DescriptorSetLayoutPtr _descriptorSetLayout;
    rhi::BindingInfo _currentBinding;
    rhi::BindingInfo _updateInfo;
    std::vector<uint32_t> _updateIndices;
};

using BindGroupPtr = std::shared_ptr<BindGroup>;

} // namespace raum::scene