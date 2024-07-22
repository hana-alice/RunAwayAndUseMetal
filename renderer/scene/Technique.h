#pragma once
#include <memory>
#include "Material.h"

namespace raum::scene {

class Technique {
public:
    Technique() = delete;
    Technique(MaterialPtr material, std::string_view phaseName);

    MaterialPtr material();
    const std::string& phaseName() const;

    void setPrimitiveType(rhi::PrimitiveType type);
    rhi::PrimitiveType primitiveType() const;

    rhi::RasterizationInfo& rasterizationInfo();
    rhi::DepthStencilInfo& depthStencilInfo();
    rhi::BlendInfo& blendInfo();
    rhi::MultisamplingInfo& multisamplingInfo();

    rhi::GraphicsPipelinePtr pipelineState();

    void bakePipeline(rhi::RenderPassPtr renderpass,
                      rhi::DescriptorSetLayoutPtr passDescSet,
                      rhi::DescriptorSetLayoutPtr batchDescSet,
                      rhi::DescriptorSetLayoutPtr instDescSet,
                      rhi::DescriptorSetLayoutPtr drawDescSet,
                      const std::vector<rhi::PushConstantRange>& constants,
                      rhi::VertexLayout vertexLayout,
                      const boost::container::flat_map<rhi::ShaderStage, std::string>& shaderIn,
                      rhi::DevicePtr device);

    void bakeMaterial(const SlotMap& perBatchBinding,
                      rhi::DescriptorSetLayoutPtr batchLayout,
                      rhi::DevicePtr device);

    bool hasPassBinding() const;
    bool hasBatchBinding() const;
    bool hasInstanceBinding() const;
    bool hasDrawBinding() const;

private:
    std::string _phaseName;
    MaterialPtr _material;
    rhi::GraphicsPipelinePtr _pso;
    rhi::PrimitiveType _primitiveType{rhi::PrimitiveType::TRIANGLE_LIST};
    rhi::RasterizationInfo _rasterizationInfo;
    rhi::DepthStencilInfo _depthStencilInfo;
    rhi::BlendInfo _blendInfo;
    rhi::MultisamplingInfo _multisamplingInfo;
    std::vector<rhi::ShaderPtr> _shaders;
    std::array<int32_t, 4> _bindingBound;
    std::array<BindGroupPtr, 4> _bindGroups;
};
using TechniquePtr = std::shared_ptr<Technique>;

} // namespace raum::scene