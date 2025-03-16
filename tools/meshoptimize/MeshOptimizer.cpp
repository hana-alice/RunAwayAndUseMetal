#include "MeshOptimizer.h"
#include "core/define.h"
#include "exec/static_thread_pool.hpp"
#include "stdexec/execution.hpp"
#include "tiny_gltf.h"
namespace raum::tools {

constexpr uint32_t MESH_THREAD_NUM{10};
constexpr uint32_t IMAGE_THREAD_NUM{5};

MeshOpt::MeshOpt(const Options& opt) : _options(opt) {
}

namespace {
void loadGLTF(const std::filesystem::path& filePath, tinygltf::Model& rawModel) {
    std::string err;
    std::string warn;
    tinygltf::TinyGLTF loader;
    bool res = loader.LoadASCIIFromFile(&rawModel, &err, &warn, filePath.string());
    raum_check(res, "failed to load scene from {}, tinyGLTF: {}", filePath.string(), err);

    if constexpr (raum_debug) {
        if (!warn.empty()) {
            raum_warn("tinyGLTF: {}", warn);
        }
    }
}

void processMesh(tinygltf::Mesh& mesh, tinygltf::Model& rawModel) {
    const auto& rawNode = rawModel.nodes[nodeIndex];
    const auto& rawMesh = rawModel.meshes[rawNode.mesh];
    auto& modelNode = sg.addModel(rawMesh.name, parentName);
    auto& sceneNode = sg.get(rawMesh.name);
    applyNodeTransform(rawNode, sceneNode);

    modelNode.model = std::make_shared<scene::Model>();
    auto& model = *modelNode.model;

    const auto& accessors = rawModel.accessors;
    const auto& rawBuffers = rawModel.buffers;
    const auto& rawBufferViews = rawModel.bufferViews;
    // TODO: Morph.
    for (const auto& prim : rawMesh.primitives) {
        auto mesh = std::make_shared<scene::Mesh>();
        auto& meshData = mesh->meshData();
        const float* position{nullptr};
        const float* normal{nullptr};
        const float* uv{nullptr};
        const float* tangent{nullptr};
        const float* color{nullptr};
        bool color4{true};
        for (const auto& [attrName, accessorIndex] : prim.attributes) {
            if (attrName == "POSITION") {
                auto viewIndex = accessors[accessorIndex].bufferView;
                const auto& buffer = rawBuffers[rawBufferViews[viewIndex].buffer];
                position = reinterpret_cast<const float*>(buffer.data.data() + accessors[accessorIndex].byteOffset + rawBufferViews[viewIndex].byteOffset);
                meshData.vertexCount = accessors[accessorIndex].count;
            } else if (attrName == "NORMAL") {
                auto viewIndex = accessors[accessorIndex].bufferView;
                const auto& buffer = rawBuffers[rawBufferViews[viewIndex].buffer];
                normal = reinterpret_cast<const float*>(buffer.data.data() + accessors[accessorIndex].byteOffset + rawBufferViews[viewIndex].byteOffset);
            } else if (attrName == "TEXCOORD_0") {
                auto viewIndex = accessors[accessorIndex].bufferView;
                const auto& buffer = rawBuffers[rawBufferViews[viewIndex].buffer];
                uv = reinterpret_cast<const float*>(buffer.data.data() + accessors[accessorIndex].byteOffset + rawBufferViews[viewIndex].byteOffset);
            } else if (attrName == "TANGENT") {
                auto viewIndex = accessors[accessorIndex].bufferView;
                const auto& buffer = rawBuffers[rawBufferViews[viewIndex].buffer];
                tangent = reinterpret_cast<const float*>(buffer.data.data() + accessors[accessorIndex].byteOffset + rawBufferViews[viewIndex].byteOffset);
            } else if (attrName == "COLOR") {
                auto viewIndex = accessors[accessorIndex].bufferView;
                const auto& buffer = rawBuffers[rawBufferViews[viewIndex].buffer];
                color = reinterpret_cast<const float*>(buffer.data.data() + accessors[accessorIndex].byteOffset + rawBufferViews[viewIndex].byteOffset);
                color4 = accessors[accessorIndex].type == TINYGLTF_TYPE_VEC4;
            } else {
                raum_warn("ignored vertex attribute: {}", attrName);
            }
        }
        std::vector<float> data;
        auto eleNum = (!!position) * 3 + (!!normal) * 3 + (!!uv) * 2 + (!!tangent) * 4 + (!!color * 4);
        data.resize(eleNum * 4 * meshData.vertexCount);
        for (size_t i = 0; i < meshData.vertexCount; ++i) {
            uint32_t offset{0};
            if (position) [[likely]] {
                data[i * eleNum] = position[i * 3];
                data[i * eleNum + 1] = position[i * 3 + 1];
                data[i * eleNum + 2] = position[i * 3 + 2];
                offset += 3;
            }
            if (normal) {
                data[i * eleNum + offset] = normal[i * 3];
                data[i * eleNum + offset + 1] = normal[i * 3 + 1];
                data[i * eleNum + offset + 2] = normal[i * 3 + 2];
                offset += 3;
            }
            if (uv) {
                data[i * eleNum + offset] = uv[i * 2];
                data[i * eleNum + offset + 1] = uv[i * 2 + 1];
                offset += 2;
            }
            if (tangent) {
                data[i * eleNum + offset] = tangent[i * 4];
                data[i * eleNum + offset + 1] = tangent[i * 4 + 1];
                data[i * eleNum + offset + 2] = tangent[i * 4 + 2];
                data[i * eleNum + offset + 3] = tangent[i * 4 + 3];
            }
            if (color) {
                uint32_t colorStride = 4;
                if (color4) {
                    data[i * eleNum + 3 + offset] = color[i * colorStride + 3];
                    offset += 4;
                } else {
                    colorStride = 3;
                    offset += 3;
                }

                data[i * eleNum + offset] = color[i * colorStride];
                data[i * eleNum + offset + 1] = color[i * colorStride + 1];
                data[i * eleNum + offset + 2] = color[i * colorStride + 2];
            }
        }

        scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/gltfpbr");

        auto& vertexLayout = meshData.vertexLayout;
        uint32_t location{0};
        uint32_t stride{0};
        if (position) [[likely]] {
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 3;
            meshData.shaderAttrs |= scene::ShaderAttribute::POSITION;
        }
        if (normal) {
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 3;
            meshData.shaderAttrs |= scene::ShaderAttribute::NORMAL;
            matTemplate->addDefine("VERTEX_NORMAL");
        }
        if (uv) {
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 2;
            meshData.shaderAttrs |= scene::ShaderAttribute::UV;
            matTemplate->addDefine("VERTEX_UV");
        }
        if (tangent) {
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGBA32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 4;
            meshData.shaderAttrs |= scene::ShaderAttribute::TANGENT;
            matTemplate->addDefine("VERTEX_TANGENT");
        }
        if (color) {
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGBA32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 4;
            meshData.shaderAttrs |= scene::ShaderAttribute::COLOR;
            matTemplate->addDefine("VERTEX_COLOR");
        }
        rhi::BufferSourceInfo bufferSourceInfo{
            .bufferUsage = rhi::BufferUsage::VERTEX,
            .size = static_cast<uint32_t>(data.size() * sizeof(float)),
            .data = data.data(),
        };
        meshData.vertexBuffer.buffer = rhi::BufferPtr(device->createBuffer(bufferSourceInfo));
        auto& bufferAttribute = vertexLayout.vertexBufferAttrs.emplace_back();
        bufferAttribute.binding = 0;
        bufferAttribute.rate = rhi::InputRate::PER_VERTEX;
        bufferAttribute.stride = stride * static_cast<uint32_t>(sizeof(float));

        rhi::BufferSourceInfo indexBufferSource{
            .bufferUsage = rhi::BufferUsage::INDEX,
        };
        const auto& indicesAccessor = accessors[prim.indices];
        meshData.indexCount = indicesAccessor.count;
        const auto& rawIndexBuffer = rawBuffers[rawBufferViews[indicesAccessor.bufferView].buffer];
        const auto* indexBufferData = rawIndexBuffer.data.data() + indicesAccessor.byteOffset + rawBufferViews[indicesAccessor.bufferView].byteOffset;
        std::vector<uint16_t> u16arr;
        switch (indicesAccessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                meshData.indexBuffer.type = rhi::IndexType::FULL;
                indexBufferSource.size = meshData.indexCount * sizeof(rhi::FullIndexType);
                indexBufferSource.data = indexBufferData;
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                meshData.indexBuffer.type = rhi::IndexType::HALF;
                indexBufferSource.size = meshData.indexCount * sizeof(rhi::HalfIndexType);
                indexBufferSource.data = indexBufferData;
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                u16arr.resize(meshData.indexCount);
                for (size_t i = 0; i < meshData.indexCount; ++i) {
                    u16arr[i] = indexBufferData[i];
                }
                meshData.indexBuffer.type = rhi::IndexType::HALF;
                indexBufferSource.size = meshData.indexCount * sizeof(rhi::HalfIndexType);
                indexBufferSource.data = u16arr.data();
                break;
            }
        }
        meshData.indexBuffer.buffer = rhi::BufferPtr(device->createBuffer(indexBufferSource));

        auto localMatIndex = prim.material;
        if (!techs.contains(localMatIndex)) {
            loadMaterial(rawModel, localMatIndex, textures, matTemplate, techs, device);
        }
        auto tech = techs.at(localMatIndex);
        if (prim.mode == TINYGLTF_MODE_TRIANGLES) {
            techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
        } else if (prim.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
            techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_STRIP);
        } else if (prim.mode == TINYGLTF_MODE_POINTS) {
            techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::POINT_LIST);
        } else if (prim.mode == TINYGLTF_MODE_LINE) {
            techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::LINE_LIST);
        } else if (prim.mode == TINYGLTF_MODE_LINE_STRIP) {
            techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::LINE_STRIP);
        } else {
            raum_error("primitive:{} not supported.", prim.mode);
        }

        auto meshRenderer = model.meshRenderers().emplace_back(std::make_shared<scene::MeshRenderer>(mesh));
        meshRenderer->addTechnique(tech);
        meshRenderer->setVertexInfo(0, meshData.vertexCount, meshData.indexCount);
        meshRenderer->setTransform(sceneNode.node.transform());
        meshRenderer->setTransformSlot("LocalMat");

        auto embededTechSize = static_cast<uint32_t>(scene::EmbededTechnique::COUNT);
        for (size_t i = 0; i < embededTechSize; ++i) {
            meshRenderer->addTechnique(scene::makeEmbededTechnique(static_cast<scene::EmbededTechnique>(i)));
        }
    }
}

void processImage() {

}

} // namespace

void MeshOpt::run(const std::filesystem::path& filePath) {
    std::string err;
    std::string warn;
    tinygltf::Model rawModel;
    loadGLTF(filePath, rawModel);
    // https://github.com/NVIDIA/stdexec?tab=readme-ov-file#example
    exec::static_thread_pool pool{MESH_THREAD_NUM + IMAGE_THREAD_NUM};
    auto sched = pool.get_scheduler();
    auto meshThreadNum = rawModel.meshes.size() > MESH_THREAD_NUM ?
        MESH_THREAD_NUM : (rawModel.meshes.size() + 3)  / 4;
    auto imageThreadNum = rawModel.images.size() > IMAGE_THREAD_NUM ?
        IMAGE_THREAD_NUM : rawModel.images.size();

    auto meshTask = stdexec::bulk(meshThreadNum, [&](int index) {

    });

    auto imageTask = stdexec::bulk(imageThreadNum, [&](int index) {

    });

}

} // namespace raum::tools