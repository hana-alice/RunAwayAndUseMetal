#pragma once
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vulkan/vulkan.h>
#include "RHIDefine.h"
namespace raum::rhi {
namespace detail {

[[noreturn]] inline void throwVkResult(VkResult result,
                                       std::string_view expression,
                                       std::source_location location) {
    std::string message{expression};
    message += " returned VkResult ";
    message += std::to_string(static_cast<int32_t>(result));
    message += " at ";
    message += location.file_name();
    message += ':';
    message += std::to_string(location.line());
    throw std::runtime_error(message);
}

template <typename... ExpectedResults>
inline void checkVkResult(VkResult result,
                          std::string_view expression,
                          std::source_location location,
                          ExpectedResults... expectedResults) {
    bool accepted = false;
    if constexpr (sizeof...(ExpectedResults) == 0) {
        accepted = result == VK_SUCCESS;
    } else {
        accepted = ((result == expectedResults) || ...);
    }
    if (!accepted) [[unlikely]] {
        throwVkResult(result, expression, location);
    }
}

inline void ensureVkCondition(bool condition,
                              std::string_view expression,
                              std::source_location location,
                              std::string_view message = {}) {
    if (condition) [[likely]] {
        return;
    }

    std::string error;
    if (!message.empty()) {
        error.assign(message);
        error += ": ";
    }
    error += expression;
    error += " at ";
    error += location.file_name();
    error += ':';
    error += std::to_string(location.line());
    throw std::runtime_error(error);
}

} // namespace detail

// Execute a Vulkan call that must return VK_SUCCESS.
#define VK_EXPECT(expression) VK_CHECK(expression, VK_SUCCESS)

// Validate an existing VkResult. Additional arguments define all accepted results.
#define VK_CHECK(result, ...)                                                                                       \
    ::raum::rhi::detail::checkVkResult((result), #result, std::source_location::current() __VA_OPT__(, ) __VA_ARGS__)

// Validate Vulkan-related state that is not represented by a VkResult.
#define VK_ENSURE(condition, ...)                                                                                          \
    ::raum::rhi::detail::ensureVkCondition(static_cast<bool>(condition), #condition, std::source_location::current()       \
                                           __VA_OPT__(, ) __VA_ARGS__)

struct FormatInfo {
    VkFormat format;
    uint32_t size;
    uint32_t macroPixelCount;
};

inline PFN_vkGetPipelineKeyKHR pfn_vkGetPipelineKeyKHR = nullptr;

} // namespace raum::rhi
