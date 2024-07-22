#include <set>
#include "RHIDefine.h"
#include "RHISparseImage.h"
#include "VKBuffer.h"
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKImage.h"
#include "VKImageView.h"
namespace raum::rhi {
class SparseImage : public RHISparseImage {
public:
    SparseImage(const SparseImageInfo& info, Device* device);
    ~SparseImage();

    void prepare(RHICommandBuffer* cmdBuffer,
                 uint32_t numCols,
                 uint32_t numRows,
                 uint32_t pageCount,
                 uint32_t pageSize) override;
    void update(RHICommandBuffer* cmdBuffer) override;
    void setMiptail(uint8_t* data, uint8_t mip) override;
    void analyze(RHIBuffer* buffer, RHICommandBuffer* cb) override;
    void bind(SparseType type) override;
    void shrink() override;
    const Vec3u& granularity() override { return _granularity; }
    uint8_t firstMipTail() override;
    void allocatePage(uint32_t pageIndex) override;
    void reset(uint32_t pageIndex) override;
    void setPageMemoryBindInfo(uint32_t pageIndex, const Vec3u& offset, const Vec3u& extent, uint8_t mip, uint32_t slice) override;

    void initPageInfo(uint32_t pageCount, uint32_t pageSize) override;

    VkImage image() { return _sparseImage; }

    const std::vector<VkSparseImageMemoryBind>& sparseImageMemoryBinds();

    const VkSparseMemoryBind& opaqueBind() {
        return _miptailBind;
    }

private:
    struct MemAllocator;
    struct MemSector {
        uint32_t index{0};
        uint32_t offset{0};
        VkDeviceMemory memory = VK_NULL_HANDLE;
        MemAllocator* allocator{nullptr};

        void reset() {
            if (allocator)
                allocator->free(index);
        }
    };

    struct MemAllocator {
        MemAllocator(Device* dev, const VkMemoryRequirements& req, uint32_t pageSize) : device(dev) {
            VkMemoryRequirements memReq = req;
            memReq.size = pageSize;
            VmaAllocationCreateInfo aci{};
            aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            aci.memoryTypeBits = req.memoryTypeBits;
            allocations.resize(PagesPerAlloc);
            allocInfos.resize(PagesPerAlloc);
            auto res = vmaAllocateMemoryPages(device->allocator(), &memReq, &aci, PagesPerAlloc, allocations.data(), allocInfos.data());
            assert(res == VK_SUCCESS);

            for (size_t i = 0U; i < PagesPerAlloc; i++) {
                availableIndex.insert(i);
            }
        }
        ~MemAllocator() {
            vmaFreeMemoryPages(device->allocator(), PagesPerAlloc, allocations.data());
        }

        MemSector allocate() {
            assert(!availableIndex.empty());
            auto it = availableIndex.begin();
            auto idx = *it;
            availableIndex.erase(it);
            return {idx, static_cast<uint32_t>(allocInfos[idx].offset), allocInfos[idx].deviceMemory, this};
        }

        void free(uint32_t index) {
            availableIndex.emplace(index);
        }

        bool full() {
            return availableIndex.empty();
        }

        Device* device;
        std::set<uint32_t> availableIndex;

        std::vector<VmaAllocationInfo> allocInfos;
        std::vector<VmaAllocation> allocations;
        uint32_t pagesPerAlloc{50};
    };

    struct PageInfo {
        MemSector memory_sector;
        uint32_t offset = 0U;
    };

    std::vector<VkSparseImageMemoryBind> _imageMemoryBinds;
    VkSparseMemoryBind _miptailBind;
    std::vector<PageInfo> _pages;
    uint32_t _pageSize{0};

    static constexpr uint32_t PagesPerAlloc{50};

    std::list<std::shared_ptr<MemAllocator>> sectors;

    PageInfo allocate(uint32_t pageIndex);

    void prepareMiptail(RHICommandBuffer* cmdBuffer);

    uint32_t _blockPerCycle{25};
    uint32_t _width{0};
    uint32_t _height{0};
    rhi::Format _format{rhi::Format::UNKNOWN};
    Vec3u _granularity;

    uint8_t frame_counter_per_transfer{0};
    VkImage _sparseImage;
    Device* _device;
    VmaAllocation _miptailAlloc;

    VkMemoryRequirements _memReq;

    VkSparseImageMemoryRequirements _req{};
    std::vector<std::pair<uint8_t, uint8_t*>> _miptails;

    uint32_t _frameStepCount{0};

    uint32_t _memTypeIndex{0};
};

} // namespace raum::rhi