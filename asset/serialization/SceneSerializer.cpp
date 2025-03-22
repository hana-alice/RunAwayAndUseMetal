#include "SceneSerializer.h"
#include <numeric>
#include "BuiltinRes.h"
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
#include "core/utils/utils.h"
#include "tiny_gltf.h"

#include "Archive.h"

namespace cereal {
template <class Archive>
void serialize(Archive& ar, raum::rhi::VertexAttribute& vertexAttr) {
    ar(vertexAttr.location, vertexAttr.binding, vertexAttr.format, vertexAttr.offset);
}

template <class Archive>
void serialize(Archive& ar, raum::rhi::VertexBufferAttribute& attr) {
    ar(attr.binding, attr.stride, attr.rate);
}

template <class Archive>
void serialize(Archive& ar, raum::rhi::VertexLayout& rhiVertexLayout) {
    ar(rhiVertexLayout.vertexAttrs);
    ar(rhiVertexLayout.vertexBufferAttrs);
}
} // namespace cereal

namespace raum::asset::serialize {

void loadTexture(
    std::string_view name,
    uint32_t width,
    uint32_t height,
    const uint8_t* data,
    rhi::DevicePtr device,
    rhi::CommandBufferPtr cmdBuffer,
    std::vector<std::pair<std::string, scene::Texture>>& textures) {
    rhi::ImageInfo info{};
    info.extent = {width, height, 1};
    info.usage = rhi::ImageUsage::TRANSFER_DST | rhi::ImageUsage::SAMPLED | rhi::ImageUsage::TRANSFER_SRC;

    info.format = rhi::Format::RGBA8_UNORM;
    info.mipCount = std::floor(std::log2(std::min(width, height))) + 1;
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

    auto bufferSize = width * height * rhi::getFormatSize(info.format);
    rhi::BufferSourceInfo bufferInfo{
        .bufferUsage = rhi::BufferUsage::TRANSFER_SRC,
        .size = bufferSize,
        .data = data,
    };
    auto stagingBuffer = rhi::BufferPtr(device->createBuffer(bufferInfo));
    rhi::BufferImageCopyRegion region{
        .bufferSize = bufferSize,
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageAspect = rhi::AspectMask::COLOR,
        .imageExtent = {
            width,
            height,
            1,
        },
    };
    auto blitEncoder = rhi::BlitEncoderPtr(cmdBuffer->makeBlitEncoder());
    blitEncoder->copyBufferToImage(stagingBuffer.get(),
                                   img.get(),
                                   rhi::ImageLayout::TRANSFER_DST_OPTIMAL,
                                   &region,
                                   1);

    rhi::generateMipmaps(img, rhi::ImageLayout::TRANSFER_DST_OPTIMAL, cmdBuffer, device);

    // genmipmap turn image layout to TRANSFER_SRC_OPTIMAL
    barrierInfo.oldLayout = rhi::ImageLayout::TRANSFER_SRC_OPTIMAL;
    barrierInfo.newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    barrierInfo.srcAccessFlag = rhi::AccessFlags::TRANSFER_WRITE;
    barrierInfo.dstAccessFlag = rhi::AccessFlags::SHADER_READ;
    barrierInfo.srcStage = rhi::PipelineStage::TRANSFER;
    barrierInfo.dstStage = rhi::PipelineStage::VERTEX_SHADER;
    barrierInfo.range.mipCount = info.mipCount;
    cmdBuffer->appendImageBarrier(barrierInfo);
    cmdBuffer->applyBarrier({});

    rhi::ImageViewInfo viewInfo{};
    viewInfo.type = rhi::ImageViewType::IMAGE_VIEW_2D;
    viewInfo.image = img.get();
    viewInfo.format = info.format;
    viewInfo.range = {
        .aspect = rhi::AspectMask::COLOR,
        .firstSlice = 0,
        .sliceCount = info.sliceCount,
        .firstMip = 0,
        .mipCount = info.mipCount,
    };

    auto imgView = rhi::ImageViewPtr(device->createImageView(viewInfo));
    textures.emplace_back(name, scene::Texture{img, imgView});

    cmdBuffer->onComplete([stagingBuffer, img, imgView]() mutable {
        stagingBuffer.reset();
        img.reset();
        imgView.reset();
    });
}

void loadTextures(const std::filesystem::path& cachePath,
                  std::vector<std::pair<std::string, scene::Texture>>& textures,
                  const tinygltf::Model& rawModel,
                  rhi::CommandBufferPtr cmdBuffer,
                  rhi::DevicePtr device) {
    auto texCachePath = cachePath / "textures";
    const auto& mats = rawModel.materials;
    uint32_t count{0};
    for (const auto& res : rawModel.images) {
        if (!std::filesystem::exists(texCachePath)) {
            std::filesystem::create_directories(texCachePath);
        }

        std::string resName = res.name;
        if (resName.empty()) {
            resName = std::to_string(count++);
        }

        auto imgPath = texCachePath / resName;
        imgPath.replace_extension(".bin");
        OutputArchive ar(imgPath);
        ar << res.width;
        ar << res.height;
        ar << res.image;

        raum_check(res.bits == 8, "image bits not supported");
        raum_check(res.component == 4, "image channel count not supported");

        loadTexture(res.name, res.width, res.height, res.image.data(), device, cmdBuffer, textures);
    }
}

void loadMaterial(
    const std::filesystem::path& cachePath,
    const tinygltf::Model& rawModel,
    int32_t index,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    scene::MaterialTemplatePtr matTemplate,
    std::map<int32_t, scene::TechniquePtr>& techs,
    rhi::DevicePtr device) {
    const auto& rawTextures = rawModel.textures;
    const auto& res = rawModel.materials[index];

    if (res.pbrMetallicRoughness.baseColorTexture.index != -1) {
        matTemplate->addDefine("BASE_COLOR_MAP");
    }
    if (res.pbrMetallicRoughness.metallicRoughnessTexture.index != -1) {
        matTemplate->addDefine("MATALLIC_ROUGHNESS_MAP");
    }
    if (res.normalTexture.index != -1) {
        matTemplate->addDefine("NORMAL_MAP");
    }
    if (res.occlusionTexture.index != -1) {
        matTemplate->addDefine("OCCLUSION_MAP");
    }
    if (res.emissiveTexture.index != -1) {
        matTemplate->addDefine("EMISSIVE_MAP");
    }

    scene::MaterialPtr mat = matTemplate->instantiate(res.name, scene::MaterialType::PBR);
    auto pbrMat = std::static_pointer_cast<scene::PBRMaterial>(mat);
    auto tech = std::make_shared<scene::Technique>(mat, "default");
    techs.emplace(index, tech);
    auto& ds = tech->depthStencilInfo();
    ds.depthTestEnable = true;
    ds.depthWriteEnable = true;
    auto& bs = tech->blendInfo();
    bs.attachmentBlends.emplace_back();

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

    std::vector<float> mrno(12); // metallic, roughness, normalscale, occlusionscale

    const auto& pmr = res.pbrMetallicRoughness;
    const auto& bcf = pmr.baseColorFactor;

    const auto& bc = pmr.baseColorTexture;
    int bcSourceIndex{0};
    int bcuvIndex{-1};
    if (bc.index != -1) {
        auto imageIndex = rawTextures[bc.index].source;
        auto& baseColor = textures[imageIndex].second;
        baseColor.uvIndex = pmr.baseColorTexture.texCoord;
        pbrMat->set("albedoMap", baseColor);
        pbrMat->setBaseColorFactor(bcf[0], bcf[1], bcf[2], bcf[3]);
        mrno[0] = bcf[0];
        mrno[1] = bcf[1];
        mrno[2] = bcf[2];
        mrno[3] = bcf[3];

        bcSourceIndex = imageIndex;
        bcuvIndex = baseColor.uvIndex;
    }

    const auto& mr = pmr.metallicRoughnessTexture;
    int mrSourceIndex{0};
    int mruvIndex{-1};
    if (mr.index != -1) {
        auto imageIndex = rawTextures[mr.index].source;
        auto& metallicRoughness = textures[imageIndex].second;
        metallicRoughness.uvIndex = pmr.metallicRoughnessTexture.texCoord;
        pbrMat->set("metallicRoughnessMap", metallicRoughness);
        pbrMat->setMetallicFactor(pmr.metallicFactor);
        pbrMat->setRoughnessFactor(pmr.roughnessFactor);
        mrno[8] = pmr.metallicFactor;
        mrno[9] = pmr.roughnessFactor;

        mrSourceIndex = imageIndex;
        mruvIndex = metallicRoughness.uvIndex;
    }

    const auto& nt = res.normalTexture;
    int ntSourceIndex{0};
    int ntuIndex{-1};
    if (nt.index != -1) {
        auto imageIndex = rawTextures[nt.index].source;
        auto& normal = textures[imageIndex].second;
        normal.uvIndex = nt.texCoord;
        pbrMat->set("normalMap", normal);
        pbrMat->setNormalScale(nt.scale);
        mrno[10] = nt.scale;

        ntSourceIndex = imageIndex;
        ntuIndex = normal.uvIndex;
    }

    const auto& ot = res.occlusionTexture;
    int otSourceIndex{0};
    int otuvIndex{-1};
    if (ot.index != -1) {
        auto imageIndex = rawTextures[ot.index].source;
        auto& occlusion = textures[imageIndex].second;
        occlusion.uvIndex = ot.texCoord;
        pbrMat->set("aoMap", occlusion);
        pbrMat->setOcclusionStrength(ot.strength);
        mrno[11] = ot.strength;

        otSourceIndex = imageIndex;
        otuvIndex = occlusion.uvIndex;
    }

    const auto& et = res.emissiveTexture;
    int etSourceIndex{0};
    int etuvIndex{-1};
    if (et.index != -1) {
        auto imageIndex = rawTextures[et.index].source;
        auto& emissive = textures[imageIndex].second;
        emissive.uvIndex = et.texCoord;
        pbrMat->set("emissiveMap", emissive);
        mrno[4] = res.emissiveFactor[0];
        mrno[5] = res.emissiveFactor[1];
        mrno[6] = res.emissiveFactor[2];
        if (res.emissiveFactor.size() == 4) {
            mrno[7] = res.emissiveFactor[3];
        } else {
            mrno[7] = 1.0f;
        }
        etSourceIndex = imageIndex;
        etuvIndex = emissive.uvIndex;
    }

    rhi::BufferSourceInfo bufferInfo{
        .bufferUsage = rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST,
        .size = static_cast<uint32_t>(mrno.size() * sizeof(float)),
        .data = mrno.data(),
    };
    auto mrnoBuffer = rhi::BufferPtr(device->createBuffer(bufferInfo));
    pbrMat->set("PBRParams", scene::Buffer{mrnoBuffer});

    rhi::SamplerInfo linearInfo{
        .magFilter = rhi::Filter::LINEAR,
        .minFilter = rhi::Filter::LINEAR,
    };
    pbrMat->set("linearSampler", {linearInfo});
    pbrMat->set("pointSampler", {rhi::SamplerInfo{}});

    scene::Texture diffuseIrradiance{
        .texture = BuiltinRes::skybox().diffuseIrradianceImage(),
        .textureView = BuiltinRes::skybox().diffuseIrradianceView(),
    };
    pbrMat->set("diffuseEnvMap", diffuseIrradiance);

    scene::Texture prefilteredSpecular{
        .texture = BuiltinRes::skybox().prefilteredSpecularImage(),
        .textureView = BuiltinRes::skybox().prefilteredSpecularView(),
    };
    pbrMat->set("specularMap", prefilteredSpecular);

    scene::Texture brdfLUT{
        .texture = BuiltinRes::iblBrdfLUT(),
        .textureView = BuiltinRes::iblBrdfLUTView(),
    };
    pbrMat->set("brdfLUT", brdfLUT);

    auto matCachePath = cachePath / "material" / std::to_string(index);
    matCachePath.replace_extension(".mat");
    if (!std::filesystem::exists(matCachePath.parent_path())) {
        std::filesystem::create_directories(matCachePath.parent_path());
    }
    OutputArchive ar(matCachePath);
    ar << res.alphaMode;
    ar << res.doubleSided;
    ar << res.alphaCutoff;
    ar << res.pbrMetallicRoughness.baseColorTexture.index;
    ar << res.pbrMetallicRoughness.metallicRoughnessTexture.index;
    ar << res.normalTexture.index;
    ar << res.occlusionTexture.index;
    ar << res.emissiveTexture.index;
    ar << mrno;
    ar << bcSourceIndex;
    ar << bcuvIndex;
    ar << mrSourceIndex;
    ar << mruvIndex;
    ar << ntSourceIndex;
    ar << ntuIndex;
    ar << otSourceIndex;
    ar << otuvIndex;
    ar << etSourceIndex;
    ar << etuvIndex;
}

void applyNodeTransform(
    const std::vector<double>& scale,
    const std::vector<double>& rot,
    const std::vector<double>& trans,
    graph::SceneNode& node) {
    raum_check(scale.size() == 3, "scale size not 3");
    raum_check(rot.size() == 4, "rot size not 4");
    raum_check(trans.size() == 3, "trans size not 3");

    node.node.setScale({scale[0], scale[1], scale[2]});

    node.node.setTranslation({trans[0], trans[1], trans[2]});

    node.node.setOrientation({
        static_cast<float>(rot[3]),
        static_cast<float>(rot[0]),
        static_cast<float>(rot[1]),
        static_cast<float>(rot[2]),
    });

    node.node.update();
}

void loadMesh(
    const std::filesystem::path& cachePath,
    const tinygltf::Model& rawModel,
    uint32_t nodeIndex,
    graph::SceneGraph& sg,
    std::string_view parentName,
    std::map<int, scene::TechniquePtr>& techs,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    rhi::CommandBufferPtr cmdBuffer,
    rhi::DevicePtr device) {
    auto meshCachePath = cachePath / std::to_string(nodeIndex);

    const auto& rawNode = rawModel.nodes[nodeIndex];
    const auto& rawMesh = rawModel.meshes[rawNode.mesh];
    auto meshName = rawMesh.name;
    if (meshName.empty()) {
        meshName = std::to_string(nodeIndex);
    }

    auto& modelNode = sg.addModel(meshName, parentName);
    auto& sceneNode = sg.get(meshName);

    std::vector<double> scale{1.0, 1.0, 1.0};
    std::vector<double> rot{0.0, 0.0, 0.0, 1.0};
    std::vector<double> trans{0.0, 0.0, 0.0};
    if (!rawNode.scale.empty()) {
        scale[0] = rawNode.scale[0];
        scale[1] = rawNode.scale[1];
        scale[2] = rawNode.scale[2];
    }
    if (!rawNode.rotation.empty()) {
        rot = rawNode.rotation;
    }
    if (!rawNode.translation.empty()) {
        trans = rawNode.translation;
    }

    applyNodeTransform(scale, rot, trans, sceneNode);

    modelNode.model = std::make_shared<scene::Model>();
    auto& model = *modelNode.model;

    const auto& accessors = rawModel.accessors;
    const auto& rawBuffers = rawModel.buffers;
    const auto& rawBufferViews = rawModel.bufferViews;

    std::filesystem::path resPath = cachePath / "mesh" / meshName;
    resPath.make_preferred();
    resPath.replace_extension(".mesh");
    if (!std::filesystem::exists(resPath.parent_path())) {
        std::filesystem::create_directories(resPath.parent_path());
    }
    OutputArchive ar(resPath);
    ar << scale;
    ar << rot;
    ar << trans;
    ar << rawMesh.primitives.size();

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
            loadMaterial(cachePath, rawModel, localMatIndex, textures, matTemplate, techs, device);
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

        std::vector<char> indexData(indexBufferData, indexBufferData + indexBufferSource.size);
        // vertexbuffer data
        ar << meshData.vertexCount;
        ar << data;
        // indexbuffer data
        ar << meshData.indexCount;
        ar << meshData.indexBuffer.type;
        ar << indexData;

        ar << meshData.shaderAttrs;
        ar << meshData.vertexLayout;

        ar << localMatIndex;
        ar << prim.mode;
    }
}

