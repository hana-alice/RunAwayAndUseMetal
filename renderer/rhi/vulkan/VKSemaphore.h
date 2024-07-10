#pragma once
#include "RHISemaphore.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class Semaphore : public RHISemaphore {
public:
    explicit Semaphore(Device* device);
    ~Semaphore();

    VkSemaphore semaphore() const { return _sem; }
private:
    VkSemaphore _sem{VK_NULL_HANDLE};
    Device* _device{nullptr};
};

}