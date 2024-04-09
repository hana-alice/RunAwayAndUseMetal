#include "RHIUtils.h"
#include <boost/functional/hash.hpp>
#include "RHIDefine.h"

#define RHIHASHER(T)                                                        \
    template <>                                                             \
    struct RHIHash<T> {                                                     \
        size_t operator()(const T& info) const { return hash_value(info); } \
    };

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
RHIHASHER(SamplerInfo);

bool operator==(const AttachmentReference& lhs, const AttachmentReference& rhs) {
    return lhs.index == rhs.index && lhs.layout == rhs.layout;
}
size_t hash_value(const AttachmentReference& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.index);
    boost::hash_combine(seed, info.layout);
    return seed;
}
RHIHASHER(AttachmentReference)

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
RHIHASHER(AttachmentInfo);

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
RHIHASHER(SubpassInfo)

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
RHIHASHER(SubpassDependency)

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
RHIHASHER(RenderPassInfo)

bool operator==(const FrameBufferInfo& lhs, const FrameBufferInfo& rhs) {
    return lhs.renderPass == rhs.renderPass && lhs.width == rhs.width && lhs.height == rhs.height && lhs.layers == rhs.layers && lhs.images == rhs.images;
}
std::size_t hash_value(const FrameBufferInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.renderPass);
    boost::hash_combine(seed, info.width);
    boost::hash_combine(seed, info.height);
    boost::hash_combine(seed, info.layers);
    boost::hash_combine(seed, info.images);
    return seed;
}
RHIHASHER(FrameBufferInfo)

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
RHIHASHER(DescriptorBinding)

bool operator==(const DescriptorSetLayoutInfo& lhs, const DescriptorSetLayoutInfo& rhs) {
    return lhs.descriptorBindings == rhs.descriptorBindings;
}
std::size_t hash_value(const DescriptorSetLayoutInfo& info) {
    size_t seed = 9527;
    boost::hash_combine(seed, info.descriptorBindings);
    return seed;
}
RHIHASHER(DescriptorSetLayoutInfo)

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
RHIHASHER(PushConstantRange)

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
RHIHASHER(PipelineLayoutInfo)

} // namespace raum::rhi