void loadCamera(const tinygltf::Model& rawModel, const tinygltf::Node& rawNode, graph::SceneGraph& sg, std::string_view parentName) {
    const auto& rawCam = rawModel.cameras[rawNode.camera];
    auto& camNode = sg.addCamera(rawCam.name, parentName);
    auto& sceneNode = sg.get(rawCam.name);
    applyNodeTransform(rawNode.scale, rawNode.rotation, rawNode.translation, sceneNode);
    if (rawCam.type == "perspective") {
        const auto& cam = rawCam.perspective;
        camNode.camera = std::make_shared<scene::Camera>(
            scene::PerspectiveFrustum{
                static_cast<float>(cam.yfov),
                static_cast<float>(cam.aspectRatio),
                static_cast<float>(cam.znear),
                static_cast<float>(cam.zfar)});

    } else if (rawCam.type == "orthographic") {
        const auto& cam = rawCam.orthographic;
        camNode.camera = std::make_shared<scene::Camera>(
            scene::OrthoFrustum{
                static_cast<float>(cam.xmag),
                static_cast<float>(cam.ymag),
                static_cast<float>(cam.znear),
                static_cast<float>(cam.zfar)});
    }
}

void loadLights(const tinygltf::Model& rawModel, uint32_t index, graph::SceneGraph& sg, std::string_view parentName) {
    const auto& rawLight = rawModel.lights[index];

    //    rawLight.
}

