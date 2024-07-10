#include "VKSemaphore.h"
#include "VKDevice.h"
namespace raum::rhi {

Semaphore::Semaphore(Device *device): RHISemaphore(device) {
    VkSemaphoreCreateInfo sci;
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    auto res = vkCreateSemaphore(_device->device(), &sci, nullptr, &_sem);
    raum_check(res == VK_SUCCESS, "failed to create semaphore!");
}

Semaphore::~Semaphore() noexcept {
    vkDestroySemaphore(_device->device(), _sem, nullptr);
}

}
