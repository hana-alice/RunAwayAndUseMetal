#pragma once
#include <memory>
#include "Phase.h"
#include "Material.h"

namespace raum::scene {

class Technique {
public:
    Technique() = delete;
    Technique(MaterialPtr material, PhasePtr phase);

    MaterialPtr material();
    PhasePtr phase();

    void bakePipelineStateObject(rhi::GraphicsPipelinePtr pso);
    rhi::GraphicsPipelinePtr pipelineStateObject() const;
private:
    MaterialPtr _material;
    PhasePtr _phase;
    rhi::GraphicsPipelinePtr _pso;
};
using TechniquePtr = std::shared_ptr<Technique>;

} // namespace raum::scene