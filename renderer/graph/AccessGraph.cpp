#include "AccessGraph.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBuffer.h"
#include "RHIImage.h"
#include "RHIImageView.h"
#include "RHIUtils.h"

namespace raum::graph {

namespace {

RenderGraph::VertexType getParentPass(RenderGraph& g, std::string_view name) {
    auto pos = name.find_last_of('/');
    if (pos == std::string::npos) {
        return RenderGraph::null_vertex();
    }
    auto parentName = name.substr(0, pos);
    auto parentVert = *boost::graph::find_vertex(parentName, g.impl());
    return parentVert;
}

ResourceGraph::VertexType getParentResource(ResourceGraph& g, std::string_view name) {
    auto pos = name.find_last_of('/');
    if (pos == std::string::npos) {
        return ResourceGraph::null_vertex();
    }
    auto parentName = name.substr(0, pos);
    auto parentVert = *boost::graph::find_vertex(parentName, g.impl());
    return parentVert;
}

std::string_view eraseView(ResourceGraph& g, std::string_view name) {
    auto v = *boost::graph::find_vertex(name, g.impl());
    if (std::holds_alternative<ImageViewData>(g.impl()[v].data)) {
        const auto& viewData = std::get<ImageViewData>(g.impl()[v].data);
        return viewData.origin;
    } else if (std::holds_alternative<BufferViewData>(g.impl()[v].data)) {
        const auto& viewData = std::get<BufferViewData>(g.impl()[v].data);
        return viewData.origin;
    }
    return name;
}

rhi::AspectMask getDepthStencilReadAspect(ResourceGraph& g, std::string_view name) {
    auto v = *boost::graph::find_vertex(name, g.impl());
    if (std::holds_alternative<ImageViewData>(g.impl()[v].data)) {
        const auto& viewData = std::get<ImageViewData>(g.impl()[v].data);
        return viewData.info.range.aspect;
    }
    return rhi::AspectMask::COLOR;

}

} // namespace

RenderGraph::VertexType getParentPass(RenderGraph& g, const RenderGraph::VertexType& v) {
    return getParentPass(g, g.impl()[v].name);
}

bool isDepthStencil(const AttachmentResource& res) {
    return res.type == ResourceType::DEPTH || res.type == ResourceType::STENCIL || res.type == ResourceType::DEPTH_STENCIL;
}

bool hasDepth(const AttachmentResource& res) {
    return res.type == ResourceType::DEPTH || res.type == ResourceType::DEPTH_STENCIL;
}

// bool hasDepth(const rhi::Format& format) {
// }
//
// bool hasStencil(const rhi::Format& format) {
// }

bool isReadAccess(const rhi::AccessFlags access) {
    return access <= rhi::AccessFlags::SHADING_RATE_ATTACHMENT_READ;
}

rhi::ImageSubresourceRange getSubresourceRange(const Resource& resourceDetail) {
    rhi::ImageSubresourceRange range{};
    std::visit(overloaded{
                   [&](const ImageData& imageData) {
                       bool depthFormat = rhi::hasDepth(imageData.info.format);
                       bool stencilFormat = rhi::hasStencil(imageData.info.format);

                       if (!depthFormat && !stencilFormat) {
                           range.aspect = rhi::AspectMask::COLOR;
                       } else if (depthFormat && stencilFormat) {
                           range.aspect = rhi::AspectMask::DEPTH | rhi::AspectMask::STENCIL;
                       } else if (depthFormat) {
                           range.aspect = rhi::AspectMask::DEPTH;
                       } else if (stencilFormat) {
                           range.aspect = rhi::AspectMask::STENCIL;
                       }
                       range.sliceCount = imageData.info.sliceCount;
                       range.mipCount = imageData.info.mipCount;
                       range.firstMip = 0;
                       range.firstSlice = 0;
                   },
                   [&](const ImageViewData& imageViewData) {
                       range = imageViewData.info.range;
                       // [ VUID-VkImageMemoryBarrier-image-03320 ]
                       // format and barrier access should match if no related extension is enabled
                       if (range.aspect == rhi::AspectMask::DEPTH || range.aspect == rhi::AspectMask::STENCIL) {
                           range.aspect = rhi::AspectMask::DEPTH | rhi::AspectMask::STENCIL;
                       }
                   },
                   [&](const SwapchainData& swapchainData) {
                       range.aspect = rhi::AspectMask::COLOR;
                       range.sliceCount = 1;
                       range.mipCount = 1;
                       range.firstMip = 0;
                       range.firstSlice = 0;
                   },
                   [](const auto& _) {

                   }},
               resourceDetail.data);
    return range;
};

rhi::PipelineStage getPipelineStage(rhi::ShaderStage visibility) {
    auto stage = static_cast<rhi::PipelineStage>(0);
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

rhi::ImageLayout getImageLayout(ResourceGraph& resg, std::string_view name, rhi::AccessFlags flags) {
    auto depthStencilAspect = getDepthStencilReadAspect(resg, name);
    rhi::ImageLayout imgLayout{rhi::ImageLayout::UNDEFINED};
    if (test(flags, rhi::AccessFlags::INPUT_ATTACHMENT_READ)) {
        imgLayout = rhi::ImageLayout::GENERAL;
    } else {
        if (test(flags, rhi::AccessFlags::COLOR_ATTACHMENT_WRITE)) {
            imgLayout = rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        } else if (test(flags, rhi::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE)) {
            imgLayout = rhi::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
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
        // Validation Error: [ VUID-VkImageMemoryBarrier-aspectMask-08703 ]
        if (depthStencilAspect == rhi::AspectMask::DEPTH) {
            // imgLayout = rhi::ImageLayout::DEPTH_READ_ONLY_OPTIMAL;
            imgLayout = rhi::ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } else if (depthStencilAspect == rhi::AspectMask::STENCIL) {
            // imgLayout = rhi::ImageLayout::STENCIL_READ_ONLY_OPTIMAL;
            imgLayout = rhi::ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        } else if (depthStencilAspect == (rhi::AspectMask::DEPTH | rhi::AspectMask::STENCIL)) {
            imgLayout = rhi::ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        }
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

auto decomposeDetail(std::string_view name, ResourceGraph& resg) {
    uint32_t samples{1};
    rhi::Format format{rhi::Format::UNKNOWN};
    uint32_t width{0};
    uint32_t height{0};
    uint32_t sliceCount{1};
    rhi::AspectMask aspect{rhi::AspectMask::COLOR};

    const auto& res = resg.get(name);
    std::visit(
        overloaded{
            [&](const ImageData& img) {
                samples = img.info.sampleCount;
                format = img.info.format;
                width = img.info.extent.x;
                height = img.info.extent.y;
                sliceCount = img.info.sliceCount;
            },
            [&](const ImageViewData& imgView) {
                const auto& originRes = resg.get(imgView.origin);
                const auto& imgInfo = std::get<ImageData>(originRes.data).info;
                samples = imgInfo.sampleCount;
                format = imgView.info.format;
                width = imgInfo.extent.x;
                height = imgInfo.extent.y;
                sliceCount = imgView.info.range.sliceCount;
            },
            [&](const SwapchainData& swapchainData) {
                const auto& swapchain = swapchainData.swapchain;
                samples = 1;
                samples = 1;
                format = swapchain->format();
                width = swapchain->width();
                height = swapchain->height();
                sliceCount = 1;
            },
            [](auto) {
            },
        },
        res.data);
    return std::make_tuple(samples, format, width, height, sliceCount);
}

struct AccessVisitor : public boost::dfs_visitor<> {
public:
    void discover_vertex(const RenderGraphImpl::vertex_descriptor v, const RenderGraphImpl& rg) {
        if (std::holds_alternative<RenderPassData>(rg[v].data)) {
            auto [fbIter, _] = _fbInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(v), std::forward_as_tuple());
            const auto& data = std::get<RenderPassData>(rg[v].data);
            for (const auto& res : data.attachments) {
                // convention: "" represents color/depth outputs
                rhi::AccessFlags access{rhi::AccessFlags::NONE};
                rhi::PipelineStage stage{rhi::PipelineStage::TOP_OF_PIPE};
                if (res.bindingName.empty()) {
                    raum_expect(res.access == Access::WRITE, "");
                    if (isDepthStencil(res)) {
                        access |= rhi::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE;
                        stage = rhi::PipelineStage::LATE_FRAGMENT_TESTS;
                    } else [[likely]] {
                        access |= rhi::AccessFlags::COLOR_ATTACHMENT_WRITE;
                        stage = rhi::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
                    }
                } else {
                    stage = rhi::PipelineStage::FRAGMENT_SHADER;
                    raum_expect(res.access != Access::WRITE, "");
                    if (res.type == ResourceType::SHADING_RATE) {
                        access |= rhi::AccessFlags::SHADING_RATE_ATTACHMENT_READ;
                    } else {
                        access |= rhi::AccessFlags::INPUT_ATTACHMENT_READ;
                    }

                    if (hasDepth(res)) {
                        stage = rhi::PipelineStage::EARLY_FRAGMENT_TESTS;
                    }

                    if (res.access != Access::READ) {
                        if (hasDepth(res)) {
                            access |= rhi::AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE;
                        } else {
                            access |= rhi::AccessFlags::COLOR_ATTACHMENT_WRITE;
                        }
                        stage = rhi::PipelineStage::COLOR_ATTACHMENT_OUTPUT;
                    }
                }
                auto [sampleCount, format, width, height, sliceCount] = decomposeDetail(res.name, _resg);

                auto layout = getImageLayout(_resg, res.name, access);
                auto [iter, _] = _rpInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(v), std::forward_as_tuple());
                iter->second.attachments.emplace_back(
                    res.loadOp,
                    res.storeOp,
                    res.stencilLoadOp,
                    res.stencilStoreOp,
                    format,
                    sampleCount,
                    layout,
                    layout);

                fbIter->second.info.width = fbIter->second.info.width ? std::min(fbIter->second.info.width, width) : width;
                fbIter->second.info.height = fbIter->second.info.height ? std::min(fbIter->second.info.height, height) : height;
                fbIter->second.info.layers = sliceCount;
                fbIter->second.images.emplace_back(res.name);

                auto resourceName = eraseView(_resg, res.name);
                _accessMap[resourceName].emplace_back(v, access, layout, stage);
            }
        } else if (std::holds_alternative<RenderQueueData>(rg[v].data)) {
            const auto& data = std::get<RenderQueueData>(rg[v].data);
            for (const auto& res : data.resources) {
                rhi::AccessFlags access{rhi::AccessFlags::NONE};
                rhi::PipelineStage stage{rhi::PipelineStage::TOP_OF_PIPE};
                const auto& resourceDetail = _resg.get(res.name);
                if (std::holds_alternative<SamplerData>(resourceDetail.data)) {
                    continue;
                }
                if (res.access == Access::READ) {
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

                auto layout = getImageLayout(_resg, res.name, access);

                stage = getPipelineStage(res.visibility);
                auto resourceName = eraseView(_resg, res.name);
                _accessMap[resourceName].emplace_back(v, access, layout, stage);
            }
        } else if (std::holds_alternative<ComputePassData>(rg[v].data)) {
            const auto& data = std::get<ComputePassData>(rg[v].data);
            for (const auto& res : data.resources) {
                rhi::AccessFlags access{rhi::AccessFlags::NONE};
                rhi::PipelineStage stage{rhi::PipelineStage::TOP_OF_PIPE};
                const auto& resourceDetail = _resg.get(res.name);
                if (std::holds_alternative<SamplerData>(resourceDetail.data)) {
                    continue;
                }
                if (res.access == Access::READ) {
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
                stage = getPipelineStage(res.visibility);
                auto layout = getImageLayout(_resg, res.name, access);
                auto resourceName = eraseView(_resg, res.name);
                _accessMap[resourceName].emplace_back(v, access, layout, stage);
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
                     RenderGraph& rg,
                     AccessGraph::BufferBarrierMap& bufferBarrierMap,
                     AccessGraph::ImageBarrierMap& imageBarrierMap,
                     AccessGraph::ImageBarrier& presentBarrier) {
    for (const auto& [name, status] : accessMap) {
        auto& resDetail = resg.get(name);
        auto lastAccess = resDetail.access;
        auto lastStage = rhi::PipelineStage::TOP_OF_PIPE;
        auto lastLayout = rhi::ImageLayout::UNDEFINED;
        std::string_view backbuffer{};
        for (const auto& [v, access, layout, stage] : status) {

            // current perpass barrier
            auto parentPass = getParentPass(rg, v);
            parentPass = parentPass == RenderGraph::null_vertex() ? v : parentPass;

            if (std::holds_alternative<BufferData>(resDetail.data) || std::holds_alternative<BufferViewData>(resDetail.data)) {
                if (isReadAccess(lastAccess) && isReadAccess(access)) {
                    continue;
                }

                bufferBarrierMap[parentPass].emplace_back(
                    name,
                    rhi::BufferBarrierInfo(nullptr,
                                           lastStage,
                                           stage,
                                           lastAccess,
                                           access));

                lastAccess = access;
                lastStage = stage;
                if (resDetail.residency != ResourceResidency::DONT_CARE) {
                    resDetail.access = access;
                }
            } else if (std::holds_alternative<ImageData>(resDetail.data) || std::holds_alternative<ImageViewData>(resDetail.data) || std::holds_alternative<SwapchainData>(resDetail.data)) {
                if (lastLayout == layout) {
                    continue;
                }

                imageBarrierMap[parentPass].emplace_back(
                    name,
                    rhi::ImageBarrierInfo{
                        nullptr,
                        lastStage,
                        stage,
                        lastLayout,
                        layout,
                        lastAccess,
                        access,
                        0, 0,
                        getSubresourceRange(resDetail)});

                lastAccess = access;
                lastStage = stage;
                lastLayout = layout;
                if (resDetail.residency != ResourceResidency::DONT_CARE) {
                    resDetail.access = access;
                }
            }
        }
        if (resDetail.residency == ResourceResidency::SWAPCHAIN) {
            presentBarrier = {
                name,
                rhi::ImageBarrierInfo{
                    nullptr,
                    lastStage,
                    rhi::PipelineStage::BOTTOM_OF_PIPE,
                    lastLayout,
                    rhi::ImageLayout::PRESENT,
                    lastAccess,
                    rhi::AccessFlags::NONE,
                    0, 0,
                    getSubresourceRange(resDetail)}};
            resDetail.access = rhi::AccessFlags::NONE;
        }
    }
}

AccessGraph::AccessGraph(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& sg) : _rg(rg), _resg(resg), _sg(sg) {}

void AccessGraph::analyze() {
    auto indexMap = boost::get(boost::vertex_index, _rg.impl());
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    AccessVisitor visitor{{}, _resg, _sg, _accessMap, _renderPassInfoMap, _frameBufferInfoMap};
    for (auto vertex : boost::make_iterator_range(boost::vertices(_rg.impl()))) {
        if (!std::holds_alternative<RenderQueueData>(_rg.impl()[vertex].data)) {
            boost::depth_first_visit(_rg.impl(), vertex, visitor, colorMap);
        }
    }

    populateBarrier(_accessMap,  _resg, _rg, _bufferBarrierMap, _imageBarrierMap, _presentBarrier);
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
    if (!_presentBarrier.name.empty()) {
        return &_presentBarrier;
    }
    return nullptr;
}

rhi::ImageLayout AccessGraph::getImageLayout(std::string_view name, unsigned long long v) {
    auto res = rhi::ImageLayout::UNDEFINED;
    auto dsReadAspect = getDepthStencilReadAspect(_resg, name);

    auto resName = eraseView(_resg, name);
    if (_accessMap.contains(resName)) {
        const auto& accesses = _accessMap.at(resName);
        auto iter = std::find_if(accesses.begin(), accesses.end(),
                                 [v](const Access& access) {
                                     return access.v == v;
                                 });
        if (iter != accesses.end()) {
            res = iter->layout;
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