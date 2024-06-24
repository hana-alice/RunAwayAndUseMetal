#include "RHIUtils.h"
#include <boost/functional/hash.hpp>
#include <unordered_set>
#include "RHIBufferView.h"
#include "RHIDefine.h"
#include "RHIDevice.h"
#include "RHICommandBuffer.h"
#include "RHIBlitEncoder.h"

namespace raum::rhi {
bool operator==(const SamplerInfo& lhs, const SamplerInfo& rhs) {
    return lhs.anisotropyEnable == rhs.anisotropyEnable &&
           lhs.compareEnable == rhs.compareEnable &&
           lhs.unnormalizedCoordinates == rhs.unnormalizedCoordinates &&
           lhs.magFilter == rhs.magFilter &&
           lhs.minFilter == rhs.minFilter &&
           lhs.mipmapMode == rhs.mipmapMode &&
           lhs.addressModeU == rhs.addressModeU &&
           lhs.addressModeV == rhs.addressModeV &&
           lhs.addressModeW == rhs.addressModeW &&
           lhs.mipLodBias == rhs.mipLodBias &&
           lhs.maxAnisotropy == rhs.maxAnisotropy &&
           lhs.compareOp == rhs.compareOp &&
           lhs.minLod == rhs.minLod &&
           lhs.maxLod == rhs.maxLod &&
           lhs.borderColor == rhs.borderColor;
}
std::size_t hash_value(const SamplerInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.addressModeU);
    boost::hash_combine(seed, info.addressModeV);
    boost::hash_combine(seed, info.addressModeW);
    boost::hash_combine(seed, info.borderColor);
    boost::hash_combine(seed, info.compareOp);
    boost::hash_combine(seed, info.compareEnable);
    boost::hash_combine(seed, info.magFilter);
    boost::hash_combine(seed, info.maxAnisotropy);
    boost::hash_combine(seed, info.maxLod);
    boost::hash_combine(seed, info.minFilter);
    boost::hash_combine(seed, info.minLod);
    boost::hash_combine(seed, info.mipmapMode);
    boost::hash_combine(seed, info.mipLodBias);
    boost::hash_combine(seed, info.unnormalizedCoordinates);
    boost::hash_combine(seed, info.anisotropyEnable);
    return seed;
}

bool operator==(const AttachmentReference& lhs, const AttachmentReference& rhs) {
    return lhs.index == rhs.index && lhs.layout == rhs.layout;
}
size_t hash_value(const AttachmentReference& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.index);
    boost::hash_combine(seed, info.layout);
    return seed;
}

bool operator==(const AttachmentInfo& lhs, const AttachmentInfo& rhs) {
    return lhs.loadOp == rhs.loadOp && lhs.storeOp == rhs.storeOp && lhs.stencilLoadOp == rhs.stencilLoadOp && lhs.stencilStoreOp == rhs.stencilStoreOp && lhs.format == rhs.format && lhs.sampleCount == rhs.sampleCount && lhs.initialLayout == rhs.initialLayout && lhs.finalLayout == rhs.finalLayout;
};
size_t hash_value(const AttachmentInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.loadOp);
    boost::hash_combine(seed, info.storeOp);
    boost::hash_combine(seed, info.stencilLoadOp);
    boost::hash_combine(seed, info.stencilStoreOp);
    boost::hash_combine(seed, info.format);
    boost::hash_combine(seed, info.sampleCount);
    boost::hash_combine(seed, info.initialLayout);
    boost::hash_combine(seed, info.finalLayout);
    return seed;
}

bool operator==(const SubpassInfo& lhs, const SubpassInfo& rhs) {
    return lhs.colors == rhs.colors && lhs.depthStencil == rhs.depthStencil && lhs.inputs == rhs.inputs && lhs.preserves == rhs.preserves && lhs.resolves == rhs.resolves;
}
size_t hash_value(const SubpassInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.colors);
    boost::hash_combine(seed, info.depthStencil);
    boost::hash_combine(seed, info.inputs);
    boost::hash_combine(seed, info.preserves);
    boost::hash_combine(seed, info.resolves);
    return seed;
}

