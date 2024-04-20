#include "VKDescriptorSet.h"
#include "VKDevice.h"
#include "VKDescriptorPool.h"
#include "VKDescriptorSetLayout.h"
#include "VKUtils.h"
#include "VKBuffer.h"
#include "VkBufferView.h"
#include "VKImageView.h"
#include "VKSampler.h"
namespace raum::rhi {
DescriptorSet::DescriptorSet(const DescriptorSetInfo& info, DescriptorPool* pool, RHIDevice* device)
: RHIDescriptorSet(info, device), _info(info),
  _device(static_cast<Device*>(device)),
  _descriptorPool(pool) {
    
    auto* descriptorSetLayout = static_cast<DescriptorSetLayout*>(info.layout);
    auto kLayout = descriptorSetLayout->layout();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool->descriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &kLayout;

    vkAllocateDescriptorSets(_device->device(), &allocInfo, &_descriptorSet);

    //update(info.bindingInfos);
}

void DescriptorSet::update(const BindingInfo& bindingInfo) {
    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> buffers(bindingInfo.bufferBindings.size());
    std::vector<VkDescriptorImageInfo> images(bindingInfo.imageBindings.size());
    std::vector<VkDescriptorImageInfo> samplers(bindingInfo.samplerBindings.size());
    std::vector<VkBufferView> texelBuffers(bindingInfo.texelBufferBindings.size());
    for (const auto& bufferBinding : bindingInfo.bufferBindings) {
        auto& write = writes.emplace_back();
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _descriptorSet;
        write.dstBinding = bufferBinding.binding;
        write.dstArrayElement = bufferBinding.arrayElement;
        write.descriptorCount = static_cast<uint32_t>(bufferBinding.buffers.size());
        write.descriptorType = descriptorType(bufferBinding.type);
        for (const auto& v : bufferBinding.buffers) {
            auto& dbInfo = buffers[&v - &bufferBinding.buffers[0]];
            dbInfo.buffer = static_cast<Buffer*>(v.buffer)->buffer();
            dbInfo.offset = v.offset;
            dbInfo.range = v.size;
        }
        write.pBufferInfo = &buffers[0];
    }

    for (const auto& imageBinding : bindingInfo.imageBindings) {
        auto& write = writes.emplace_back();
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _descriptorSet;
        write.dstBinding = imageBinding.binding;
        write.dstArrayElement = imageBinding.arrayElement;
        write.descriptorCount = static_cast<uint32_t>(imageBinding.imageViews.size());
        write.descriptorType = descriptorType(imageBinding.type);

        for (const auto& v : imageBinding.imageViews) {
            auto& diInfo = images[&v - &imageBinding.imageViews[0]];
            diInfo.imageView = static_cast<ImageView*>(v.imageView)->imageView();
            diInfo.imageLayout = imageLayout(v.layout);
        }
        write.pImageInfo = &images[0];
    }

    for (const auto& samplerBinding : bindingInfo.samplerBindings) {
        auto& write = writes.emplace_back();
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _descriptorSet;
        write.dstBinding = samplerBinding.binding;
        write.dstArrayElement = samplerBinding.arrayElement;
        write.descriptorCount = static_cast<uint32_t>(samplerBinding.samplers.size());
        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        for (const auto& s : samplerBinding.samplers) {
            auto& dsInfo = samplers[&s - &samplerBinding.samplers[0]];
            dsInfo.sampler = static_cast<Sampler*>(s)->sampler();
        }
        write.pImageInfo = &samplers[0];
    }

    for (const auto& texelBufferBinding : bindingInfo.texelBufferBindings) {
        auto& write = writes.emplace_back();
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _descriptorSet;
        write.dstBinding = texelBufferBinding.binding;
        write.dstArrayElement = texelBufferBinding.arrayElement;
        write.descriptorCount = static_cast<uint32_t>(texelBufferBinding.bufferViews.size());
        for (const auto& bfv : texelBufferBinding.bufferViews) {
            texelBuffers[&bfv - &texelBufferBinding.bufferViews[0]] = static_cast<BufferView*>(bfv)->bufferView();
        }
        write.pTexelBufferView = &texelBuffers[0];
    }

    vkUpdateDescriptorSets(_device->device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}


void DescriptorSet::updateBuffer(const BufferBinding& info) {
    std::vector<VkDescriptorBufferInfo> buffers;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = info.binding;
    write.dstArrayElement = info.arrayElement;
    write.descriptorCount = static_cast<uint32_t>(info.buffers.size());
    write.descriptorType = descriptorType(info.type);
    for (const auto& v : info.buffers) {
        auto& dbInfo = buffers.emplace_back();
        dbInfo.buffer = static_cast<Buffer*>(v.buffer)->buffer();
        dbInfo.offset = v.offset;
        dbInfo.range = v.size;
    }
    write.pBufferInfo = buffers.data();
    vkUpdateDescriptorSets(_device->device(), 1, &write, 0, nullptr);
}

void DescriptorSet::updateImage(const ImageBinding& info) {
    std::vector<VkDescriptorImageInfo> images;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = info.binding;
    write.dstArrayElement = info.arrayElement;
    write.descriptorCount = static_cast<uint32_t>(info.imageViews.size());
    write.descriptorType = descriptorType(info.type);

    for (const auto& v : info.imageViews) {
        auto& diInfo = images.emplace_back();
        diInfo.imageView = static_cast<ImageView*>(v.imageView)->imageView();
        diInfo.imageLayout = imageLayout(v.layout);
    }
    write.pImageInfo = images.data();
    vkUpdateDescriptorSets(_device->device(), 1, &write, 0, nullptr);
}

void DescriptorSet::updateSampler(const SamplerBinding& info) {
    std::vector<VkDescriptorImageInfo> samplers;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = info.binding;
    write.dstArrayElement = info.arrayElement;
    write.descriptorCount = static_cast<uint32_t>(info.samplers.size());
    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    for (const auto& s : info.samplers) {
        auto& dsInfo = samplers.emplace_back();
        dsInfo.sampler = static_cast<Sampler*>(s)->sampler();
    }
    write.pImageInfo = samplers.data();
    vkUpdateDescriptorSets(_device->device(), 1, &write, 0, nullptr);
}

void DescriptorSet::updateTexelBuffer(const TexelBufferBinding& info) {
    std::vector<VkBufferView> texelBuffers;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = _descriptorSet;
    write.dstBinding = info.binding;
    write.dstArrayElement = info.arrayElement;
    write.descriptorCount = static_cast<uint32_t>(info.bufferViews.size());
    for (const auto& bfv : info.bufferViews) {
        texelBuffers.emplace_back(static_cast<BufferView*>(bfv)->bufferView());
    }
    write.pTexelBufferView = texelBuffers.data();
    vkUpdateDescriptorSets(_device->device(), 1, &write, 0, nullptr);
}

DescriptorSet::~DescriptorSet() {
    vkFreeDescriptorSets(_device->device(), _descriptorPool->descriptorPool(), 1, &_descriptorSet);
}

}