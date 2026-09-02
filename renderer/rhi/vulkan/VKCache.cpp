#include "VKCache.h"
#include <algorithm>
#include <array>
#include <vulkan/vulkan.h>
#include "VKDefine.h"
#include "VKDevice.h"
namespace raum::rhi {

namespace {

struct PipelineCacheHeader {
    uint32_t maxKeySize{0};
    uint32_t keySize{0};
    std::array<uint8_t, VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR> key{};

};

template <class Archive>
void serialize(Archive& archive, PipelineCacheHeader& header) {
    archive(header.maxKeySize, header.keySize, header.key);
}

} // namespace

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
    VK_EXPECT(pfn_vkGetPipelineKeyKHR(_device->device(), nullptr, &globalKey));

    const auto& resourcePath = utils::resourceDirectory();
    auto psoCache = resourcePath / "PSOCache" / "PipelineCache.bin";

    if (exists(psoCache)) {
        try {
            utils::InputArchive archive(psoCache);
            PipelineCacheHeader cachedHeader;
            archive >> cachedHeader;

            _needRegeneratePSO = cachedHeader.maxKeySize != VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR ||
                                 cachedHeader.keySize != globalKey.keySize ||
                                 cachedHeader.keySize > VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR ||
                                 memcmp(cachedHeader.key.data(),
                                        globalKey.key,
                                        std::min(cachedHeader.keySize,
                                                 uint32_t{VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR})) != 0;
        } catch (const std::exception&) {
            _needRegeneratePSO = true;
        }
    } else {
        _needRegeneratePSO = true;
    }
}


}
