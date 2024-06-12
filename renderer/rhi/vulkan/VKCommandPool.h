#pragma once
#include "RHICommandPool.h"
#include "VKDefine.h"

namespace raum::rhi {
class Device;
class CommandPool :public RHICommandPool {
public:
    explicit CommandPool(const CommandPoolInfo&, RHIDevice* device);
    ~CommandPool() override;

    RHICommandBuffer* makeCommandBuffer(const CommandBufferInfo&) override;

    VkCommandPool commandPool() const { return _pool; }

private:
    Device* _device;
    VkCommandPool _pool;
};
}