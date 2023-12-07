#pragma once
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <optional>
#include "define.h"
namespace raum::rhi {

static constexpr uint32_t SwapchainCount{3};

enum class QueueType : uint32_t {
    GRAPHICS,
    COMPUTE,
    TRANSFER,
};

struct QueueInfo {
    QueueType type;
};

enum class SyncType : uint32_t {
    IMMEDIATE,
    VSYNC,
    RELAX,
    MAILBOX,
};

struct SwapchainInfo {
    uint32_t width{0};
    uint32_t height{0};
    SyncType type;
    void* hwnd;
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;
};

struct DeviceInfo {
    // void* hwnd;
};
} // namespace raum::rhi