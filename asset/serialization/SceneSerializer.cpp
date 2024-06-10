#include "SceneSerializer.h"
#include <numeric>
#include "Mesh.h"
#include "PBRMaterial.h"
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIUtils.h"
#include "Technique.h"
#include "core/define.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace raum::asset::serialize {

void loadTextures(std::vector<scene::Texture>& textures,
                  const tinygltf::Model& rawModel,
                  rhi::CommandBufferPtr cmdBuffer,
                  rhi::DevicePtr device) {
    const auto& mats = rawModel.materials;
    auto blitEncoder = rhi::BlitEncoderPtr(cmdBuffer->makeBlitEncoder());
    for (const auto& res : rawModel.images) {
        rhi::ImageInfo info{};
        info.extent = {res.width, res.height, 1};
        info.usage = rhi::ImageUsage::TRANSFER_DST | rhi::ImageUsage::SAMPLED;
        raum_check(res.bits == 8, "image bits not supported");
        raum_check(res.component == 4, "image channel count not supported");
        info.format = rhi::Format::RGBA8_UNORM;
        auto img = rhi::ImagePtr(device->createImage(info));

        rhi::ImageBarrierInfo barrierInfo{
            .image = img.get(),
            .dstStage = rhi::PipelineStage::TRANSFER,
            .newLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL,
            .dstAccessFlag = rhi::AccessFlags::TRANSFER_WRITE,
            .range = {
                .aspect = rhi::AspectMask::COLOR,
                .sliceCount = 1,
                .mipCount = 1,
            },
        };
        cmdBuffer->appendImageBarrier(barrierInfo);
        cmdBuffer->applyBarrier({});

        auto bufferSize = res.width * res.height * rhi::getFormatSize(info.format);
        rhi::BufferSourceInfo bufferInfo{
            .bufferUsage = rhi::BufferUsage::TRANSFER_SRC,
            .size = bufferSize,
            .data = res.image.data(),
        };
        auto stagingBuffer = rhi::BufferPtr(device->createBuffer(bufferInfo));
        rhi::BufferImageCopyRegion region{
            .bufferSize = bufferSize,
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageAspect = rhi::AspectMask::COLOR,
            .imageExtent = {
                res.width,
                res.height,
                1,
            },
        };
        blitEncoder->copyBufferToImage(stagingBuffer.get(),
                                       img.get(),
                                       rhi::ImageLayout::TRANSFER_DST_OPTIMAL,
                                       &region,
                                       1);
        barrierInfo.oldLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL;
        barrierInfo.newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
        barrierInfo.srcAccessFlag = rhi::AccessFlags::TRANSFER_WRITE;
        barrierInfo.dstAccessFlag = rhi::AccessFlags::SHADER_READ;
        barrierInfo.srcStage = rhi::PipelineStage::TRANSFER;
        barrierInfo.dstStage = rhi::PipelineStage::VERTEX_SHADER;
        cmdBuffer->appendImageBarrier(barrierInfo);
        cmdBuffer->applyBarrier({});

        rhi::ImageViewInfo viewInfo{};
        viewInfo.type = rhi::ImageViewType::IMAGE_VIEW_2D;
        viewInfo.image = img.get();
        viewInfo.format = info.format;
        viewInfo.range = {
            .aspect = rhi::AspectMask::COLOR,
            .firstSlice = 0,
            .sliceCount = 1,
            .firstMip = 0,
            .mipCount = 1};

        auto imgView = rhi::ImageViewPtr(device->createImageView(viewInfo));
        textures.emplace_back(scene::Texture{res.name, img, imgView});

        cmdBuffer->onComplete([stagingBuffer]() mutable {
            stagingBuffer.reset();
        });
    }
}

