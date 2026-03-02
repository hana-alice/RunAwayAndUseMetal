#pragma once
#include <vector>
#include "RHISwapchain.h"
#include "VKDefine.h"
#include <stack>
#include "VKSemaphore.h"

#include "VKDevice.h"
namespace raum::rhi {
class Device;
class ImageView;
class Image;
class Queue;

class SemaphorePool {
public:
    SemaphorePool() = default;
    SemaphorePool(const SemaphorePool&) = delete;
    SemaphorePool& operator=(const SemaphorePool&) = delete;
    SemaphorePool(SemaphorePool&&) = delete;
    SemaphorePool& operator=(SemaphorePool&&) = delete;

    Semaphore* allocate(Device* device) {
        Semaphore* sem = nullptr;
        if (_availables.empty()) {
            sem = static_cast<Semaphore*>(device->createSemaphore());
        } else {
            sem = _availables.top();
            _availables.pop();
        }
        return sem;
    }

    void dealloccate(Semaphore* semaphore) {
        _availables.push(semaphore);
    }

    ~SemaphorePool() {
        while (!_availables.empty()) {
            auto* sem = static_cast<Semaphore*>(_availables.top());
            delete sem;
            _availables.pop();
        }
    }

private:
    std::stack<Semaphore*> _availables;
};

class Swapchain : public RHISwapchain {
public:
    bool acquire() override;
    void present() override;
    RHIImage* allocateImage(uint32_t index) override;
    uint32_t imageCount() const override;
    uint32_t imageIndex() const override;
    uint32_t width() const override { return _info.width; }
    uint32_t height() const override { return _info.height; }
    Format format() const override { return _preferredFormat; }

    void resize(uint32_t w, uint32_t h) override;
    void resize(uint32_t w, uint32_t h, uintptr_t surface) override;

    bool imageValid(uint32_t index) override;
    bool holds(RHIImage* img) override;
    RHISemaphore* getAvailableSemaphore() override;
    RHISemaphore* getSignalPresentSemaphore() override;

private:
    Swapchain(const SwapchainSurfaceInfo& info, Device* device);
    Swapchain(const SwapchainInfo& info, Device* device);

    void destroy();

    ~Swapchain();

    void initialize(uintptr_t hwnd, SyncType type, uint32_t width, uint32_t height);

    SwapchainInfo _info;
    VkSurfaceKHR _surface{VK_NULL_HANDLE};
    VkSwapchainKHR _swapchain;
    Format _preferredFormat;
    Device* _device{nullptr};
    Queue* _presentQueue{nullptr};

    uint32_t _imageIndex{0};
    uint32_t _imageCount{0};
    std::vector<VkImage> _vkImages;
    std::vector<uint32_t> _valid;

    std::vector<Semaphore*> _acquireSemaphores;
    std::vector<Semaphore*> _readyPresentSemaphores;

    SemaphorePool _semaphorePool;

    friend class Device;
};

} // namespace raum::rhi