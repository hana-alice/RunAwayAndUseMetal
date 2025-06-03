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
        for (size_t i = 0; i < framebufferInfo->images.size(); ++i) {
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

bool culled(const scene::Mesh& mesh, const SceneGraph& sg) {
    bool culled = false;

    const auto& graph = sg.impl();
    // todo: Traits camera only.
    for (auto v : boost::make_iterator_range(boost::vertices(graph))) {
        if (std::holds_alternative<CameraNode>(graph[v].sceneNodeData)) {
            const auto& camNode = std::get<CameraNode>(graph[v].sceneNodeData);
            auto cam = camNode.camera;
            if (cam->cullingEnabled()) {
                culled |= !scene::frustumCulling(cam->frustumPlanes(), mesh.aabb());
            }
        }
    }
    return culled;
}

void collectRenderables(
    std::vector<scene::RenderablePtr>& renderables,
    std::span<scene::RenderablePtr>& cullableRenderables,
    std::span<scene::RenderablePtr>& nocullRenderables,
    const SceneGraph& sg) {
    const auto& graph = sg.impl();
    std::vector<scene::MeshRendererPtr> noCullings;
    std::vector<scene::MeshRendererPtr> cullables;
    for (auto v : boost::make_iterator_range(boost::vertices(graph))) {
        if (std::holds_alternative<ModelNode>(graph[v].sceneNodeData)) {
            const auto& modelNode = std::get<ModelNode>(graph[v].sceneNodeData);
            if (graph[v].node.enabled()) {
                for (auto& meshRenderer : modelNode.model->meshRenderers()) {
                    if (!test(modelNode.hint, ModelHint::NO_CULLING)) {
                        cullables.emplace_back(meshRenderer);
                    } else {
                        noCullings.emplace_back(meshRenderer);
                    }
                }
            }
        }
    }
    std::ranges::sort(cullableRenderables, [](const auto& lhs, const auto& rhs) {
        return std::static_pointer_cast<scene::MeshRenderer>(lhs)->mesh()->aabb().minBound.x <
               std::static_pointer_cast<scene::MeshRenderer>(rhs)->mesh()->aabb().minBound.x;
    });
    std::ranges::copy(cullables, std::back_inserter(renderables));
    std::ranges::copy(noCullings, std::back_inserter(renderables));
    cullableRenderables = std::span(renderables).subspan(0, cullables.size());
    nocullRenderables = std::span(renderables).subspan(cullables.size());
}

namespace {
// Helper function to build a BVH node
scene::AABB getAABB(const std::span<scene::RenderablePtr>& renderables) {
    scene::AABB aabb{};
    aabb.minBound = Vec3f(std::numeric_limits<float>::max());
    aabb.maxBound = Vec3f(std::numeric_limits<float>::lowest());
    for (const auto& renderable : renderables) {
        auto meshRenderer = std::static_pointer_cast<const scene::MeshRenderer>(renderable);
        const auto& meshAABB = meshRenderer->mesh()->aabb();
        aabb.minBound = glm::min(aabb.minBound, meshAABB.minBound);
        aabb.maxBound = glm::max(aabb.maxBound, meshAABB.maxBound);
    }
    return aabb;
}

} // namespace

scene::BVHNode* buildBVH(std::span<scene::RenderablePtr>& renderables, uint32_t maxObjectsPerNode) {
    // expect renderables to be sorted

    if (renderables.size() <= maxObjectsPerNode) {
        scene::BVHNode* leafNode = new scene::BVHNode();
        leafNode->aabb = getAABB(renderables);
        leafNode->children = renderables;
        return leafNode;
    }

    std::span<scene::RenderablePtr> lefts = renderables.subspan(0, renderables.size() / 2);
    std::span<scene::RenderablePtr> rights = renderables.subspan(renderables.size() / 2);

    scene::BVHNode* node = new scene::BVHNode();
    node->aabb = getAABB(renderables);
    if (!lefts.empty()) {
        node->left = buildBVH(lefts, maxObjectsPerNode);
    }
    if (!rights.empty()) {
        node->right = buildBVH(rights, maxObjectsPerNode);
    }
    return node;
}

void BVHCulling(const scene::FrustumPlanes& frustumPlanes,
                const scene::BVHNode* node,
                std::vector<scene::RenderablePtr>& renderables) {
    if (!node) {
        return;
    }
    if (scene::frustumCulling(frustumPlanes, node->aabb)) {
        if (!node->children.empty()) {
            for (const auto& renderable : node->children) {
                const auto meshRenderer = std::static_pointer_cast<const scene::MeshRenderer>(renderable);
                if (scene::frustumCulling(frustumPlanes, meshRenderer->mesh()->aabb())) {
                    renderables.emplace_back(renderable);
                }
            }
        } else {
            BVHCulling(frustumPlanes, node->left, renderables);
            BVHCulling(frustumPlanes, node->right, renderables);
        }
    }
}

void BVHCulling(const std::vector<CameraNode*>& cameras,
                const scene::BVHNode* node,
                std::vector<scene::RenderablePtr>& renderables) {
    for (const auto& camera : cameras) {
        const auto& frustum = camera->camera->frustumPlanes();
        BVHCulling(frustum, node, renderables);
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

void bindResourceToMaterial(std::string_view resourceName, std::string_view slotName, scene::MaterialPtr mat, ResourceGraph& resg) {
    auto imgView = resg.getImageView(resourceName);
    auto img = resg.getImage(resourceName);
    scene::Texture texture{img, imgView, 0};
    mat->set(slotName, texture);
}

} // namespace raum::graph