#include "GraphUtils.h"
#include "RHIDevice.h"

namespace raum::graph {

namespace {
static std::unordered_map<rhi::RenderPassInfo, rhi::RenderPassPtr, rhi::RHIHash<rhi::RenderPassInfo>> _renderPassMap;
static std::unordered_map<size_t, rhi::GraphicsPipelinePtr> _psoMap;
static std::unordered_map<size_t, rhi::DescriptorSetLayoutPtr> _descLayoutMap;
static std::unordered_map<size_t, rhi::PipelineLayoutPtr> _pplLayoutMap;

class FrameBufferManager {
public:
    FrameBufferManager(rhi::RHIDevice* device, rhi::RHISwapchain* swapchain)
    : _device(device), _swapchain(swapchain) {
        _swapchainFrameBuffers.resize(swapchain->imageCount());
    }

    rhi::FrameBufferPtr get(const rhi::FrameBufferInfo& info) {
        auto hash = rhi::RHIHash<rhi::FrameBufferInfo>{}(info);
        if (!_frameBufferMap.contains(hash)) {
            // invalidate swapchain-related framebuffers in case of pointer ABA problem
            bool hasSwapchainView{false};
            for (auto* imgView : info.images) {
                if (_swapchain->holds(imgView->image())) {
                    auto& currFramebuffers = _swapchainFrameBuffers[_swapchain->imageIndex()];
                    for (const auto& val : currFramebuffers) {
                        _frameBufferMap.erase(val);
                    }
                    currFramebuffers.clear();
                    hasSwapchainView = true;
                }
            }
            if (hasSwapchainView) {
                _swapchainFrameBuffers[_swapchain->imageIndex()].emplace_back(hash);
            }
            _frameBufferMap[hash] = rhi::FrameBufferPtr(_device->createFrameBuffer(info));
        }
        return _frameBufferMap.at(hash);
    }

    rhi::RHIDevice* _device;
    rhi::RHISwapchain* _swapchain;
    std::vector<std::vector<std::size_t>> _swapchainFrameBuffers;
    std::unordered_map<std::size_t, rhi::FrameBufferPtr> _frameBufferMap;
};

} // namespace

rhi::RenderPassPtr getOrCreateRenderPass(const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device) {
    auto* renderpassInfo = ag.getRenderPassInfo(v);
    raum_check(renderpassInfo, "failed to analyze renderpass info at {}", v);
    if (renderpassInfo) {
        if (!_renderPassMap.contains(*renderpassInfo)) {
            _renderPassMap[*renderpassInfo] = rhi::RenderPassPtr(device->createRenderPass(*renderpassInfo));
        }
        return _renderPassMap[*renderpassInfo];
    }
    raum_unreachable();
    return nullptr;
}



rhi::FrameBufferPtr getOrCreateFrameBuffer(
    rhi::RenderPassPtr renderpass,
    const RenderGraph::VertexType v,
    AccessGraph& ag,
    ResourceGraph& resg,
    rhi::DevicePtr device,
    rhi::SwapchainPtr swapchain) {

    static FrameBufferManager fbManager(device.get(), swapchain.get());

    auto* framebufferInfo = ag.getFrameBufferInfo(v);
    raum_check(framebufferInfo, "failed to analyze framebuffer info at {}", v);
    if (framebufferInfo) {
        auto& rhiFbInfo = framebufferInfo->info;
        rhiFbInfo.images.resize(framebufferInfo->images.size());
        rhiFbInfo.renderPass = renderpass.get();
        for(size_t i = 0; i < framebufferInfo->images.size(); ++i) {
            auto resName = framebufferInfo->images[i];
            auto imgView = resg.getImageView(resName);
            rhiFbInfo.images[i] = imgView.get();
        }

        return fbManager.get(rhiFbInfo);
    }
    raum_unreachable();
    return nullptr;
}

rhi::DescriptorSetLayoutPtr getOrCreateDescriptorSetLayout(const rhi::DescriptorSetLayoutInfo& info, rhi::DevicePtr device) {
    size_t seed = rhi::RHIHash<rhi::DescriptorSetLayoutInfo>{}(info);
    if (!_descLayoutMap.contains(seed)) {
        _descLayoutMap[seed] = rhi::DescriptorSetLayoutPtr(device->createDescriptorSetLayout(info));
    }
    return _descLayoutMap.at(seed);
}

rhi::PipelineLayoutPtr getOrCreatePipelineLayout(const rhi::PipelineLayoutInfo& info, rhi::DevicePtr device) {
    size_t seed = 9527;
    for (auto* descLayout : info.setLayouts) {
        boost::hash_combine(seed, descLayout);
    }
    for (const auto& constantRange : info.pushConstantRanges) {
        boost::hash_combine(seed, constantRange.stage);
        boost::hash_combine(seed, constantRange.offset);
        boost::hash_combine(seed, constantRange.size);
    }
    if (!_pplLayoutMap.contains(seed)) {
        _pplLayoutMap[seed] = rhi::PipelineLayoutPtr(device->createPipelineLayout(info));
    }
    return _pplLayoutMap.at(seed);
}

bool culling(const ModelNode& node) {
    return true;
}

void collectRenderables(std::vector<scene::RenderablePtr>& renderables, const SceneGraph& sg, bool enableCullling) {
    const auto& graph = sg.impl();
    for (auto v : boost::make_iterator_range(boost::vertices(graph))) {
        if (std::holds_alternative<ModelNode>(graph[v].sceneNodeData)) {
            const auto& modelNode = std::get<ModelNode>(graph[v].sceneNodeData);
            if ((culling(modelNode) || !enableCullling) && graph[v].node.enabled()) {
                for (auto& meshRenderer : modelNode.model->meshRenderers()) {
                    renderables.emplace_back(meshRenderer);
                }
            }
        }
    }
}

std::string_view getPhaseName(std::string_view queueName) {
    auto index = queueName.find_last_of('/');
    if (index != std::string_view::npos) {
        return queueName.substr(index + 1);
    } else {
        return {};
    }
}


} // namespace raum::graph