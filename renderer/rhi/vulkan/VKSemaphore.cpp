#include "VKSemaphore.h"
#include <stdexcept>
#include "VKDevice.h"
namespace raum::rhi {

Semaphore::Semaphore(Device *device): RHISemaphore(device), _device(device) {

    VkSemaphoreCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    ci.pNext = nullptr;
    ci.flags = 0;
    auto res = vkCreateSemaphore(_device->device(), &ci, nullptr, &_sem);
    raum_check(res == VK_SUCCESS, "failed to create semaphore!");
    if (res != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan semaphore");
    }
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
