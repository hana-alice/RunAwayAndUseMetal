#include "VKSwapchain.h"
#ifdef WINDOWS
// clang-format off
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
// clang-format on
#endif
#include "VKDevice.h"
#include "VKQueue.h"
#include "log.h"

namespace raum::rhi {
Swapchain::Swapchain(const SwapchainInfo& info, Device* device) {
#ifdef WINDOWS
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = (HWND)info.hwnd;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);

    VkResult res = vkCreateWin32SurfaceKHR(device->instance(), &surfaceInfo, nullptr, &_surface);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create surface");
    VkBool32 support{false};
    vkGetPhysicalDeviceSurfaceSupportKHR(device->physicalDevice(), device->defaultQueue()->index(), _surface, &support);

    RAUM_CRITICAL_IF(!support, "surface presentation not supported");
#else
    #pragma error Run Away
#endif
}

Swapchain::~Swapchain() {
    vkDestroySurfaceKHR(Device::getInstance()->instance(), _surface, nullptr);
}
} // namespace raum::rhi