void loadEmpty(uint32_t index, graph::SceneGraph& sg, std::string_view parentName) {
}

void loadScene(const std::filesystem::path& cachePath,
               graph::SceneGraph& sg,
               const tinygltf::Model& rawModel,
               uint32_t index,
               rhi::CommandBufferPtr cmdBuffer,
               rhi::DevicePtr device) {
    raum_check(index < rawModel.scenes.size(), "incorrect index of scene");
    const auto& scene = rawModel.scenes[index];
    auto& root = sg.addEmpty(scene.name);

    std::vector<std::pair<std::string, scene::Texture>> textures;
    loadTextures(cachePath, textures, rawModel, cmdBuffer, device);
    std::map<int32_t, scene::TechniquePtr> techniques;

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
            loadMesh(cachePath, rawModel, nodeIndex, sg, parentName, techniques, textures, cmdBuffer, device);
        } else if (node.light != -1) {
            loadLights(rawModel, node.light, sg, parentName);
        } else if (node.camera != -1) {
            auto& camNode = sg.addCamera(node.name, parentName);
            loadCamera(rawModel, node, sg, parentName);
        } else {
            auto& emptyNode = sg.addEmpty(node.name, parentName);
            loadEmpty(nodeIndex, sg, parentName);
        }
        for (auto child : node.children) {
            loadNodes(child, nodeIndex);
        }
    };

    for (const auto& node : scene.nodes) {
        loadNodes(node, -1);
    }
}