bool operator==(const SubpassDependency& lhs, const SubpassDependency& rhs) {
    return lhs.dependencyFlags == rhs.dependencyFlags && lhs.dst == rhs.dst && lhs.src == rhs.src && lhs.srcStage == rhs.srcStage && lhs.dstStage == rhs.dstStage &&
           lhs.srcAccessFlags == rhs.dstAccessFlags;
}
size_t hash_value(const SubpassDependency& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.dependencyFlags);
    boost::hash_combine(seed, info.src);
    boost::hash_combine(seed, info.dst);
    boost::hash_combine(seed, info.srcStage);
    boost::hash_combine(seed, info.dstStage);
    boost::hash_combine(seed, info.srcAccessFlags);
    boost::hash_combine(seed, info.dstAccessFlags);
    return seed;
}

bool operator==(const RenderPassInfo& lhs, const RenderPassInfo& rhs) {
    return lhs.attachments == rhs.attachments && lhs.subpasses == rhs.subpasses && lhs.dependencies == rhs.dependencies;
}
std::size_t hash_value(const RenderPassInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.attachments);
    boost::hash_combine(seed, info.subpasses);
    boost::hash_combine(seed, info.dependencies);
    return seed;
}

bool operator==(const FrameBufferInfo& lhs, const FrameBufferInfo& rhs) {
    bool imageSame = lhs.images == rhs.images;
    for (size_t i = 0; i < lhs.images.size() && imageSame; ++i) {
        imageSame &= (lhs.images[i]->objectID() == rhs.images[i]->objectID());
    }
    return lhs.renderPass == rhs.renderPass && lhs.width == rhs.width && lhs.height == rhs.height && lhs.layers == rhs.layers && imageSame;
}

std::size_t hash_value(const FrameBufferInfo& info) {
    size_t seed = info.images.size();
    boost::hash_combine(seed, info.renderPass);
    boost::hash_combine(seed, info.width);
    boost::hash_combine(seed, info.height);
    boost::hash_combine(seed, info.layers);
    boost::hash_combine(seed, info.images);
    for (const auto* imgView : info.images) {
        boost::hash_combine(seed, imgView->objectID());
    }
    return seed;
}

bool operator==(const DescriptorBinding& lhs, const DescriptorBinding& rhs) {
    return lhs.type == rhs.type &&
           lhs.binding == rhs.binding &&
           lhs.count == rhs.count &&
           lhs.visibility == rhs.visibility &&
           lhs.immutableSamplers == rhs.immutableSamplers;
}
std::size_t hash_value(const DescriptorBinding& binding) {
    size_t seed = 9527;
    boost::hash_combine(seed, binding.type);
    boost::hash_combine(seed, binding.binding);
    boost::hash_combine(seed, binding.count);
    boost::hash_combine(seed, binding.visibility);
    boost::hash_combine(seed, binding.immutableSamplers);
    return seed;
}

bool operator==(const DescriptorSetLayoutInfo& lhs, const DescriptorSetLayoutInfo& rhs) {
    return lhs.descriptorBindings == rhs.descriptorBindings;
}
std::size_t hash_value(const DescriptorSetLayoutInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.descriptorBindings);
    return seed;
}

bool operator==(const PushConstantRange& lhs, const PushConstantRange& rhs) {
    return lhs.stage == rhs.stage &&
           lhs.size == rhs.size &&
           lhs.offset == rhs.offset;
}
std::size_t hash_value(const PushConstantRange& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.stage);
    boost::hash_combine(seed, info.size);
    boost::hash_combine(seed, info.offset);
    return seed;
}

bool operator==(const VertexAttribute& lhs, const VertexAttribute& rhs) {
    return lhs.offset == rhs.offset &&
           lhs.binding == rhs.binding &&
           lhs.format == rhs.format &&
           lhs.location == rhs.location;
}
std::size_t hash_value(const VertexAttribute& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.offset);
    boost::hash_combine(seed, info.binding);
    boost::hash_combine(seed, info.format);
    boost::hash_combine(seed, info.location);
    return seed;
}