void loadMaterials(std::vector<scene::TechniquePtr>& techs,
                   std::vector<scene::Texture>& textures,
                   const tinygltf::Model& rawModel) {
    for (const auto& res : rawModel.materials) {
        scene::MaterialTemplatePtr matTemplate = scene::getOrCreateMaterialTemplate("asset/layout/cook-torrance");
        scene::MaterialPtr mat = matTemplate->instantiate(res.name, scene::MaterialType::PBR);
        auto pbrMat = std::static_pointer_cast<scene::PBRMaterial>(mat);
        auto& tech = techs.emplace_back(std::make_shared<scene::Technique>(mat, "default"));
        auto& ds = tech->depthStencilInfo();
        ds.depthTestEnable = true;
        ds.depthWriteEnable = true;

        const auto& ef = res.emissiveFactor;
        raum_check(ef.size() == 3, "Unexpected emissive factor components!");
        pbrMat->setEmissiveFactor(ef[0], ef[1], ef[2]);

        if (res.alphaMode == "OPAQUE") {
            // TODO: cmake introduce <wingdi.h> cause `OPAQUE` was preprocessed by macro.
            pbrMat->setAlphaMode(scene::PBRMaterial::AlphaMode::AM_OPAQUE);
        } else if (res.alphaMode == "MASK") {
            pbrMat->setAlphaMode(scene::PBRMaterial::AlphaMode::AM_MASK);
        } else if (res.alphaMode == "BLEND") {
            pbrMat->setAlphaMode(scene::PBRMaterial::AlphaMode::AM_BLEND);
        }
        pbrMat->setDoubleSided(res.doubleSided);
        pbrMat->setAlphaCutoff(res.alphaCutoff);

        const auto& pmr = res.pbrMetallicRoughness;
        const auto& bcf = pmr.baseColorFactor;

        const auto& bc = pmr.baseColorTexture;
        if (bc.index != -1) {
            auto& baseColor = textures[pmr.baseColorTexture.index];
            baseColor.uvIndex = pmr.baseColorTexture.texCoord;
            baseColor.name = "albedoMap";
            pbrMat->add(baseColor);
            pbrMat->setBaseColorFactor(bcf[0], bcf[1], bcf[2], bcf[3]);
        }

        const auto& mr = pmr.metallicRoughnessTexture;
        if (mr.index != -1) {
            auto& metallicRoughness = textures[pmr.metallicRoughnessTexture.index];
            metallicRoughness.uvIndex = pmr.metallicRoughnessTexture.texCoord;
            metallicRoughness.name = "metallicRoughnessMap";
            pbrMat->add(metallicRoughness);
            pbrMat->setMetallicFactor(pmr.metallicFactor);
            pbrMat->setRoughnessFactor(pmr.roughnessFactor);
        }

        const auto& nt = res.normalTexture;
        if (nt.index != -1) {
            auto& normal = textures[nt.index];
            normal.uvIndex = nt.texCoord;
            normal.name = "normalMap";
            pbrMat->add(normal);
            pbrMat->setNormalScale(nt.scale);
        }

        const auto& ot = res.occlusionTexture;
        if (ot.index != -1) {
            auto& occlusion = textures[ot.index];
            occlusion.uvIndex = ot.texCoord;
            occlusion.name = "aoMap";
            pbrMat->add(occlusion);
        }

        const auto& et = res.emissiveTexture;
        if (et.index != -1) {
            auto& emissive = textures[et.index];
            emissive.uvIndex = et.texCoord;
            emissive.name = "emissiveMap";
            pbrMat->add(emissive);
        }
    }
}

void applyNodeTransform(const tinygltf::Node& rawNode, graph::SceneNode& node) {
    const auto& scale = rawNode.scale;
    const auto& trans = rawNode.translation;
    const auto& rot = rawNode.rotation;
    if (!scale.empty()) {
        node.node.setScale({scale[0], scale[1], scale[2]});
    }
    if (!trans.empty()) {
        node.node.setTranslation({trans[0], trans[1], trans[2]});
    }
    if (!rot.empty()) {
        node.node.setOrientation({
            static_cast<float>(rot[3]),
            static_cast<float>(rot[0]),
            static_cast<float>(rot[1]),
            static_cast<float>(rot[2]),
        });
    }
    node.node.update();
}

