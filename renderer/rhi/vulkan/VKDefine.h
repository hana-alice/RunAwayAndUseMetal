#pragma once
#include <algorithm>
#include <functional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
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

template <typename Value, typename Function, typename... PrefixArgs>
std::vector<Value> enumerateVk(std::string_view expression,
                               std::source_location location,
                               Function function,
                               PrefixArgs... prefixArgs) {
    for (;;) {
        uint32_t count = 0;
        const auto countResult = std::invoke(function, prefixArgs..., &count, static_cast<Value*>(nullptr));
        checkVkResult(countResult, expression, location, VK_SUCCESS);

        if (!count) {
            return {};
        }

        std::vector<Value> values(count);
        const auto valuesResult = std::invoke(function, prefixArgs..., &count, values.data());
        if (valuesResult == VK_INCOMPLETE) {
            continue;
        }
        checkVkResult(valuesResult, expression, location, VK_SUCCESS);
        values.resize(count);
        return values;
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

inline std::string_view vkPropertyName(const VkLayerProperties& property) {
    return property.layerName;
}

inline std::string_view vkPropertyName(const VkExtensionProperties& property) {
    return property.extensionName;
}

template <typename Properties>
bool hasVkProperty(const Properties& available, std::string_view required) {
    return std::ranges::any_of(available, [required](const auto& property) {
        return vkPropertyName(property) == required;
    });
}

template <typename RequiredNames, typename Properties>
void requireVkProperties(const RequiredNames& required,
                         const Properties& available,
                         std::string_view propertyType,
                         std::source_location location = std::source_location::current()) {
    std::string missing;
    for (const auto* name : required) {
        if (hasVkProperty(available, name)) {
            continue;
        }
        if (!missing.empty()) {
            missing += ", ";
        }
        missing += name;
    }

    if (!missing.empty()) {
        std::string message{"Missing required Vulkan "};
        message += propertyType;
        message += ": ";
        message += missing;
        detail::ensureVkCondition(false, propertyType, location, message);
    }
}

// Execute a Vulkan call that must return VK_SUCCESS.
#define VK_EXPECT(expression) VK_CHECK(expression, VK_SUCCESS)

// Validate an existing VkResult. Additional arguments define all accepted results.
#define VK_CHECK(result, ...)                                                                                       \
    ::raum::rhi::detail::checkVkResult((result), #result, std::source_location::current() __VA_OPT__(, ) __VA_ARGS__)

// Validate Vulkan-related state that is not represented by a VkResult.
#define VK_ENSURE(condition, ...)                                                                                          \
    ::raum::rhi::detail::ensureVkCondition(static_cast<bool>(condition), #condition, std::source_location::current()       \
                                           __VA_OPT__(, ) __VA_ARGS__)

// Enumerate Vulkan values through the standard (count, values) API pattern.
#define VK_ENUMERATE(value_type, function, ...)                                                                    \
    ::raum::rhi::detail::enumerateVk<value_type>(#function, std::source_location::current(), (function)            \
                                                 __VA_OPT__(, ) __VA_ARGS__)

struct FormatInfo {
    VkFormat format;
    uint32_t size;
    uint32_t macroPixelCount;
};

inline PFN_vkGetPipelineKeyKHR pfn_vkGetPipelineKeyKHR = nullptr;

} // namespace raum::rhi