bool operator==(const VertexBufferAttribute& lhs, const VertexBufferAttribute& rhs) {
    return lhs.binding == rhs.binding &&
           lhs.stride == rhs.stride &&
           lhs.rate == rhs.rate;
}
std::size_t hash_value(const VertexBufferAttribute& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.binding);
    boost::hash_combine(seed, info.stride);
    boost::hash_combine(seed, info.rate);
    return seed;
}

bool operator==(const VertexLayout& lhs, const VertexLayout& rhs) {
    return lhs.vertexAttrs == rhs.vertexAttrs &&
           lhs.vertexBufferAttrs == rhs.vertexBufferAttrs;
}
std::size_t hash_value(const VertexLayout& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.vertexAttrs);
    boost::hash_combine(seed, info.vertexBufferAttrs);
    return seed;
}

bool operator==(const PipelineLayoutInfo& lhs, const PipelineLayoutInfo& rhs) {
    return lhs.setLayouts == rhs.setLayouts &&
           lhs.pushConstantRanges == rhs.pushConstantRanges;
}
std::size_t hash_value(const PipelineLayoutInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.setLayouts);
    boost::hash_combine(seed, info.pushConstantRanges);
    return seed;
}

bool operator==(const RasterizationInfo& lhs, const RasterizationInfo& rhs) {
    return lhs.depthClamp == rhs.depthClamp &&
           lhs.depthBiasEnable == rhs.depthBiasEnable &&
           lhs.polygonMode == rhs.polygonMode &&
           lhs.cullMode == rhs.cullMode &&
           lhs.frontFace == rhs.frontFace &&
           lhs.depthBiasConstantFactor == rhs.depthBiasConstantFactor &&
           lhs.depthBiasClamp == rhs.depthBiasClamp &&
           lhs.depthBiasSlopeFactor == rhs.depthBiasSlopeFactor &&
           lhs.lineWidth == rhs.lineWidth;
}
std::size_t hash_value(const RasterizationInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.depthClamp);
    boost::hash_combine(seed, info.depthBiasEnable);
    boost::hash_combine(seed, info.polygonMode);
    boost::hash_combine(seed, info.cullMode);
    boost::hash_combine(seed, info.frontFace);
    boost::hash_combine(seed, info.depthBiasConstantFactor);
    boost::hash_combine(seed, info.depthBiasClamp);
    boost::hash_combine(seed, info.depthBiasSlopeFactor);
    boost::hash_combine(seed, info.lineWidth);
    return seed;
}

bool operator==(const MultisamplingInfo& lhs, const MultisamplingInfo& rhs) {
    return lhs.enable == rhs.enable &&
           lhs.sampleShadingEnable == rhs.sampleShadingEnable &&
           lhs.alphaToCoverageEnable == rhs.alphaToCoverageEnable &&
           lhs.minSampleShading == rhs.minSampleShading &&
           lhs.sampleCount == rhs.sampleCount &&
           lhs.sampleMask == rhs.sampleMask;
}
std::size_t hash_value(const MultisamplingInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.enable);
    boost::hash_combine(seed, info.sampleShadingEnable);
    boost::hash_combine(seed, info.alphaToCoverageEnable);
    boost::hash_combine(seed, info.minSampleShading);
    boost::hash_combine(seed, info.sampleCount);
    boost::hash_combine(seed, info.sampleMask);
    return seed;
}

bool operator==(const StencilInfo& lhs, const StencilInfo& rhs) {
    return lhs.failOp == rhs.failOp &&
           lhs.passOp == rhs.passOp &&
           lhs.depthFailOp == rhs.depthFailOp &&
           lhs.compareOp == rhs.compareOp &&
           lhs.compareMask == rhs.compareMask &&
           lhs.writeMask == rhs.writeMask &&
           lhs.reference == rhs.reference;
}
std::size_t hash_value(const StencilInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.failOp);
    boost::hash_combine(seed, info.passOp);
    boost::hash_combine(seed, info.depthFailOp);
    boost::hash_combine(seed, info.compareOp);
    boost::hash_combine(seed, info.compareMask);
    boost::hash_combine(seed, info.writeMask);
    boost::hash_combine(seed, info.reference);
    return seed;
}

