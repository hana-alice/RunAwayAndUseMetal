#include "VKCache.h"
#include <algorithm>
#include <vulkan/vulkan.h>
#include "VKDefine.h"
#include "VKDevice.h"
namespace raum::rhi {

ProgramCache::ProgramCache(Device* device):_device(device) {
}

void ProgramCache::validate() {
    if (!pfn_vkGetPipelineKeyKHR) {
        _needRegeneratePSO = true;
        return;
    }

    VkPipelineBinaryKeyKHR globalKey{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR
    };
    pfn_vkGetPipelineKeyKHR(_device->device(), nullptr, &globalKey);

    const auto& resourcePath = utils::resourceDirectory();
    auto psoCache = resourcePath / "PSOCache" / "PipelineCache.bin";

    if (exists(psoCache)) {
        utils::InputArchive archive(psoCache);

        uint32_t maxGlobalKeySize{0};
        uint32_t globalKeySize{0};
        uint8_t key[VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR];

        archive >> maxGlobalKeySize;
        archive >> globalKeySize;
        archive >> key;

        _needRegeneratePSO = maxGlobalKeySize != VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR ||
                             globalKeySize != globalKey.keySize ||
                             globalKeySize > VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR ||
                             memcmp(&key[0], &globalKey.key[0],
                                    std::min(globalKeySize, uint32_t{VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR})) != 0;
    } else {
        _needRegeneratePSO = true;
    }
}


}
