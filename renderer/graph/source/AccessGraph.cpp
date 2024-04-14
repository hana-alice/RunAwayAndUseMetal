#include "AccessGraph.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBuffer.h"
#include "RHIImage.h"
#include "RHIImageView.h"

namespace raum::graph {

namespace {

bool isDepthStencil(const AttachmentResource& res) {
    return res.type == ResourceType::DEPTH || res.type == ResourceType::STENCIL || res.type == ResourceType::DEPTH_STENCIL;
}

bool hasDepth(const AttachmentResource& res) {
    return res.type == ResourceType::DEPTH || res.type == ResourceType::DEPTH_STENCIL;
}

bool isReadAccess(const rhi::AccessFlags access) {
    return access <= rhi::AccessFlags::SHADING_RATE_ATTACHMENT_READ;
}

rhi::PipelineStage getPipelineStage(rhi::ShaderStage visibility) {
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
    if (test(flags, rhi::AccessFlags::INPUT_ATTACHMENT_READ)) {
        imgLayout = rhi::ImageLayout::GENERAL;
    } else {
        if (test(flags, rhi::AccessFlags::COLOR_ATTACHMENT_WRITE)) {
            imgLayout = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        } else if (test(flags, rhi::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE)) {
            imgLayout = rhi::ImageLayout::DEPTH_ATTACHMENT_OPTIMAL;
        } else if (test(flags, rhi::AccessFlags::SHADING_RATE_ATTACHMENT_READ)) {
            imgLayout = rhi::ImageLayout::SHADING_RATE;
        }
    }

    if (test(flags, rhi::AccessFlags::TRANSFER_READ)) {
        imgLayout = rhi::ImageLayout::TRANSFER_SRC_OPTIMAL;
    } else if (test(flags, rhi::AccessFlags::TRANSFER_WRITE)) {
        imgLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL;
    }

    if (test(flags, rhi::AccessFlags::SHADER_READ)) {
        imgLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    }
    if (test(flags, rhi::AccessFlags::SHADER_WRITE)) {
        imgLayout = rhi::ImageLayout::GENERAL;
    }

    return imgLayout;
}

rhi::Format getFormat(const Resource& res) {
    rhi::Format format{rhi::Format::UNKNOWN};
    std::visit(
        overloaded{
            [&format](const ImageData& img) {
                format = img.info.format;
            },
            [&format](const ImageViewData& imgView) {
                format = imgView.info.format;
            },
            [](auto) {
            },
        },
        res.data);
    return format;
}

uint32_t getSampleCount(const Resource& res) {
    uint32_t samples{1};
    std::visit(
        overloaded{
            [&samples](const ImageData& img) {
                samples = img.info.sampleCount;
            },
            [&samples](const ImageViewData& imgView) {
                samples = imgView.imageView->image()->info().sampleCount;
            },
            [](auto) {
            },
        },
        res.data);
    return samples;
}

auto decomposeDetail(std::string_view name, ResourceGraph& resg) {
    uint32_t samples{1};
    rhi::Format format{rhi::Format::UNKNOWN};
    uint32_t width{0};
    uint32_t height{0};

    const auto& res = resg.get(name);
    std::visit(
        overloaded{
            [&](const ImageData& img) {
                samples = img.info.sampleCount;
                format = img.info.format;
                width = img.info.extent.x;
                height = img.info.extent.y;
            },
            [&](const ImageViewData& imgView) {
                const auto& originRes = resg.get(imgView.origin);
                const auto& imgInfo = std::get<ImageData>(originRes.data).info;
                samples = imgInfo.sampleCount;
                format = imgView.info.format;
                width = imgInfo.extent.x;
                height = imgInfo.extent.y;
            },
            [](auto) {
            },
        },
        res.data);
    return std::tie(samples, format, width, height);
}

struct AccessVisitor : public boost::dfs_visitor<> {
public:
    void discover_vertex(const RenderGraphImpl::vertex_descriptor v, const RenderGraphImpl& rg) {
        if (std::holds_alternative<RenderPassData>(rg[v].data)) {
            auto [fbIter, _] = _fbInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(v), std::forward_as_tuple());
            const auto& data = std::get<RenderPassData>(rg[v].data);
            //            fbIter->second.renderPassName = rg[v].name;

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
                auto [sampleCount, format, width, height] = decomposeDetail(res.name, _resg);

                auto [iter, _] = _rpInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(v), std::forward_as_tuple());
                iter->second.attachments.emplace_back(
                    res.loadOp,
                    res.storeOp,
                    res.stencilLoadOp,
                    res.stencilStoreOp,
                    format,
                    sampleCount,
                    getImageLayout(access),
                    getImageLayout(access));