bool operator==(const DepthStencilInfo& lhs, const DepthStencilInfo& rhs) {
    return lhs.depthTestEnable == rhs.depthTestEnable &&
           lhs.depthWriteEnable == rhs.depthWriteEnable &&
           lhs.depthBoundsTestEnable == rhs.depthBoundsTestEnable &&
           lhs.stencilTestEnable == rhs.stencilTestEnable &&
           lhs.depthCompareOp == rhs.depthCompareOp &&
           lhs.front == rhs.front &&
           lhs.back == rhs.back &&
           lhs.minDepthBounds == rhs.minDepthBounds &&
           lhs.maxDepthBounds == rhs.maxDepthBounds;
}
std::size_t hash_value(const DepthStencilInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.depthTestEnable);
    boost::hash_combine(seed, info.depthWriteEnable);
    boost::hash_combine(seed, info.depthBoundsTestEnable);
    boost::hash_combine(seed, info.stencilTestEnable);
    boost::hash_combine(seed, info.depthCompareOp);
    boost::hash_combine(seed, info.front);
    boost::hash_combine(seed, info.back);
    boost::hash_combine(seed, info.minDepthBounds);
    boost::hash_combine(seed, info.maxDepthBounds);
    return seed;
}

bool operator==(const AttachmentBlendInfo& lhs, const AttachmentBlendInfo& rhs) {
    return lhs.blendEnable == rhs.blendEnable &&
           lhs.srcColorBlendFactor == rhs.srcColorBlendFactor &&
           lhs.dstColorBlendFactor == rhs.dstColorBlendFactor &&
           lhs.colorBlendOp == rhs.colorBlendOp &&
           lhs.srcAlphaBlendFactor == rhs.srcAlphaBlendFactor &&
           lhs.dstAlphaBlendFactor == rhs.dstAlphaBlendFactor &&
           lhs.alphaBlendOp == rhs.alphaBlendOp &&
           lhs.writemask == rhs.writemask;
}
std::size_t hash_value(const AttachmentBlendInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.blendEnable);
    boost::hash_combine(seed, info.srcColorBlendFactor);
    boost::hash_combine(seed, info.dstColorBlendFactor);
    boost::hash_combine(seed, info.colorBlendOp);
    boost::hash_combine(seed, info.srcAlphaBlendFactor);
    boost::hash_combine(seed, info.dstAlphaBlendFactor);
    boost::hash_combine(seed, info.alphaBlendOp);
    boost::hash_combine(seed, info.writemask);
    return seed;
}

bool operator==(const BlendInfo& lhs, const BlendInfo& rhs) {
    return lhs.logicOpEnable == rhs.logicOpEnable &&
           lhs.logicOp == rhs.logicOp &&
           lhs.attachmentBlends == rhs.attachmentBlends &&
           std::equal(std::begin(lhs.blendConstants), std::end(lhs.blendConstants), std::begin(rhs.blendConstants), std::end(rhs.blendConstants));
}
std::size_t hash_value(const BlendInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.logicOpEnable);
    boost::hash_combine(seed, info.logicOp);
    boost::hash_combine(seed, info.attachmentBlends);
    for (const auto& v : info.blendConstants) {
        boost::hash_combine(seed, v);
    }
    return seed;
}

bool operator==(const GraphicsPipelineInfo& lhs, const GraphicsPipelineInfo& rhs) {
    return lhs.primitiveType == rhs.primitiveType &&
           lhs.pipelineLayout == rhs.pipelineLayout &&
           lhs.renderPass == rhs.renderPass &&
           lhs.shaders == rhs.shaders &&
           lhs.subpassIndex == rhs.subpassIndex &&
           lhs.viewportCount == rhs.viewportCount &&
           lhs.vertexLayout == rhs.vertexLayout &&
           lhs.rasterizationInfo == rhs.rasterizationInfo &&
           lhs.multisamplingInfo == rhs.multisamplingInfo &&
           lhs.depthStencilInfo == rhs.depthStencilInfo &&
           lhs.colorBlendInfo == rhs.colorBlendInfo;
}
std::size_t hash_value(const GraphicsPipelineInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.primitiveType);
    boost::hash_combine(seed, info.pipelineLayout);
    boost::hash_combine(seed, info.renderPass);
    boost::hash_combine(seed, info.shaders);
    boost::hash_combine(seed, info.subpassIndex);
    boost::hash_combine(seed, info.viewportCount);
    boost::hash_combine(seed, info.vertexLayout);
    boost::hash_combine(seed, info.rasterizationInfo);
    boost::hash_combine(seed, info.multisamplingInfo);
    boost::hash_combine(seed, info.depthStencilInfo);
    boost::hash_combine(seed, info.colorBlendInfo);
    return seed;
}

