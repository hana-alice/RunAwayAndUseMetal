#pragma once
#include "RHIDefine.h"
#include "RHIImage.h"
namespace raum::rhi {

class RHICommandBuffer;

class RHISparseImage : public RHIImage {
public:
    RHISparseImage(const SparseImageInfo& info, RHIDevice* device)
    : RHIImage(ImageInfo{
                   .imageFlag = ImageFlag::SPARSE_RESIDENCY,
                   .extent = {info.width, info.height, 1},
               },
               device){};
    virtual void prepare(RHICommandBuffer* cmdBuffer,
                         uint32_t numCols,
                         uint32_t numRows,
                         uint32_t pageCount,
                         uint32_t pageSize) = 0;
    virtual void update(RHICommandBuffer* cmdBuffer) = 0;
    virtual void reset(uint32_t pageIndex) = 0;
    virtual void setMiptail(uint8_t* data, uint8_t mip) = 0;
    virtual void analyze(RHIBuffer* buffer, RHICommandBuffer* cb) = 0;
    virtual void bind(SparseType type) = 0;
    virtual void shrink() = 0;
    virtual const Vec3u& granularity() = 0;
    virtual uint8_t firstMipTail() = 0;
    virtual void allocatePage(uint32_t pageIndex) = 0;
    virtual void setPageMemoryBindInfo(uint32_t pageIndex, const Vec3u& offset, const Vec3u& extent, uint8_t mip, uint32_t slice) = 0;

    // might deprecated future
    virtual void initPageInfo(uint32_t pageCount, uint32_t pageSize) = 0;
};

} // namespace raum::rhi