void loadFromFile(graph::SceneGraph& sg, const std::filesystem::path& filePath, rhi::DevicePtr device) {
    std::filesystem::path cachePath = raum::utils::resourceDirectory() / "cache" / filePath.stem();

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

    loadScene(cachePath, sg, rawModel, rawModel.defaultScene, commandBuffer, device);

    commandBuffer->commit();
    queue->submit(false);
}

void loadTexturesFromCache(
    const std::filesystem::path& cachePath,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    rhi::DevicePtr device,
    rhi::CommandBufferPtr cmdBuffer) {
    const auto texCachePath = cachePath / "textures";
    raum_check(std::filesystem::exists(texCachePath), "textures cache not found: %s", texCachePath.string());

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(texCachePath)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        files.emplace_back(entry.path());
    }

    std::ranges::sort(files, [](const std::filesystem::path& lhs, const std::filesystem::path rhs) {
        return std::stoi(lhs.filename().stem()) < std::stoi(rhs.filename().stem());
    });
    for (auto& entry : files) {
        std::string texName = entry.filename().string();
        std::string texIndex = texName.substr(0, texName.find_last_of('.'));
        InputArchive ar(entry);
        std::vector<uint8_t> imgData;
        uint32_t width{0}, height{0};
        ar >> width;
        ar >> height;
        ar >> imgData;

        loadTexture(texIndex, width, height, imgData.data(), device, cmdBuffer, textures);
    }
}