                const auto& resView = _resg.getView(res.name);
                const auto& imageView = std::get<ImageViewData>(resView.data);
                fbIter->second.info.width = width;
                fbIter->second.info.height = height;
                fbIter->second.info.layers = imageView.info.range.sliceCount;
                fbIter->second.images.emplace_back(res.name);

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
                stage = getPipelineStage(vis);
                _accessMap[res.name].emplace_back(v, access, stage);
            }
        }
    }

    void finish_vertex(const RenderGraphImpl::vertex_descriptor v, const RenderGraphImpl& rg) {
        std::visit(
            overloaded{
                [&](RenderPassData& renderpass) {
                    if (_rpInfoMap[v].subpasses.empty()) {
                        auto& subpassInfo = _rpInfoMap[v].subpasses.emplace_back();
                        for (const auto& attachment : renderpass.attachments) {
                            auto index = static_cast<uint8_t>(&attachment - &renderpass.attachments[0]);
                            if (isDepthStencil(attachment)) {
                                subpassInfo.depthStencil.emplace_back(index,
                                                                      _rpInfoMap[v].attachments[v].finalLayout);
                            } else {
                                subpassInfo.colors.emplace_back(index,
                                                                _rpInfoMap[v].attachments[v].finalLayout);
                            }
                        }
                    }
                },
                [](auto _) {
                }},
            rg[v].data);
    }
    ResourceGraph& _resg;
    const ShaderGraph& _sg;
    AccessGraph::ResourceAccessMap& _accessMap;
    AccessGraph::RenderPassInfoMap& _rpInfoMap;
    AccessGraph::FrameBufferInfoMap& _fbInfoMap;
};

void populateBarrier(const AccessGraph::ResourceAccessMap& accessMap,
                     ResourceGraph& resg,
                     AccessGraph::BufferBarrierMap& bufferBarrierMap,
                     AccessGraph::ImageBarrierMap& imageBarrierMap,
                     AccessGraph::ImageBarrier& presentBarrier) {
    for (const auto& [name, status] : accessMap) {
        auto& resDetail = resg.get(name);
        auto lastAccess = resDetail.access;
        auto lastStage = rhi::PipelineStage::TOP_OF_PIPE;
        std::string_view backbuffer{};
        for (const auto& [v, access, stage] : status) {
            if (std::holds_alternative<BufferData>(resDetail.data) || std::holds_alternative<BufferViewData>(resDetail.data)) {
                if (isReadAccess(lastAccess) && isReadAccess(access)) {
                    continue;
                }

                bufferBarrierMap[v].emplace_back(
                    name,
                    rhi::BufferBarrierInfo(nullptr,
                                           lastStage,
                                           stage,
                                           lastAccess,
                                           access));

                lastAccess = access;
                if (resDetail.residency != ResourceResidency::DONT_CARE) {
                    resDetail.access = access;
                }
            } else if (std::holds_alternative<ImageData>(resDetail.data) || std::holds_alternative<ImageViewData>(resDetail.data)) {
                if (lastAccess == access) {
                    continue;
                }

                imageBarrierMap[v].emplace_back(
                    name,
                    rhi::ImageBarrierInfo{
                        nullptr,
                        lastStage,
                        stage,
                        getImageLayout(lastAccess),
                        getImageLayout(access),
                        lastAccess,
                        access});

                lastAccess = access;
                if (resDetail.residency != ResourceResidency::DONT_CARE) {
                    resDetail.access = access;
                    if (resDetail.residency == ResourceResidency::SWAPCHAIN) {
                        backbuffer = name;
                    }
                }
            }
        }
        if (resDetail.residency == ResourceResidency::SWAPCHAIN) {
            if (!backbuffer.empty()) {
                presentBarrier = {
                    backbuffer,
                    rhi::ImageBarrierInfo{
                        nullptr,
                        lastStage,
                        rhi::PipelineStage::BOTTOM_OF_PIPE,
                        getImageLayout(lastAccess),
                        rhi::ImageLayout::PRESENT,
                        lastAccess,
                        rhi::AccessFlags::NONE}};
                resDetail.access = rhi::AccessFlags::NONE;
            }
        }
    }
}
} // namespace