namespace {
std::unordered_map<DescriptorSetLayoutInfo, DescriptorSetLayoutPtr, RHIHash<DescriptorSetLayoutInfo>> _descriptorsetLayoutMap;
std::unordered_map<PipelineLayoutInfo, PipelineLayoutPtr, RHIHash<PipelineLayoutInfo>> _pplLayoutMap;
} // namespace

DescriptorSetLayoutPtr getOrCreateDescriptorSetLayout(const DescriptorSetLayoutInfo& info, DevicePtr device) {
    if (!_descriptorsetLayoutMap.contains(info)) {
        _descriptorsetLayoutMap[info] = DescriptorSetLayoutPtr(device->createDescriptorSetLayout(info));
    }
    return _descriptorsetLayoutMap.at(info);
}

PipelineLayoutPtr getOrCreatePipelineLayout(const PipelineLayoutInfo& info, DevicePtr device) {
    if (!_pplLayoutMap.contains(info)) {
        _pplLayoutMap[info] = PipelineLayoutPtr(device->createPipelineLayout(info));
    }
    return _pplLayoutMap.at(info);
}

const std::unordered_set<Format> depthFormats{
    Format::D16_UNORM,
    Format::X8_D24_UNORM_PACK32,
    Format::D32_SFLOAT,
    Format::D16_UNORM_S8_UINT,
    Format::D24_UNORM_S8_UINT,
    Format::D32_SFLOAT_S8_UINT,
};
const std::unordered_set<Format> stencilFormats{
    Format::X8_D24_UNORM_PACK32,
    Format::S8_UINT,
    Format::D16_UNORM_S8_UINT,
    Format::D24_UNORM_S8_UINT,
    Format::D32_SFLOAT_S8_UINT,
};

bool hasDepth(Format format) {
    return depthFormats.contains(format);
}

bool hasStencil(Format format) {
    return stencilFormats.contains(format);
}

namespace {
ImagePtr sampledImage;
ImagePtr storageImage;
ImageViewPtr sampledImageView;
ImageViewPtr storageImageView;
BufferPtr uniformBuffer;
BufferPtr storageBuffer;
BufferViewPtr uniformBufferView;
BufferViewPtr storageBufferView;

SamplerInfo sampler{
    .magFilter = Filter::LINEAR,
    .minFilter = Filter::LINEAR,
};
} // namespace
[[nodiscard]] ImagePtr defaultSampledImage(DevicePtr device) {
    if (!sampledImage) {
        ImageInfo info{
            .format = Format::RGBA8_UNORM,
            .extent = {2, 2, 1},
        };
        sampledImage = ImagePtr(device->createImage(info));
    }
    return sampledImage;
}
[[nodiscard]] ImagePtr defaultStorageImage(DevicePtr device) {
    if (!storageImage) {
        ImageInfo info{
            .usage = ImageUsage::STORAGE,
            .format = Format::RGBA8_UNORM,
            .extent = {2, 2, 1},
        };
        storageImage = ImagePtr(device->createImage(info));
    }
    return storageImage;
}

[[nodiscard]] ImageViewPtr defaultSampledImageView(DevicePtr device) {
    auto image = defaultSampledImage(device);
    if (!sampledImageView) {
        ImageViewInfo info{
            .image = image.get(),
            .range = {
                .aspect = AspectMask::COLOR,
                .sliceCount = 1,
                .mipCount = 1,
            },
            .format = image->info().format,
        };
        sampledImageView = ImageViewPtr(device->createImageView(info));
    }
    return sampledImageView;
}

