#pragma once
#include <span>
#include "AccessGraph.h"
#include "Material.h"
#include "RHIDevice.h"
#include "RenderGraph.h"
#include "SceneGraph.h"
#include "ShaderGraph.h"
#include "core/utils/Archive.h"

namespace raum {
template <class Archive>
void serialize(Archive& ar, raum::graph::CameraNode& cameraNode) {
    ar(cameraNode.camera);
}

template <class Archive>
void serialize(Archive& ar, raum::graph::ModelNode& modelNode) {
    ar(modelNode.model);
    ar(modelNode.hint);
}

template <class Archive>
void serialize(Archive& ar, raum::graph::LightNode& lightNode) {
    ar(lightNode.light);
}

template <class Archive>
void serialize(Archive& ar, raum::graph::EmptyNode& emptyNode) {
    // ar(emptyNode);
}

template <class Archive>
void serialize(Archive& ar, raum::graph::SceneNode& node) {
    ar(node.name);
    ar(node.node);
    ar(node.sceneNodeData);
}


template<>
inline void utils::InputArchive::read(graph::SceneGraph& sg) {
    auto& ar = *iarchive;
    sg.reset();

    auto vnum{0};
    ar >> vnum;

    for (int i = 0; i < vnum; i++) {
        graph::SceneNode node;
        ar >> node.name;
        ar >> node.node;
        ar >> node.sceneNodeData;

        std::visit(overloaded{
                       [&](graph::ModelNode&) {
                           sg.addModel(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](graph::CameraNode&) {
                           sg.addCamera(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](graph::LightNode&) {
                           sg.addLight(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](graph::EmptyNode&) {
                           sg.addEmpty(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](auto&& arg) {},
                   },
                   node.sceneNodeData);
    }

    auto eNum{0};
    ar >> eNum;
    auto& impl = sg.impl();
    for (int i = 0; i < eNum; i++) {
        graph::SceneGraph::VertexType v1, v2;
        ar(v1, v2);
        add_edge(v1, v2, impl);
    }
}

template<>
inline void utils::OutputArchive::write(const graph::SceneGraph& sg) {
    const auto& graph = sg.impl();
    auto& ar = *oarchive;
    ar << boost::num_vertices(graph);
    for (auto [it, end] = vertices(graph); it != end; ++it) {
        auto v = *it;
        auto& node = graph[v];
        ar << node.name;
        ar << node.node;
        ar << node.sceneNodeData;
    }
    ar << boost::num_edges(graph);
    for (auto [it, end] = edges(graph); it != end; ++it) {
        auto e = *it;
        ar(e.m_source, e.m_target);
    }
}

namespace graph {
rhi::RenderPassPtr getOrCreateRenderPass(const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device);

rhi::FrameBufferPtr getOrCreateFrameBuffer(rhi::RenderPassPtr renderpass,
                                           const RenderGraph::VertexType v,
                                           AccessGraph& ag,
                                           ResourceGraph& resg,
                                           rhi::DevicePtr device,
                                           rhi::SwapchainPtr swapchain);

rhi::GraphicsPipelinePtr getOrCreateGraphicsPipeline(rhi::RenderPassPtr renderPass,
                                                     scene::MaterialPtr material,
                                                     ShaderGraph& shg);

bool culling(const ModelNode& node);

void collectRenderables(std::vector<scene::RenderablePtr>& renderables,
                        std::span<scene::RenderablePtr>& cullableRenderables,
                        std::span<scene::RenderablePtr>& nocullRenderables,
                        const SceneGraph& sg);

void BVHCulling(const std::vector<CameraNode*>& cameras,
                const scene::BVHNode* node,
                std::vector<scene::RenderablePtr>& renderables);

scene::BVHNode* buildBVH(std::span<scene::RenderablePtr>& renderables, uint32_t maxObjectsPerNode = 4);

void warmUp(SceneGraph& sg, ShaderGraph& shg, rhi::DevicePtr device);

std::string_view getPhaseName(std::string_view queueName);

void bindResourceToMaterial(std::string_view resourceName, std::string_view slotName, scene::MaterialPtr mat, ResourceGraph& resg);

scene::MeshRendererPtr getLocalQuad(rhi::DevicePtr device);

}

} // namespace raum
