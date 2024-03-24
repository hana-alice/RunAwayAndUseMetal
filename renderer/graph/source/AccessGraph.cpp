#include "AccessGraph.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBuffer.h"

namespace raum::graph {

namespace {

bool isDepthStencil(const RenderingResource& res) {
    return res.type == ResourceType::DEPTH || res.type == ResourceType::STENCIL || res.type == ResourceType::DEPTH_STENCIL;
}

bool hasDepth(const RenderingResource& res) {
    return res.type == ResourceType::DEPTH || res.type == ResourceType::DEPTH_STENCIL;
}

bool isReadAccess(const rhi::AccessFlags access) {
    return access <= rhi::AccessFlags::SHADING_RATE_ATTACHMENT_READ;
}

rhi::PipelineStage getPipelineState(rhi::ShaderStage visibility) {
    auto stage{rhi::PipelineStage::TOP_OF_PIPE};
    if (test(visibility, rhi::ShaderStage::VERTEX)) {
        stage |= rhi::PipelineStage::VERTEX_SHADER;
    }
    if (test(visibility, rhi::ShaderStage::FRAGMENT)) {
        stage |= rhi::PipelineStage::FRAGMENT_SHADER;
    }
    if (test(visibility, rhi::ShaderStage::COMPUTE)) {
        stage |= rhi::PipelineStage::COMPUTE_SHADER;
    }
    if (test(visibility, rhi::ShaderStage::TASK)) {
        stage |= rhi::PipelineStage::TASK_SHADER;
    }
    if (test(visibility, rhi::ShaderStage::MESH)) {
        stage |= rhi::PipelineStage::MESH_SHADER;
    }
    return stage;
}

rhi::ImageLayout getImageLayout(rhi::AccessFlags flags) {
    rhi::ImageLayout imgLayout{rhi::ImageLayout::UNDEFINED};
    if(test(flags, rhi::AccessFlags::INPUT_ATTACHMENT_READ)) {
        imgLayout = rhi::ImageLayout::GENERAL;
    } else {
        if(test(flags, rhi::AccessFlags::COLOR_ATTACHMENT_WRITE)) {
            imgLayout = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        } else if(test(flags, rhi::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE)) {
            imgLayout = rhi::ImageLayout::DEPTH_ATTACHMENT_OPTIMAL;
        } else if(test(flags, rhi::AccessFlags::SHADING_RATE_ATTACHMENT_READ)) {
            imgLayout = rhi::ImageLayout::SHADING_RATE;
        }
    }

    if(test(flags, rhi::AccessFlags::TRANSFER_READ)) {
        imgLayout = rhi::ImageLayout::TRANSFER_SRC_OPTIMAL;
    } else if(test(flags, rhi::AccessFlags::TRANSFER_WRITE)) {
        imgLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL;
    }

    if(test(flags, rhi::AccessFlags::SHADER_READ)) {
        imgLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    }
    if(test(flags, rhi::AccessFlags::SHADER_WRITE)) {
        imgLayout = rhi::ImageLayout::GENERAL;
    }

    return imgLayout;
}

struct AccessVisitor : public boost::dfs_visitor<> {
public:
    void discover_vertex(const RenderGraphImpl::vertex_descriptor v, const RenderGraphImpl& rg) {
        if (std::holds_alternative<RenderPassData>(rg[v].data)) {
            const auto& data = std::get<RenderPassData>(rg[v].data);
            for (const auto& res : data.attachments) {
                // convention: "" represents color/depth outputs
                rhi::AccessFlags access{rhi::AccessFlags::NONE};
                rhi::PipelineStage stage{rhi::PipelineStage::TOP_OF_PIPE};
                if (res.bindingName.empty()) {
                    raum_expect(res.access == Access::WRITE, "");
                    if (isDepthStencil(res)) {
                        access |= rhi::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE;
                    } else [[likely]] {
                        access |= rhi::AccessFlags::COLOR_ATTACHMENT_WRITE;
                    }
                    stage = rhi::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
                } else {
                    stage = rhi::PipelineStage::FRAGMENT_SHADER;
                    raum_expect(res.access != Access::WRITE, "");
                    if (res.type == ResourceType::SHADING_RATE) {
                        access |= rhi::AccessFlags::SHADING_RATE_ATTACHMENT_READ;
                    } else {
                        access |= rhi::AccessFlags::INPUT_ATTACHMENT_READ;
                    }

                    if (hasDepth(res)) {
                        stage |= rhi::PipelineStage::EARLY_FRAGMENT_TESTS;
                    }

                    if (res.access != Access::READ) {
                        if (hasDepth(res)) {
                            access |= rhi::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE;
                        } else {
                            access |= rhi::AccessFlags::COLOR_ATTACHMENT_WRITE;
                        }
                        stage |= rhi::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
                    }
                }
                _accessMap[res.name].emplace_back(v, access, stage);
            }
        } else if (std::holds_alternative<RenderQueueData>(rg[v].data)) {
            const auto& data = std::get<RenderQueueData>(rg[v].data);
            for (const auto& res : data.resources) {
                rhi::AccessFlags access{rhi::AccessFlags::NONE};
                rhi::PipelineStage stage{rhi::PipelineStage::TOP_OF_PIPE};
                if (res.access == Access::READ) {
                    const auto& resourceDetail = _resg.get(res.name);
                    if (std::holds_alternative<BufferData>(resourceDetail.data)) {
                        const auto& bufferData = std::get<BufferData>(resourceDetail.data);
                        if (rhi::test(bufferData.info.bufferUsage, rhi::BufferUsage::UNIFORM)) {
                            access |= rhi::AccessFlags::UNIFORM_READ;
                        } else {
                            access |= rhi::AccessFlags::SHADER_READ;
                        }
                    } else if (std::holds_alternative<BufferViewData>(resourceDetail.data)) {
                        const auto& bufferViewData = std::get<BufferViewData>(resourceDetail.data);
                        const auto& bufferInfo = bufferViewData.info.buffer->info();
                        if (rhi::test(bufferInfo.bufferUsage, rhi::BufferUsage::UNIFORM)) {
                            access |= rhi::AccessFlags::UNIFORM_READ;
                        } else {
                            access |= rhi::AccessFlags::SHADER_READ;
                        }
                    } else {
                        access |= rhi::AccessFlags::SHADER_READ;
                    }
                } else if (res.access == Access::READ_WRITE) {
                    access |= rhi::AccessFlags::SHADER_READ | rhi::AccessFlags::SHADER_WRITE;
                } else {
                    access |= rhi::AccessFlags::SHADER_WRITE;
                }
                const auto& layout = _sg.layout(data.phase);
                auto vis = layout.bindings.at(res.name).visibility;
                stage = getPipelineState(vis);
                _accessMap[res.name].emplace_back(v, access, stage);
            }
        }
    }
    const ResourceGraph& _resg;
    const ShaderGraph& _sg;
    AccessGraph::ResourceAccessMap& _accessMap;
};

void populateBarrier(const AccessGraph::ResourceAccessMap& accessMap,
                     ResourceGraph& resg,
                     AccessGraph::BufferBarrierMap& bufferBarrierMap,
                     AccessGraph::ImageBarrierMap& imageBarrierMap) {
    for (const auto& [name, status] : accessMap) {
        auto& resDetail = resg.get(name);
        auto lastAccess = resDetail.access;
        auto lastStage = rhi::PipelineStage::TOP_OF_PIPE;
        rhi::RHIImage* backbuffer{nullptr};
        for (const auto& [v, access, stage] : status) {
            if (std::holds_alternative<BufferData>(resDetail.data) || std::holds_alternative<BufferViewData>(resDetail.data)) {
                if (isReadAccess(lastAccess) && isReadAccess(access)) {
                    continue;
                }

                rhi::RHIBuffer* rhiRes{nullptr};
                std::visit(overloaded{
                               [&rhiRes](const BufferData& bufferData) {
                                   rhiRes = bufferData.buffer;
                               },
                               [&rhiRes](const BufferViewData& bufferViewData) {
                                   rhiRes = bufferViewData.info.buffer;
                               },
                               [&rhiRes](auto _) {
                                   raum_unreachable();
                               },
                           },
                           resDetail.data);

                bufferBarrierMap.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(v),
                    std::forward_as_tuple(
                        rhiRes,
                        lastStage,
                        stage,
                        lastAccess,
                        access));

                lastAccess = access;
                if(resDetail.residency != ResourceResidency::DONT_CARE) {
                    resDetail.access = access;
                }
            } else if (std::holds_alternative<ImageData>(resDetail.data) || std::holds_alternative<ImageViewData>(resDetail.data)) {
                if (lastAccess == access) {
                    continue;
                }

                rhi::RHIImage* rhiRes{nullptr};
                std::visit(overloaded{
                               [&rhiRes](const ImageData& imageData) {
                                   rhiRes = imageData.image;
                               },
                               [&rhiRes](const ImageViewData& imageViewData) {
                                   rhiRes = imageViewData.info.image;
                               },
                               [&rhiRes](auto _) {
                                   raum_unreachable();
                               },
                           },
                           resDetail.data);

                imageBarrierMap.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(v),
                    std::forward_as_tuple(
                        rhiRes,
                        lastStage,
                        stage,
                        getImageLayout(lastAccess),
                        getImageLayout(access),
                        lastAccess,
                        access));

                lastAccess = access;
                if(resDetail.residency != ResourceResidency::DONT_CARE) {
                    resDetail.access = access;
                }
            }
        }
        if(resDetail.residency == ResourceResidency::SWAPCHAIN) {
            if(backbuffer) {
                imageBarrierMap.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(0xFFFFFFFF),
                    std::forward_as_tuple(
                        backbuffer,
                        lastStage,
                        rhi::PipelineStage::BOTTOM_OF_PIPE,
                        getImageLayout(lastAccess),
                        rhi::ImageLayout::PRESENT,
                        lastAccess,
                        rhi::AccessFlags::NONE));
                resDetail.access == rhi::AccessFlags::NONE;
            }
        }
    }
}

} // namespace

AccessGraph::AccessGraph(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& sg) : _rg(rg), _resg(resg), _sg(sg) {}

void AccessGraph::analyze() {
    auto indexMap = boost::get(boost::vertex_index, _rg.impl());
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    AccessVisitor visitor{{}, _resg, _sg, _accessMap};
    for (auto vertex : boost::make_iterator_range(boost::vertices(_rg.impl()))) {
        if (std::holds_alternative<RenderPassData>(_rg.impl()[vertex].data)) {
            boost::depth_first_visit(_rg.impl(), 0, visitor, colorMap);
        }
    }

    populateBarrier(_accessMap, _resg, _bufferBarrierMap, _imageBarrierMap);
}

} // namespace raum::graph