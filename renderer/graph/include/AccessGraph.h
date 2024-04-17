#pragma once
#include "RenderGraph.h"
#include "ResourceGraph.h"
#include "ShaderGraph.h"

namespace raum::graph {

class AccessGraph {
public:
    struct Access {
        RenderGraph::VertexType v{0};
        rhi::AccessFlags access{rhi::AccessFlags::NONE};
        rhi::PipelineStage stage{rhi::PipelineStage::TOP_OF_PIPE};
    };

    struct BufferBarrier {
        std::string_view name;
        rhi::BufferBarrierInfo info;
    };

    struct ImageBarrier {
        std::string_view name;
        rhi::ImageBarrierInfo info;
    };

    struct FrameBuffer {
        std::vector<std::string_view> images;
        rhi::FrameBufferInfo info;
    };

    using ResourceAccessMap = std::unordered_map<std::string_view, std::vector<Access>, hash_string, std::equal_to<>>;
    using BufferBarrierMap = std::unordered_map<RenderGraph::VertexType, std::vector<BufferBarrier>>;
    using ImageBarrierMap = std::unordered_map<RenderGraph::VertexType, std::vector<ImageBarrier>>;
    using RenderPassInfoMap = std::unordered_map<RenderGraph::VertexType, rhi::RenderPassInfo>;
    using FrameBufferInfoMap = std::unordered_map<RenderGraph::VertexType, FrameBuffer>;

    AccessGraph() = delete;
    AccessGraph(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& sg);

    void analyze();

    std::vector<BufferBarrier>* getBufferBarrier(RenderGraph::VertexType v);
    std::vector<ImageBarrier>* getImageBarrier(RenderGraph::VertexType v);
    rhi::RenderPassInfo* getRenderPassInfo(RenderGraph::VertexType v);
    FrameBuffer* getFrameBufferInfo(RenderGraph::VertexType v);
    ImageBarrier* presentBarrier();

    rhi::ImageLayout getImageLayout(std::string_view name, RenderGraph::VertexType v);

    void clear();

private:
    RenderGraph& _rg;
    ResourceGraph& _resg;
    ShaderGraph& _sg;
    ResourceAccessMap _accessMap;
    BufferBarrierMap _bufferBarrierMap;
    ImageBarrierMap _imageBarrierMap;
    RenderPassInfoMap _renderPassInfoMap;
    FrameBufferInfoMap _frameBufferInfoMap;
    ImageBarrier _presentBarrier{};
};

}