AccessGraph::AccessGraph(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& sg) : _rg(rg), _resg(resg), _sg(sg) {}

void AccessGraph::analyze() {
    auto indexMap = boost::get(boost::vertex_index, _rg.impl());
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    AccessVisitor visitor{{}, _resg, _sg, _accessMap, _renderPassInfoMap, _frameBufferInfoMap};
    for (auto vertex : boost::make_iterator_range(boost::vertices(_rg.impl()))) {
        if (std::holds_alternative<RenderPassData>(_rg.impl()[vertex].data)) {
            boost::depth_first_visit(_rg.impl(), 0, visitor, colorMap);
        }
    }

    populateBarrier(_accessMap, _resg, _bufferBarrierMap, _imageBarrierMap, _presentBarrier);
}

std::vector<AccessGraph::BufferBarrier>* AccessGraph::getBufferBarrier(RenderGraph::VertexType v) {
    if (_bufferBarrierMap.contains(v)) {
        return &_bufferBarrierMap[v];
    }
    return nullptr;
}

std::vector<AccessGraph::ImageBarrier>* AccessGraph::getImageBarrier(RenderGraph::VertexType v) {
    if (_imageBarrierMap.contains(v)) {
        return &_imageBarrierMap[v];
    }
    return nullptr;
}

rhi::RenderPassInfo* AccessGraph::getRenderPassInfo(RenderGraph::VertexType v) {
    if (_renderPassInfoMap.contains(v)) {
        return &_renderPassInfoMap[v];
    }
    return nullptr;
}

AccessGraph::FrameBuffer* AccessGraph::getFrameBufferInfo(RenderGraph::VertexType v) {
    if (_frameBufferInfoMap.contains(v)) {
        return &_frameBufferInfoMap[v];
    }
    return nullptr;
}

AccessGraph::ImageBarrier* AccessGraph::presentBarrier() {
    if(!_presentBarrier.name.empty()) {
        return &_presentBarrier;
    }
    return nullptr;
}

rhi::ImageLayout AccessGraph::getImageLayout(std::string_view name, unsigned long long v) {
    auto res = rhi::ImageLayout::UNDEFINED;
    if(_accessMap.contains(name)) {
        const auto& accesses = _accessMap.at(name);
        auto iter = std::find_if(accesses.begin(), accesses.end(),
                     [v](const Access& access){
                         return access.v == v;
                     });
        if(iter != accesses.end()) {
            res = graph::getImageLayout(iter->access);
        }
    }
    return res;
}

void AccessGraph::clear() {
    _accessMap.clear();
    _bufferBarrierMap.clear();
    _imageBarrierMap.clear();
    _renderPassInfoMap.clear();
    _frameBufferInfoMap.clear();
    _presentBarrier.name = "";
}


} // namespace raum::graph