void loadLightsFromCache() {}

void loadCamerasFromCache() {}

void loadMaterialFromCache(
    std::filesystem::path cachePath,
    std::string_view modelName,
    int32_t matIndex,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    scene::MaterialTemplatePtr matTemplate,
    std::map<int32_t, scene::TechniquePtr>& techs,
    rhi::DevicePtr device) {
    auto matCachePath = cachePath / "material" / std::to_string(matIndex);
    matCachePath.replace_extension(".mat");
    InputArchive ar(matCachePath);

    std::string alphaMode{};
    bool doubleSided{false};
    double alphaCutoff{0.5f};
    ar >> alphaMode;
    ar >> doubleSided;
    ar >> alphaCutoff;

    int32_t baseColorIndex{-1}, metallicRoughnessIndex{-1}, normalIndex{-1}, occlusionIndex{-1}, emissiveIndex{-1};
    ar >> baseColorIndex;
    ar >> metallicRoughnessIndex;
    ar >> normalIndex;
    ar >> occlusionIndex;
    ar >> emissiveIndex;

    if (baseColorIndex != -1) {
        matTemplate->addDefine("BASE_COLOR_MAP");
    }
    if (metallicRoughnessIndex != -1) {
        matTemplate->addDefine("MATALLIC_ROUGHNESS_MAP");
    }
    if (normalIndex != -1) {
        matTemplate->addDefine("NORMAL_MAP");
    }
    if (occlusionIndex != -1) {
        matTemplate->addDefine("OCCLUSION_MAP");
    }
    if (emissiveIndex != -1) {
        matTemplate->addDefine("EMISSIVE_MAP");
    }

    std::string matName{modelName};
    matName.append("_");
    matName.append(std::to_string(matIndex));
    scene::MaterialPtr mat = matTemplate->instantiate(matName, scene::MaterialType::PBR);
    auto pbrMat = std::static_pointer_cast<scene::PBRMaterial>(mat);
    auto tech = std::make_shared<scene::Technique>(mat, "default");
    techs.emplace(matIndex, tech);
    auto& ds = tech->depthStencilInfo();
    ds.depthTestEnable = true;
    ds.depthWriteEnable = true;
    auto& bs = tech->blendInfo();
    bs.attachmentBlends.emplace_back();

    if (alphaMode == "OPAQUE") {
        // TODO: cmake introduce <wingdi.h> cause `OPAQUE` was preprocessed by macro.
        pbrMat->setAlphaMode(scene::PBRMaterial::AlphaMode::AM_OPAQUE);
    } else if (alphaMode == "MASK") {
        pbrMat->setAlphaMode(scene::PBRMaterial::AlphaMode::AM_MASK);
    } else if (alphaMode == "BLEND") {
        pbrMat->setAlphaMode(scene::PBRMaterial::AlphaMode::AM_BLEND);
    }
    pbrMat->setDoubleSided(doubleSided);
    pbrMat->setAlphaCutoff(alphaCutoff);

    std::vector<float> mrno; // metallic, roughness, normalscale, occlusionscale
    ar >> mrno;

    raum_check(mrno.size() == 12, "Unexpected components size!");

    int32_t bcSourceIndex{0};
    int32_t bcuvIndex{-1};
    ar >> bcSourceIndex;
    ar >> bcuvIndex;
    if (baseColorIndex != -1) {
        auto imageIndex = bcSourceIndex;
        auto& baseColor = textures[imageIndex].second;
        baseColor.uvIndex = bcuvIndex;
        pbrMat->set("albedoMap", baseColor);
        pbrMat->setBaseColorFactor(mrno[0], mrno[1], mrno[2], mrno[3]);
    }

    int metallicRoughnessSourceIndex{0};
    int mruvIndex{-1};
    ar >> metallicRoughnessSourceIndex;
    ar >> mruvIndex;
    if (metallicRoughnessIndex != -1) {
        auto imageIndex = metallicRoughnessSourceIndex;
        auto& metallicRoughness = textures[imageIndex].second;
        metallicRoughness.uvIndex = mruvIndex;
        pbrMat->set("metallicRoughnessMap", metallicRoughness);
        pbrMat->setMetallicFactor(mrno[8]);
        pbrMat->setRoughnessFactor(mrno[9]);
    }

    int normalSourceIndex{0};
    int normaluvIndex{-1};
    ar >> normalSourceIndex;
    ar >> normaluvIndex;
    if (normalIndex != -1) {
        auto imageIndex = normalSourceIndex;
        auto& normal = textures[imageIndex].second;
        normal.uvIndex = normaluvIndex;
        pbrMat->set("normalMap", normal);
        pbrMat->setNormalScale(mrno[10]);
    }

    int occlusionSourceIndex{0};
    int occlusionuvIndex{-1};
    ar >> occlusionSourceIndex;
    ar >> occlusionuvIndex;
    if (occlusionIndex != -1) {
        auto imageIndex = occlusionSourceIndex;
        auto& occlusion = textures[imageIndex].second;
        occlusion.uvIndex = occlusionuvIndex;
        pbrMat->set("aoMap", occlusion);
        pbrMat->setOcclusionStrength(mrno[11]);
    }

    int emissiveSourceIndex{0};
    int emissiveuvIndex{-1};
    ar >> emissiveSourceIndex;
    ar >> emissiveuvIndex;
    if (emissiveIndex != -1) {
        auto imageIndex = emissiveSourceIndex;
        auto& emissive = textures[imageIndex].second;
        emissive.uvIndex = emissiveuvIndex;
        pbrMat->set("emissiveMap", emissive);
        pbrMat->setEmissiveFactor(mrno[4], mrno[5], mrno[6]);
    }

    rhi::BufferSourceInfo bufferInfo{
        .bufferUsage = rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST,
        .size = static_cast<uint32_t>(mrno.size() * sizeof(float)),
        .data = mrno.data(),
    };
    auto mrnoBuffer = rhi::BufferPtr(device->createBuffer(bufferInfo));
    pbrMat->set("PBRParams", scene::Buffer{mrnoBuffer});

    rhi::SamplerInfo linearInfo{
        .magFilter = rhi::Filter::LINEAR,
        .minFilter = rhi::Filter::LINEAR,
    };
    pbrMat->set("linearSampler", {linearInfo});
    pbrMat->set("pointSampler", {rhi::SamplerInfo{}});

    scene::Texture diffuseIrradiance{
        .texture = BuiltinRes::skybox().diffuseIrradianceImage(),
        .textureView = BuiltinRes::skybox().diffuseIrradianceView(),
    };
    pbrMat->set("diffuseEnvMap", diffuseIrradiance);

    scene::Texture prefilteredSpecular{
        .texture = BuiltinRes::skybox().prefilteredSpecularImage(),
        .textureView = BuiltinRes::skybox().prefilteredSpecularView(),
    };
    pbrMat->set("specularMap", prefilteredSpecular);

    scene::Texture brdfLUT{
        .texture = BuiltinRes::iblBrdfLUT(),
        .textureView = BuiltinRes::iblBrdfLUTView(),
    };
    pbrMat->set("brdfLUT", brdfLUT);
}

