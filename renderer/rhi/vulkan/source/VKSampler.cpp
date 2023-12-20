#include "VKSampler.h"
#include "VKDevice.h"
#include "VKUtils.h"
namespace raum::rhi {
Sampler::Sampler(const SamplerInfo& samplerInfo, RHIDevice* device) 
	: RHISampler(samplerInfo, device),
	_device(static_cast<Device*>(device)) {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = mapFilter(samplerInfo.magFilter);
    info.minFilter = mapFilter(samplerInfo.minFilter);
    info.mipmapMode = samplerMipmapMode(samplerInfo.mipmapMode);
    info.addressModeU = samplerAddressMode(samplerInfo.addressModeU);
    info.addressModeV = samplerAddressMode(samplerInfo.addressModeV);
    info.addressModeW = samplerAddressMode(samplerInfo.addressModeW);
    info.mipLodBias = samplerInfo.mipLodBias;
    info.anisotropyEnable = samplerInfo.anisotropyEnable;
    info.maxAnisotropy = samplerInfo.maxAnisotropy;
    info.compareEnable = samplerInfo.compareEnable;
    info.compareOp = compareOp(samplerInfo.compareOp);
    info.minLod = samplerInfo.minLod;
    info.maxLod = samplerInfo.maxLod;
    info.borderColor = borderColor(samplerInfo.borderColor);
    info.unnormalizedCoordinates = samplerInfo.unnormalizedCoordinates;

    vkCreateSampler(_device->device(), &info, nullptr, &_sampler);
}

Sampler::~Sampler() {
    vkDestroySampler(_device->device(), _sampler, nullptr);
}

}