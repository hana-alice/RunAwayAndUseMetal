#include <set>
#include "RHIDefine.h"
#include "RHISparseImage.h"
#include "VKCommandBuffer.h"
#include "VKImage.h"
#include "VKImageView.h"
#include "VKBuffer.h"
#include "VKDevice.h"
namespace raum::rhi {
class SparseImage : public RHISparseImage {
public:
    SparseImage(const SparseImageInfo& info, Device* device);

    void prepare(RHICommandBuffer* cmdBuffer) override;
    void update(RHICommandBuffer* cmdBuffer) override;

    void setMiptail(uint8_t* data, uint8_t mip) override;

    void analyze(RHIBuffer* buffer, RHICommandBuffer* cb) override;
    void bind() override;

    const Vec3u& granularity() override { return _granularity; }

    VkImage image() { return _sparseImage; }

    const std::vector<VkSparseImageMemoryBind>& sparseImageMemoryBinds() {
        return _vt.sparseImageMemoryBinds;
    };

    const VkSparseMemoryBind& opaqueBind() {
        return _vt.miptailBind;
    }

private:
    struct MipProps {
        uint32_t rowCount{0};
        uint32_t columnCount{0};
        uint32_t pageCount{0};
        uint32_t pageStartIndex{0};
        uint32_t width{0};
        uint32_t height{0};
    };

    struct MipBlock {
        uint8_t mipLevel{0};
        bool onScreen{false};
    };

    struct TextureBlock {
        bool operator<(TextureBlock const& other) const {
            return std::tie(new_mip_level, column, row) < std::tie(other.new_mip_level, other.column, other.row);
        };

        size_t row;
        size_t column;
        uint8_t old_mip_level;
        uint8_t new_mip_level;
        bool on_screen{false};
    };

    struct MemPageDescription {
        size_t x;
        size_t y;
        uint8_t mip_level;
    };

    struct MemAllocInfo {
        uint64_t pageSize{0};
        uint32_t pagesPerAlloc{0};
    };

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
        MemAllocator(Device* dev, const VkMemoryRequirements& req) : device(dev) {
            VkMemoryRequirements memReq = req;
            memReq.size = PageSize;
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

        static uint32_t MemoryTypeIndex;
        static uint32_t PageSize;
        static uint32_t PagesPerAlloc;
    };

    struct PageInfo {
        MemSector memory_sector;
        uint32_t offset = 0U;
    };

    struct PageTable {
        bool valid{false};
        bool resident{false};
        bool needGenMipMap{false};
        bool tail{false};
        std::set<std::tuple<uint8_t, size_t, size_t>> render_required_set;
        PageInfo pageInfo;
    };

    struct VirtualTexture {
        uint32_t pageSize{0};

        std::vector<MipProps> mips;
        std::vector<PageTable> pageTable;
        std::vector<VkSparseImageMemoryBind> sparseImageMemoryBinds;
        VkSparseMemoryBind miptailBind;
        std::vector<std::vector<MipBlock>> currMipTable;
        std::vector<std::vector<MipBlock>> newMipTable;
        std::set<TextureBlock> texture_block_update_set;
        std::set<size_t> update_set;
        std::vector<PageInfo> mipTails;
    };

    MemAllocInfo memAllocInfo;
    std::list<std::shared_ptr<MemAllocator>> sectors;

    PageInfo allocate(uint32_t pageIndex);

    void resetMipTable();
    uint8_t getMipLevel(uint32_t pageIndex);
    void fillTail();
    void compareMipTable();
    void processTextureBlock(const TextureBlock& block);
    void loadLeastDetail();
    void processTextureBlock();
    void updateAndGenerate(RHICommandBuffer* cmdBuffer);
    void bindSparseImage();

    void getRelatedBlocks(uint32_t row, uint32_t col, uint8_t mip, std::set<size_t>& indices);

    std::vector<size_t> getDependentPages(size_t col, size_t row, uint8_t mipLevel);
    MemPageDescription getMemPageDescription(size_t pageIndex);
    uint32_t getPageIndex(uint32_t row, uint32_t col, uint8_t mip);

    bool update_required{false};

    uint32_t _blockPerCycle{25};

    uint8_t _minLod{0};
    uint8_t _maxLod{0};
    uint32_t _width{0};
    uint32_t _height{0};
    rhi::Format _format{rhi::Format::UNKNOWN};
    Vec3u _granularity;

    VkSemaphore bound_semaphore;
    VkSemaphore submit_semaphore;

    uint8_t frame_counter_per_transfer{0};
    VirtualTexture _vt;
    VkImage _sparseImage;
    uint8_t* _data;
    Device* _device;
    VmaAllocation _miptailAlloc;
    VmaPool _pool;

    VkMemoryRequirements _memReq;

    VkSparseImageMemoryRequirements _req{};
    std::vector<std::pair<uint8_t, uint8_t*>> _miptails;
    Buffer* _accessBuffer{nullptr};
};

} // namespace raum::rhi