void loadMesh(const tinygltf::Model& rawModel,
              const tinygltf::Node& rawNode,
              graph::SceneGraph& sg,
              std::string_view parentName,
              std::vector<scene::TechniquePtr>& techs,
              rhi::CommandBufferPtr cmdBuffer,
              rhi::DevicePtr device) {
    const auto& rawMesh = rawModel.meshes[rawNode.mesh];
    auto& node = sg.addModel(rawMesh.name, parentName);
    applyNodeTransform(rawNode, node);

    auto& modelNode = std::get<graph::ModelNode>(node.sceneNodeData);
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
        }
        rhi::BufferSourceInfo bufferSourceInfo{
            .bufferUsage = rhi::BufferUsage::VERTEX,
            .size = static_cast<uint32_t>(data.size() * sizeof(float)),
            .data = data.data(),
        };
        meshData.vertexBuffer.buffer = device->createBuffer(bufferSourceInfo);
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
        meshData.indexBuffer.buffer = device->createBuffer(indexBufferSource);

        auto localMatIndex = prim.material;
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
        meshRenderer->addTechnique(techs[localMatIndex]);
        meshRenderer->setVertexInfo(0, meshData.vertexCount, meshData.indexCount);
        meshRenderer->setTransform(node.node.transform());
    }
}

void loadCamera(const tinygltf::Model& rawModel, const tinygltf::Node& rawNode, graph::SceneGraph& sg, std::string_view parentName) {
    const auto& rawCam = rawModel.cameras[rawNode.camera];
    auto& node = sg.addCamera(rawCam.name, parentName);
    auto& camNode = std::get<graph::CameraNode>(node.sceneNodeData);
    applyNodeTransform(rawNode, node);
    if (rawCam.type == "perspective") {
        const auto& cam = rawCam.perspective;
        camNode.camera = std::make_shared<scene::Camera>(
            scene::Frustum{
                static_cast<float>(cam.yfov),
                static_cast<float>(cam.aspectRatio),
                static_cast<float>(cam.znear),
                static_cast<float>(cam.zfar)},
            scene::Projection::PERSPECTIVE);

    } else if (rawCam.type == "orthographic") {
        const auto& cam = rawCam.orthographic;
        camNode.camera = std::make_shared<scene::Camera>(
            scene::Frustum{
                static_cast<float>(cam.xmag),
                static_cast<float>(cam.ymag),
                static_cast<float>(cam.znear),
                static_cast<float>(cam.zfar)},
            scene::Projection::ORTHOGRAPHIC);
    }
}

void loadLights(const tinygltf::Model& rawModel, uint32_t index, graph::SceneGraph& sg, std::string_view parentName) {
    const auto& rawLight = rawModel.lights[index];

    //    rawLight.
}

void loadEmpty(const tinygltf::Model& rawModel, uint32_t index, graph::SceneGraph& sg, std::string_view parentName) {
}

void loadScene(graph::SceneGraph& sg,
               const tinygltf::Model& rawModel,
               uint32_t index,
               rhi::CommandBufferPtr cmdBuffer,
               rhi::DevicePtr device) {
    raum_check(index < rawModel.scenes.size(), "incorrect index of scene");
    const auto& scene = rawModel.scenes[index];
    auto& root = sg.addEmpty(scene.name);

    std::vector<scene::Texture> textures;
    loadTextures(textures, rawModel, cmdBuffer, device);
    std::vector<scene::TechniquePtr> techniques;
    loadMaterials(techniques, textures, rawModel);

    std::function<void(uint32_t, uint32_t)> loadNodes;
    loadNodes = [&](uint32_t nodeIndex, uint32_t parent) {
        const auto& node = rawModel.nodes[nodeIndex];
        std::string_view parentName{};
        if (parent == -1) {
            parentName = scene.name;
        } else {
            parentName = rawModel.nodes[parent].name;
        }
        if (node.mesh != -1) {
            loadMesh(rawModel, node, sg, parentName, techniques, cmdBuffer, device);
        } else if (node.light != -1) {
            loadLights(rawModel, node.light, sg, parentName);
        } else if (node.camera != -1) {
            auto& camNode = sg.addCamera(node.name, parentName);
            loadCamera(rawModel, node, sg, parentName);
        } else {
            auto& emptyNode = sg.addEmpty(node.name, parentName);
            loadEmpty(rawModel, nodeIndex, sg, parentName);
        }
        for (auto child : node.children) {
            loadNodes(child, nodeIndex);
        }
    };

    for (const auto& node : scene.nodes) {
        loadNodes(node, -1);
    }
}