[[nodiscard]] ImageViewPtr defaultStorageImageView(DevicePtr device) {
    auto image = defaultStorageImage(device);
    if (!storageImageView) {
        ImageViewInfo info{
            .image = image.get(),
            .range = {
                .aspect = AspectMask::COLOR,
                .sliceCount = 1,
                .mipCount = 1,
            },
            .format = image->info().format,
        };
        storageImageView = ImageViewPtr(device->createImageView(info));
    }
    return storageImageView;
}

[[nodiscard]] BufferPtr defaultUniformBuffer(DevicePtr device) {
    if (!uniformBuffer) {
        BufferInfo info{
            .size = 16,
        };
        uniformBuffer = BufferPtr(device->createBuffer(info));
    }
    return uniformBuffer;
}
[[nodiscard]] BufferPtr defaultStorageBuffer(DevicePtr device) {
    if (!storageBuffer) {
        BufferInfo info{
            .bufferUsage = BufferUsage::STORAGE,
            .size = 16,
        };
        storageBuffer = BufferPtr(device->createBuffer(info));
    }
    return storageBuffer;
}

[[nodiscard]] BufferViewPtr defaultUniformBufferView(DevicePtr device) {
    if (!uniformBufferView) {
        auto uniformBuffer = defaultUniformBuffer(device);
        BufferViewInfo info{
            .buffer = uniformBuffer.get(),
            .format = Format::RGBA8_UNORM,
            .offset = 0,
            .size = uniformBuffer->info().size,
        };
        uniformBufferView = BufferViewPtr(device->createBufferView(info));
    }
    return uniformBufferView;
}

[[nodiscard]] BufferViewPtr defaultStorageBufferView(DevicePtr device) {
    if (!storageBufferView) {
        auto storageBuffer = defaultStorageBuffer(device);
        BufferViewInfo info{
            .buffer = storageBuffer.get(),
            .format = Format::RGBA8_UNORM,
            .offset = 0,
            .size = storageBuffer->info().size,
        };
        storageBufferView = BufferViewPtr(device->createBufferView(info));
    }
    return storageBufferView;
}

[[nodiscard]] SamplerInfo defaultLinearSampler(DevicePtr device) {
    return sampler;
}

[[nodiscard]]ImagePtr createImageFromBuffer(BufferPtr buffer,
                                             uint32_t width,
                                             uint32_t height,
                                             Format format,
                                             CommandBufferPtr cmdBuffer,
                                             DevicePtr device) {
    ImageInfo imgInfo {
        .usage = ImageUsage::TRANSFER_DST | ImageUsage::SAMPLED,
        .format = format,
        .extent = {width, height, 1},
    };
    ImagePtr img = ImagePtr(device->createImage(imgInfo));
    ImageBarrierInfo transferDstBarrier{
        .image = img.get(),
        .dstStage = PipelineStage::TRANSFER,
        .newLayout = ImageLayout::TRANSFER_DST_OPTIMAL,
        .dstAccessFlag = AccessFlags::TRANSFER_WRITE,
        .range = {
            .sliceCount = 1,
            .mipCount = 1,
        }};
    cmdBuffer->appendImageBarrier(transferDstBarrier);
    cmdBuffer->applyBarrier({});

    auto blit = BlitEncoderPtr(cmdBuffer->makeBlitEncoder());
    BufferImageCopyRegion region{
        .bufferSize = buffer->info().size,
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageAspect = AspectMask::COLOR,
        .imageExtent = {
            width,
            height,
            1,
        },
    };
    blit->copyBufferToImage(buffer.get(),
                            img.get(),
                            ImageLayout::TRANSFER_DST_OPTIMAL,
                            &region,
                            1);
    ImageBarrierInfo imgBarrierInfo{
        .image = img.get(),
        .srcStage = PipelineStage::TRANSFER,
        .dstStage = PipelineStage::FRAGMENT_SHADER,
        .oldLayout = ImageLayout::TRANSFER_DST_OPTIMAL,
        .newLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        .srcAccessFlag = AccessFlags::TRANSFER_WRITE,
        .dstAccessFlag = AccessFlags::SHADER_READ,
        .range = {
            .sliceCount = 1,
            .mipCount = 1,
        }};
    cmdBuffer->appendImageBarrier(imgBarrierInfo);
    cmdBuffer->applyBarrier({});

    return img;
}

} // namespace raum::rhi