void loadMeshFromCache(
    const std::filesystem::path& cachePath,
    std::string_view parentName,
    graph::SceneGraph& sg,
    std::map<int, scene::TechniquePtr>& techs,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    rhi::CommandBufferPtr cmdBuffer,
    rhi::DevicePtr device) {
    const auto& meshCachePath = cachePath / "mesh";
    raum_check(std::filesystem::exists(meshCachePath), "mesh cache not found: %s", meshCachePath.string());

    uint32_t count{0};
    for (const auto& entry : std::filesystem::directory_iterator(meshCachePath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto& meshName = entry.path().filename().string();
        std::filesystem::path resPath = cachePath / "mesh" / meshName;
        resPath.replace_extension(".mesh");
        InputArchive ar(resPath);

        if (!std::filesystem::exists(resPath.parent_path())) {
            std::filesystem::create_directories(resPath.parent_path());
        }

        std::vector<double> scale;
        std::vector<double> rot;
        std::vector<double> trans;

        ar >> scale;
        ar >> rot;
        ar >> trans;

        auto& modelNode = sg.addModel(meshName, parentName);
        auto& sceneNode = sg.get(meshName);

        applyNodeTransform(scale, rot, trans, sceneNode);

        modelNode.model = std::make_shared<scene::Model>();
        auto& model = *modelNode.model;

        size_t primCount{0};
        ar >> primCount;

        for (size_t i = 0; i < primCount; i++) {
            auto mesh = std::make_shared<scene::Mesh>();
            auto& meshData = mesh->meshData();
            const float* position{nullptr};
            const float* normal{nullptr};
            const float* uv{nullptr};
            const float* tangent{nullptr};
            const float* color{nullptr};
            bool color4{true};

            ar >> meshData.vertexCount;
            std::vector<float> data;
            ar >> data;
            rhi::BufferSourceInfo bufferSourceInfo{
                .bufferUsage = rhi::BufferUsage::VERTEX,
                .size = static_cast<uint32_t>(data.size() * sizeof(float)),
                .data = data.data(),
            };
            meshData.vertexBuffer.buffer = rhi::BufferPtr(device->createBuffer(bufferSourceInfo));

            ar >> meshData.indexCount;
            ar >> meshData.indexBuffer.type;
            std::vector<uint8_t> indexData;
            ar >> indexData;

            ar >> meshData.shaderAttrs;
            ar >> meshData.vertexLayout;

            rhi::BufferSourceInfo indexBufferSource{
                .bufferUsage = rhi::BufferUsage::INDEX,
            };
            std::vector<uint16_t> u16arr;
            switch (meshData.indexBuffer.type) {
                case rhi::IndexType::FULL: {
                    indexBufferSource.size = meshData.indexCount * sizeof(rhi::FullIndexType);
                    indexBufferSource.data = indexData.data();
                    break;
                }
                case rhi::IndexType::HALF: {
                    indexBufferSource.size = meshData.indexCount * sizeof(rhi::HalfIndexType);
                    indexBufferSource.data = indexData.data();
                    break;
                }
                default: {
                    raum_error("wrong index type: {}", static_cast<uint8_t>(meshData.indexBuffer.type));
                }
            }
            meshData.indexBuffer.buffer = rhi::BufferPtr(device->createBuffer(indexBufferSource));

            scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/gltfpbr");
            int32_t localMatIndex{0};
            ar >> localMatIndex;
            int primMode{0};
            ar >> primMode;

            if (!techs.contains(localMatIndex)) {
                loadMaterialFromCache(cachePath, cachePath.filename().string(), localMatIndex, textures, matTemplate, techs, device);
            }
            auto tech = techs.at(localMatIndex);
            if (primMode == TINYGLTF_MODE_TRIANGLES) {
                techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
            } else if (primMode == TINYGLTF_MODE_TRIANGLE_STRIP) {
                techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_STRIP);
            } else if (primMode == TINYGLTF_MODE_POINTS) {
                techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::POINT_LIST);
            } else if (primMode == TINYGLTF_MODE_LINE) {
                techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::LINE_LIST);
            } else if (primMode == TINYGLTF_MODE_LINE_STRIP) {
                techs[localMatIndex]->setPrimitiveType(rhi::PrimitiveType::LINE_STRIP);
            } else {
                raum_error("primitive:{} not supported.", primMode);
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

        ++count;
    }
}

