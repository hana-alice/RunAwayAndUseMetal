#pragma once
#include <stdint.h>
#include <optional>
namespace raum::rhi {

struct QueueHandle {
    std::optional<uint32_t> index{0};
    VkQueue queue;
};

struct QueueFamilyIndices {
    QueueHandle graphicsFamily;
    QueueHandle computeFamily;
    QueueHandle transferFamily;
};
} // namespace raum::rhi