void defaultResourceTransition(rhi::CommandBufferPtr commandBuffer, rhi::DevicePtr device) {
    auto sampledImage = rhi::defaultSampledImage(device);
    rhi::ImageBarrierInfo transition{
        .image = sampledImage.get(),
        .dstStage = rhi::PipelineStage::VERTEX_SHADER,
        .newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        .dstAccessFlag = rhi::AccessFlags::SHADER_READ,
        .range = {
            .aspect = rhi::AspectMask::COLOR,
            .sliceCount = 1,
            .mipCount = 1,
        }};
    commandBuffer->appendImageBarrier(transition);

    auto storageImage = rhi::defaultStorageImage(device);
    transition.image = storageImage.get();
    transition.newLayout = rhi::ImageLayout::GENERAL;
    transition.dstAccessFlag = rhi::AccessFlags::SHADER_WRITE;
    transition.dstStage = rhi::PipelineStage::VERTEX_SHADER | rhi::PipelineStage::COMPUTE_SHADER;
    commandBuffer->appendImageBarrier(transition);
    commandBuffer->applyBarrier({});
}

void load(graph::SceneGraph& sg, const std::filesystem::path& filePath, rhi::DevicePtr device) {
    std::string err;
    std::string warn;
    tinygltf::Model rawModel;
    tinygltf::TinyGLTF loader;
    bool res = loader.LoadASCIIFromFile(&rawModel, &err, &warn, filePath.string());
    raum_check(res, "failed to load scene from {}, tinyGLTF: {}", filePath.string(), err);

    if constexpr (raum_debug) {
        if (!warn.empty()) {
            raum_warn("tinyGLTF: {}", warn);
        }
    }

    auto commandPool = rhi::CommandPoolPtr(device->createCoomandPool({}));
    auto commandBuffer = rhi::CommandBufferPtr(commandPool->makeCommandBuffer({}));
    auto* queue = device->getQueue({rhi::QueueType::GRAPHICS});
    commandBuffer->enqueue(queue);
    commandBuffer->begin({});

    loadScene(sg, rawModel, rawModel.defaultScene, commandBuffer, device);

    defaultResourceTransition(commandBuffer, device);
    commandBuffer->commit();
    queue->submit();
}

void load(graph::SceneGraph& sg, const std::filesystem::path& filePath, std::string_view sceneName, rhi::DevicePtr device) {
    std::string err;
    std::string warn;
    tinygltf::Model rawModel;
    tinygltf::TinyGLTF loader;
    bool res = loader.LoadASCIIFromFile(&rawModel, &err, &warn, filePath.string());
    raum_check(res, "failed to load scene from {}, tinyGLTF: {}", filePath.string(), err);

    if constexpr (raum_debug) {
        if (!warn.empty()) {
            raum_warn("tinyGLTF: {}", warn);
        }
    }

    auto commandPool = rhi::CommandPoolPtr(device->createCoomandPool({}));
    auto commandBuffer = rhi::CommandBufferPtr(commandPool->makeCommandBuffer({}));
    auto* queue = device->getQueue({rhi::QueueType::GRAPHICS});
    commandBuffer->enqueue(queue);
    commandBuffer->begin({});

    for (const auto& s : rawModel.scenes) {
        if (s.name == sceneName) {
            loadScene(sg, rawModel, &s - &rawModel.scenes[0], commandBuffer, device);
            break;
        }
    }

    defaultResourceTransition(commandBuffer, device);

    commandBuffer->commit();
    queue->submit();
}

} // namespace raum::asset::serialize