// TODO: hierarchy
void loadSceneFromCache(const std::filesystem::path& cachePath,
                        graph::SceneGraph& sg,
                        rhi::CommandBufferPtr cmdBuffer,
                        rhi::DevicePtr device) {
    auto& root = sg.addEmpty("Scene");
    std::vector<std::pair<std::string, scene::Texture>> textures;
    loadTexturesFromCache(cachePath, textures, device, cmdBuffer);

    std::map<int32_t, scene::TechniquePtr> techniques;
    loadMeshFromCache(cachePath, "Scene", sg, techniques, textures, cmdBuffer, device);
}

void loadFromCache(graph::SceneGraph& sg, const std::filesystem::path& cachePath, rhi::DevicePtr device) {
    auto commandPool = rhi::CommandPoolPtr(device->createCoomandPool({}));
    auto commandBuffer = rhi::CommandBufferPtr(commandPool->makeCommandBuffer({}));
    auto* queue = device->getQueue({rhi::QueueType::GRAPHICS});
    commandBuffer->enqueue(queue);
    commandBuffer->begin({});

    loadSceneFromCache(cachePath, sg, commandBuffer, device);

    commandBuffer->commit();
    queue->submit(false);
}

void load(graph::SceneGraph& sg, const std::filesystem::path& filePath, rhi::DevicePtr device) {
    std::filesystem::path cachePath = raum::utils::resourceDirectory() / "cache" / filePath.stem();
    if (std::filesystem::exists(cachePath)) {
        loadFromCache(sg, cachePath, device);
    } else {
        loadFromFile(sg, filePath, device);
    }
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

    std::filesystem::path cachePath = raum::utils::resourceDirectory() / "cache" / filePath.filename();
    for (const auto& s : rawModel.scenes) {
        if (s.name == sceneName) {
            loadScene(cachePath, sg, rawModel, &s - &rawModel.scenes[0], commandBuffer, device);
            break;
        }
    }

    commandBuffer->commit();
    queue->submit(false);
}

} // namespace raum::asset::serialize