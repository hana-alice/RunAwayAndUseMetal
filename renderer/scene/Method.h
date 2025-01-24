#pragma once
#include <string>
#include "BindGroup.h"

namespace raum::scene {
class Method;
using MethodPtr = std::shared_ptr<Method>;

class Method {
    Method(std::string_view programName, flat_set<std::string> defines) : _programName(programName), _defines(defines) {}

public:
    Method() = delete;

    const std::string& programName() const {
        return _programName;
    }

    const flat_set<std::string>& defines() const {
        return _defines;
    }

    void bakeBindGroup(const SlotMap& perPassBinding,
                       const SlotMap& perBatchBinding,
                       rhi::DescriptorSetLayoutPtr passLayout,
                       rhi::DescriptorSetLayoutPtr batchLayout,
                       rhi::DevicePtr device);

    void bakePipeline(rhi::DescriptorSetLayoutPtr descriptorSet,
                      rhi::DescriptorSetLayoutPtr batchDescriptorSet,
                      const std::vector<rhi::PushConstantRange>& constants,
                      const flat_map<rhi::ShaderStage, std::string>& shaderIn,
                      rhi::DevicePtr device);

    bool hasPassBinding() const;
    bool hasBatchBinding() const;

    BindGroupPtr perPassBindGroup() const;
    BindGroupPtr perBatchBindGroup() const;

    template <typename... Args>
    void bindBuffer(std::string_view name, Args&&... args) {
        if (_passBindGroup->contains(name)) {
            _passBindGroup->bindBuffer(name, std::forward<Args>(args)...);
        } else if (_batchBindGroup->contains(name)) {
            _batchBindGroup->bindBuffer(name, std::forward<Args>(args)...);
        } else {
            raum_unreachable();
        }
    }

    template <typename... Args>
    void bindImage(std::string_view name, Args&&... args) {
        if (_passBindGroup->contains(name)) {
            _passBindGroup->bindImage(name, std::forward<Args>(args)...);
        } else if (_batchBindGroup->contains(name)) {
            _batchBindGroup->bindImage(name, std::forward<Args>(args)...);
        } else {
            raum_unreachable();
        }
    }

    template <typename... Args>
    void bindSampler(std::string_view name, Args&&... args) {
        if (_passBindGroup->contains(name)) {
            _passBindGroup->bindSampler(name, std::forward<Args>(args)...);
        } else if (_batchBindGroup->contains(name)) {
            _batchBindGroup->bindSampler(name, std::forward<Args>(args)...);
        } else {
            raum_unreachable();
        }
    }

    template <typename... Args>
    void bindTexelBuffer(std::string_view name, Args&&... args) {
        if (_passBindGroup->contains(name)) {
            _passBindGroup->bindTexelBuffer(name, std::forward<Args>(args)...);
        } else if (_batchBindGroup->contains(name)) {
            _batchBindGroup->bindTexelBuffer(name, std::forward<Args>(args)...);
        } else {
            raum_unreachable();
        }
    }

    void update();

    rhi::ComputePipelinePtr pipelineState();

    class Pool {
    public:
        void clear();
        void shrink();
        MethodPtr makeMethod(std::string_view programName, flat_set<std::string> defines);
    };

    static Pool& pool();

    friend MethodPtr Pool::makeMethod(std::string_view programName, flat_set<std::string> defines);

private:


    std::string _programName;
    scene::BindGroupPtr _passBindGroup;
    scene::BindGroupPtr _batchBindGroup;
    rhi::ComputePipelinePtr _pso;
    const flat_set<std::string> _defines;
    std::array<int32_t, 2> _bindingBound;
};

} // namespace raum::scene