#pragma once
#include "RHISemaphore.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class Semaphore : public RHISemaphore {
public:
    explicit Semaphore(Device* device);
    ~Semaphore();

    void setStage(PipelineStage stage) override;
    PipelineStage getStage() override;

    VkSemaphore semaphore() const { return _sem; }
private:
    VkSemaphore _sem{VK_NULL_HANDLE};
    PipelineStage _stage{PipelineStage::TOP_OF_PIPE};
    Device* _device{nullptr};
};

}