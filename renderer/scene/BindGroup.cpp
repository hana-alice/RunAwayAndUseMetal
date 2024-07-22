#include "BindGroup.h"
#include "RHIUtils.h"
#include <algorithm>
namespace raum::scene {

BindGroup::BindGroup(const SlotMap &bindings, rhi::DescriptorSetLayoutPtr layout, rhi::DevicePtr device)
:_device(device) {
    std::for_each(bindings.begin(), bindings.end(), [&](const auto& p) {
        _bindingMap.emplace(p.first, p.second);
    });
    const auto& poolInfo = rhi::makeDescriptorPoolInfo({layout.get()});
    _descriptorSetPool = rhi::DescriptorPoolPtr(device->createDescriptorPool(poolInfo));
    rhi::DescriptorSetInfo descSetInfo{
        .layout = layout.get(),
        .bindingInfos = {},
    };
    _descriptorSet = rhi::DescriptorSetPtr (_descriptorSetPool->makeDescriptorSet(descSetInfo));
    _descriptorSetLayout = layout;
    _updateIndices.resize(16);

    for(const auto& descBinding : layout->info().descriptorBindings) {
        switch (descBinding.type) {
            case rhi::DescriptorType::UNIFORM_BUFFER:
            case rhi::DescriptorType::UNIFORM_BUFFER_DYNAMIC: {
                auto& bufferBinding = _currentBinding.bufferBindings.emplace_back();
                bufferBinding.binding = descBinding.binding;
                bufferBinding.arrayElement = 0;
                bufferBinding.type = descBinding.type;
                bufferBinding.buffers.resize(descBinding.count);
                for(auto& bindingView : bufferBinding.buffers) {
                    auto uniformBuffer = rhi::defaultUniformBuffer(device);
                    bindingView.buffer = uniformBuffer.get();
                    bindingView.size = uniformBuffer->info().size;
                }
                _updateIndices[descBinding.binding] = _currentBinding.bufferBindings.size() - 1;
                break;
            };
            case rhi::DescriptorType::STORAGE_BUFFER:
            case rhi::DescriptorType::STORAGE_BUFFER_DYNAMIC: {
                auto& bufferBinding = _currentBinding.bufferBindings.emplace_back();
                bufferBinding.binding = descBinding.binding;
                bufferBinding.arrayElement = 0;
                bufferBinding.type = descBinding.type;
                bufferBinding.buffers.resize(descBinding.count);
                for(auto& bindingView : bufferBinding.buffers) {
                    auto storageBuffer = rhi::defaultStorageBuffer(device);
                    bindingView.buffer = storageBuffer.get();
                    bindingView.size = storageBuffer->info().size;
                }
                _updateIndices[descBinding.binding] = _currentBinding.bufferBindings.size() - 1;
                break;
            };
            case rhi::DescriptorType::STORAGE_IMAGE: {
                auto& imageBinding = _currentBinding.imageBindings.emplace_back();
                imageBinding.binding = descBinding.binding;
                imageBinding.arrayElement = 0;
                imageBinding.type = descBinding.type;
                imageBinding.imageViews.resize(descBinding.count);
                for(auto& bindingView : imageBinding.imageViews) {
                    auto storageImageView = rhi::defaultStorageImageView(device);
                    bindingView.imageView = storageImageView.get();
                    bindingView.layout = rhi::ImageLayout::GENERAL;
                }
                _updateIndices[descBinding.binding] = _currentBinding.imageBindings.size() - 1;
                break;
            }
            case rhi::DescriptorType::SAMPLED_IMAGE:{
                auto& imageBinding = _currentBinding.imageBindings.emplace_back();
                imageBinding.binding = descBinding.binding;
                imageBinding.arrayElement = 0;
                imageBinding.type = descBinding.type;
                imageBinding.imageViews.resize(descBinding.count);
                for(auto& bindingView : imageBinding.imageViews) {
                    auto sampledImageView = rhi::defaultSampledImageView(device);
                    bindingView.imageView = sampledImageView.get();
                    bindingView.layout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                }
                _updateIndices[descBinding.binding] = _currentBinding.imageBindings.size() - 1;
                break;
            }
            case rhi::DescriptorType::UNIFORM_TEXEL_BUFFER: {
                auto& texelBinding = _currentBinding.texelBufferBindings.emplace_back();
                texelBinding.binding = descBinding.binding;
                texelBinding.arrayElement = 0;
                texelBinding.type = descBinding.type;
                texelBinding.bufferViews.resize(descBinding.count, rhi::defaultUniformBufferView(device).get());
                _updateIndices[descBinding.binding] = _currentBinding.texelBufferBindings.size() - 1;
                break;
            }
            case rhi::DescriptorType::STORAGE_TEXEL_BUFFER: {
                auto& texelBinding = _currentBinding.texelBufferBindings.emplace_back();
                texelBinding.binding = descBinding.binding;
                texelBinding.arrayElement = 0;
                texelBinding.type = descBinding.type;
                texelBinding.bufferViews.resize(descBinding.count, rhi::defaultStorageBufferView(device).get());
                _updateIndices[descBinding.binding] = _currentBinding.texelBufferBindings.size() - 1;
                break;
            }
            case rhi::DescriptorType::SAMPLER: {
                auto& samplerBinding = _currentBinding.samplerBindings.emplace_back();
                samplerBinding.binding = descBinding.binding;
                samplerBinding.arrayElement = 0;
                const auto& linearSampler = rhi::defaultLinearSampler(device);
                samplerBinding.samplers.resize(descBinding.count, device->getSampler(linearSampler));
                _updateIndices[descBinding.binding] = _currentBinding.samplerBindings.size() - 1;
                break;
            }
        }
    }
    _descriptorSet->update(_currentBinding);
}

rhi::DescriptorSetPtr BindGroup::descriptorSet() const {
    return _descriptorSet;
}

void BindGroup::bindBuffer(std::string_view name, uint32_t index, rhi::BufferPtr buffer) {
    if(name.empty()) return;
    auto bindingSlot = _bindingMap.at(name.data());
    auto bindingIndex = _updateIndices[bindingSlot];

    auto& currentBinding = _currentBinding.bufferBindings[bindingIndex];
    if(currentBinding.buffers[index].buffer != buffer.get()) {
        currentBinding.buffers[index] = {
            .offset = 0,
            .size = buffer->info().size,
            .buffer = buffer.get(),
        };
        _updateInfo.bufferBindings.emplace_back(currentBinding);
    }
}

void BindGroup::bindBuffer(std::string_view name, uint32_t index, uint32_t offset, uint32_t size, rhi::BufferPtr buffer) {
    if(name.empty()) return;
    auto bindingSlot = _bindingMap.at(name.data());
    auto bindingIndex = _updateIndices[bindingSlot];

    auto& currentBinding = _currentBinding.bufferBindings[bindingIndex];
    if(currentBinding.buffers[index].buffer != buffer.get() ||
        currentBinding.buffers[index].offset != offset ||
        currentBinding.buffers[index].size != size) {
        currentBinding.buffers[index] = {
            .offset = offset,
            .size = size,
            .buffer = buffer.get(),
        };
        _updateInfo.bufferBindings.emplace_back(currentBinding);
    }
}

void BindGroup::bindImage(std::string_view name, uint32_t index, rhi::ImageViewPtr imgView, rhi::ImageLayout layout) {
    if(name.empty()) return;
    auto bindingSlot = _bindingMap.at(name.data());
    auto bindingIndex = _updateIndices[bindingSlot];

    auto& currentBinding = _currentBinding.imageBindings[bindingIndex];
    if(currentBinding.imageViews[index].imageView != imgView.get()) {
        currentBinding.imageViews[index] = {
            .layout = layout,
            .imageView = imgView.get(),
        };
        _updateInfo.imageBindings.emplace_back(currentBinding);
    }
}

void BindGroup::bindSampler(std::string_view name, uint32_t index, const rhi::SamplerInfo& samplerInfo) {
    if(name.empty()) return;
    auto bindingSlot = _bindingMap.at(name.data());
    auto bindingIndex = _updateIndices[bindingSlot];

    auto& currentBinding = _currentBinding.samplerBindings[bindingIndex];
    auto* sampler = _device->getSampler(samplerInfo);
    if(currentBinding.samplers[index] != sampler) {
        currentBinding.samplers[index] = sampler;
        _updateInfo.samplerBindings.emplace_back(currentBinding);
    }
}

void BindGroup::bindTexelBuffer(std::string_view name, uint32_t index, rhi::BufferViewPtr bufferView) {
    if(name.empty()) return;
    auto bindingSlot = _bindingMap.at(name.data());
    auto bindingIndex = _updateIndices[bindingSlot];

    auto& currentBinding = _currentBinding.texelBufferBindings[bindingIndex];
    if(currentBinding.bufferViews[index] != bufferView.get()) {
        currentBinding.bufferViews[index] = bufferView.get();
        _updateInfo.texelBufferBindings.emplace_back(currentBinding);
    }
}

void BindGroup::update() {
    for(auto& bufferBinding : _updateInfo.bufferBindings) {
        _descriptorSet->updateBuffer(bufferBinding);
    }
    for(auto& imageBinding  : _updateInfo.imageBindings) {
        _descriptorSet->updateImage(imageBinding);
    }
    for(auto& samplerBinding : _updateInfo.samplerBindings) {
        _descriptorSet->updateSampler(samplerBinding);
    }
    for(auto& texelBufferBinding : _updateInfo.texelBufferBindings) {
        _descriptorSet->updateTexelBuffer(texelBufferBinding);
    }
    _updateInfo.bufferBindings.clear();
    _updateInfo.imageBindings.clear();
    _updateInfo.samplerBindings.clear();
    _updateInfo.texelBufferBindings.clear();
}

}




