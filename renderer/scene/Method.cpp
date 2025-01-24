#include "Method.h"
#include <algorithm>
#include <boost/functional/hash.hpp>
#include "RHIComputePipeline.h"
#include "RHIUtils.h"

namespace raum::scene {

namespace {
std::unordered_map<std::size_t, rhi::ShaderPtr> _shaderMap;
std::unordered_map<rhi::ComputePipelineInfo, rhi::ComputePipelinePtr, rhi::RHIHash<rhi::ComputePipelineInfo>> _psoMap;
flat_map<size_t, MethodPtr> _methods;
} // namespace

void Method::bakeBindGroup(const SlotMap& perPassBinding,
                           const SlotMap& perBatchBinding,
                           rhi::DescriptorSetLayoutPtr passLayout,
                           rhi::DescriptorSetLayoutPtr batchLayout,
                           rhi::DevicePtr device) {
    if (!perPassBinding.empty()) {
        _passBindGroup = std::make_shared<BindGroup>(perPassBinding, passLayout, device);
    }

    if (!perBatchBinding.empty()) {
        _batchBindGroup = std::make_shared<BindGroup>(perBatchBinding, batchLayout, device);
    }
}

void Method::bakePipeline(rhi::DescriptorSetLayoutPtr passDescriptorSet,
                          rhi::DescriptorSetLayoutPtr batchDescriptorSet,
                          const std::vector<rhi::PushConstantRange>& constants,
                          const flat_map<rhi::ShaderStage, std::string>& shaderIn,
                          rhi::DevicePtr device) {
    raum_check(shaderIn.size() == 1, "Compute pipeline only support one shader.");
    size_t hash = std::hash<std::string>{}(_programName);
    size_t seed = 9527;
    std::string prefix = "#version 450 core\n";

    std::ranges::for_each(_defines, [&seed, &prefix](const std::string& s) {
        prefix.append("#define " + s + '\n');
    });
    boost::hash_combine(seed, _programName);
    boost::hash_combine(seed, prefix);
    rhi::ShaderPtr shader;
    if (!_shaderMap.contains(seed)) {
        const auto& [stage, source] = *shaderIn.begin();
        raum_check(stage == rhi::ShaderStage::COMPUTE, "Compute pipeline only support compute shader.");
        rhi::ShaderSourceInfo info{
            _programName,
            {rhi::ShaderStage::COMPUTE, prefix + source},
        };

        _shaderMap.emplace(seed, rhi::ShaderPtr(device->createShader(info)));
    }
    shader = _shaderMap.at(seed);

    _bindingBound[0] = passDescriptorSet && !passDescriptorSet->info().descriptorBindings.empty();
    _bindingBound[1] = batchDescriptorSet && !batchDescriptorSet->info().descriptorBindings.empty();

    std::vector<rhi::RHIDescriptorSetLayout*> setLayouts = {
        passDescriptorSet.get(),
        batchDescriptorSet.get(),
    };

    rhi::PipelineLayoutInfo layoutInfo = {
        constants,
        setLayouts,
    };
    auto pplLayout = rhi::getOrCreatePipelineLayout(layoutInfo, device);

    rhi::ComputePipelineInfo info{
        .pipelineLayout = pplLayout.get(),
        .shader = shader.get(),
    };

    if (!_psoMap.contains(info)) {
        _psoMap.emplace(info, rhi::ComputePipelinePtr(device->createComputePipeline(info)));
    }
    _pso = _psoMap.at(info);
}

bool Method::hasPassBinding() const {
    return _bindingBound[0];
}

bool Method::hasBatchBinding() const {
    return _bindingBound[1];
}

rhi::ComputePipelinePtr Method::pipelineState() {
    return _pso;
}

void Method::update() {
    if (_passBindGroup) {
        _passBindGroup->update();
    }
    if (_batchBindGroup) {
        _batchBindGroup->update();
    }
}

BindGroupPtr Method::perPassBindGroup() const {
    return _passBindGroup;
}

BindGroupPtr Method::perBatchBindGroup() const {
    return _batchBindGroup;
}

Method::Pool& Method::pool() {
    static Method::Pool pool;
    return pool;
}

MethodPtr Method::Pool::makeMethod(std::string_view programName, flat_set<std::string> defines) {
    size_t seed = 9527;
    boost::hash_combine(seed, programName);
    boost::hash_combine(seed, defines);
    auto* t = new Method(programName, defines);
    if (!_methods.contains(seed)) {
        _methods.emplace(seed, MethodPtr(new Method(programName, defines)));
    }
    return _methods.at(seed);
}

} // namespace raum::scene