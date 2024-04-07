#include "Technique.h"

namespace raum::scene {

Technique::Technique(MaterialPtr material, raum::scene::PhasePtr phase)
:_material(material), _phase(phase){
}

MaterialPtr Technique::material() {
    return _material;
}

PhasePtr Technique::phase() {
    return _phase;
}

void Technique::bakePipelineStateObject(rhi::GraphicsPipelinePtr pso) {
    _pso = pso;
}

rhi::GraphicsPipelinePtr Technique::pipelineStateObject() const {
    return _pso;
}

}
