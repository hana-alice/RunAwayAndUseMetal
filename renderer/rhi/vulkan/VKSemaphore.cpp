#include "VKSemaphore.h"
#include "VKDevice.h"
namespace raum::rhi {

Semaphore::Semaphore(Device *device): RHISemaphore(device), _device(device) {
    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sci.pNext = nullptr;

    auto res = vkCreateSemaphore(_device->device(), &sci, nullptr, &_sem);
    raum_check(res == VK_SUCCESS, "failed to create semaphore!");
}

void Semaphore::setStage(PipelineStage stage) {
    _stage = stage;
}

PipelineStage Semaphore::getStage() {
    return _stage;
}

Semaphore::~Semaphore() noexcept {
    vkDestroySemaphore(_device->device(), _sem, nullptr);
}

}
