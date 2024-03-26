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
    using ResourceAccessMap = std::unordered_map<std::string_view, std::vector<Access>, hash_string, std::equal_to<>>;
    using BufferBarrierMap = std::unordered_map<RenderGraph::VertexType, std::vector<rhi::BufferBarrierInfo>>;
    using ImageBarrierMap = std::unordered_map<RenderGraph::VertexType, std::vector<rhi::ImageBarrierInfo>>;
    using RenderPassInfoMap = std::unordered_map<RenderGraph::VertexType, rhi::RenderPassInfo>;
    using FrameBufferInfoMap = std::unordered_map<RenderGraph::VertexType, rhi::FrameBufferInfo>;

    AccessGraph() = delete;
    AccessGraph(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& sg);

    void analyze();

    rhi::BufferBarrierInfo* getBufferBarrier(RenderGraph::VertexType v);

private:
    RenderGraph& _rg;
    ResourceGraph& _resg;
    ShaderGraph& _sg;
    ResourceAccessMap _accessMap;
    BufferBarrierMap _bufferBarrierMap;
    ImageBarrierMap _imageBarrierMap;
    RenderPassInfoMap _renderPassInfoMap;
    FrameBufferInfoMap _frameBufferInfoMap;
};

}