//
// Created by zeqia on 2024/7/15.
//
#pragma once
#include <set>
#include "RHIDefine.h"
#include "core/utils/CyclicCounter.h"
#include "BindGroup.h"
namespace raum::render {

struct MipProps {
    uint32_t rowCount{0};
    uint32_t columnCount{0};
    uint32_t pageCount{0};
    uint32_t pageStartIndex{0};
    uint32_t width{0};
    uint32_t height{0};
};

struct PageMemBindInfo {
    uint32_t x{0};
    uint32_t y{0};
    uint32_t z{0};
    uint32_t w{0};
    uint32_t h{0};
    uint32_t d{0};
};

struct PageTable {
    bool valid{false};
    bool resident{false};
    uint8_t mip{255};
    PageMemBindInfo bindInfo;
};

class VirtualTexture {
public:
    explicit VirtualTexture(uint8_t* data,
                            uint32_t width,
                            uint32_t height,
                            rhi::DevicePtr device);
    ~VirtualTexture();

    void prepare(rhi::CommandBufferPtr cb);
    void update(rhi::CommandBufferPtr cb);
    void setMiptail(uint8_t* data, uint8_t mip);

    void resetAccessCounter(rhi::CommandBufferPtr cmd);
    void invalidateFeedback(rhi::CommandBufferPtr cmd);

    bool hasRemainedTask() const { return _remainTask || !_updates.empty(); }

    rhi::SparseImagePtr sparseImage() const { return _sparseImage; }
    rhi::ImageViewPtr sparseView() const { return _sparseView; }
    rhi::BufferPtr accessCounterBuffer() const { return _accessCounter; };
    rhi::BufferPtr metaInfoBuffer() const { return _metaInfoBuffer; }
private:
    void analyze(rhi::CommandBufferPtr cb);
    uint8_t getMipLevel(uint32_t pageIndex);
    void getRelatedBlocks(uint32_t row, uint32_t col, uint8_t mip, std::set<size_t>& indices);
    uint32_t getPageIndex(uint32_t row, uint32_t col, uint8_t mip);

    uint32_t _width{0};
    uint32_t _height{0};
    rhi::Format _format{rhi::Format::UNKNOWN};
    uint8_t* _data{nullptr};
    uint32_t _numCols{0};
    uint32_t _numRows{0};
    uint32_t _pageSize{0};
    uint8_t _firstMiptail{0};
    Vec3u _granularity;
    std::vector<PageTable> _pageTable;
    std::vector<MipProps> _mipProps;
    std::set<size_t> _updates;

    bool _remainTask{false};
    CyclicCounter<uint8_t> _frameCounter{rhi::FRAMES_IN_FLIGHT};
    rhi::BufferPtr _accessCounter;
    rhi::BufferPtr _feedbackBuffer;
    rhi::BufferPtr _metaInfoBuffer;
    rhi::ImageViewPtr _sparseView;
    rhi::SparseImagePtr _sparseImage;
    rhi::DevicePtr _device;
};

using VirtualTexturePtr = std::shared_ptr<VirtualTexture>;

} // namespace raum::render
