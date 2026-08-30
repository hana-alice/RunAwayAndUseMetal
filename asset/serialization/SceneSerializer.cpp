#include "SceneSerializer.h"
#include <cstring>
#include <functional>
#include <numeric>
#include <stdexcept>
#include "BuiltinRes.h"
#include "Mesh.h"
#include "PBRMaterial.h"
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIUtils.h"
#include "Technique.h"
#include "core/define.h"
#include "core/thread/execution.h"
#include "core/utils/containers.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "core/utils/utils.h"
#include "tiny_gltf.h"

#include "core/utils/Archive.h"
#include "RHIIO.h"
#include "SceneIO.h"
#include "GraphIO.h"

namespace raum::asset::serialize {

using OutputArchive = utils::OutputArchive;
using InputArchive = utils::InputArchive;

namespace {
constexpr uint32_t SceneCacheVersion{3};
constexpr std::string_view SceneCacheMetadata{".raum-cache"};

int64_t sourceTimestamp(const std::filesystem::path& filePath) {
    return std::filesystem::last_write_time(filePath).time_since_epoch().count();
}

bool cacheIsCurrent(const std::filesystem::path& cachePath, const std::filesystem::path& sourcePath) {
    const auto metadataPath = cachePath / SceneCacheMetadata;
    if (!std::filesystem::exists(metadataPath)) {
        return false;
    }

    InputArchive archive(metadataPath);
    uint32_t version{0};
    int64_t timestamp{0};
    archive >> version;
    archive >> timestamp;
    return version == SceneCacheVersion && timestamp == sourceTimestamp(sourcePath);
}

void writeCacheMetadata(const std::filesystem::path& cachePath, const std::filesystem::path& sourcePath) {
    std::filesystem::create_directories(cachePath);
    OutputArchive archive(cachePath / SceneCacheMetadata);
    archive << SceneCacheVersion;
    archive << sourceTimestamp(sourcePath);
}

Mat4 nodeLocalTransform(const tinygltf::Node& node) {
    if (node.matrix.size() == 16) {
        Mat4 transform{1.0f};
        for (uint32_t column = 0; column < 4; ++column) {
            for (uint32_t row = 0; row < 4; ++row) {
                transform[column][row] = static_cast<float>(node.matrix[column * 4 + row]);
            }
        }
        return transform;
    }

    Vec3f scale{1.0f};
    Vec3f translation{0.0f};
    Quaternion rotation{1.0f, 0.0f, 0.0f, 0.0f};
    if (node.scale.size() == 3) {
        scale = {node.scale[0], node.scale[1], node.scale[2]};
    }
    if (node.translation.size() == 3) {
        translation = {node.translation[0], node.translation[1], node.translation[2]};
    }
    if (node.rotation.size() == 4) {
        rotation = {
            static_cast<float>(node.rotation[3]),
            static_cast<float>(node.rotation[0]),
            static_cast<float>(node.rotation[1]),
            static_cast<float>(node.rotation[2]),
        };
    }
    return glm::translate(Mat4{1.0f}, translation) * Mat4{rotation} * glm::scale(Mat4{1.0f}, scale);
}

float readAccessorComponent(const uint8_t* data, int componentType, bool normalized) {
    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE: {
            int8_t value{};
            std::memcpy(&value, data, sizeof(value));
            if (!normalized) return static_cast<float>(value);
            const float result = static_cast<float>(value) / 127.0f;
            return result < -1.0f ? -1.0f : result;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            uint8_t value{};
            std::memcpy(&value, data, sizeof(value));
            return normalized ? static_cast<float>(value) / 255.0f : static_cast<float>(value);
        }
        case TINYGLTF_COMPONENT_TYPE_SHORT: {
            int16_t value{};
            std::memcpy(&value, data, sizeof(value));
            if (!normalized) return static_cast<float>(value);
            const float result = static_cast<float>(value) / 32767.0f;
            return result < -1.0f ? -1.0f : result;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            uint16_t value{};
            std::memcpy(&value, data, sizeof(value));
            return normalized ? static_cast<float>(value) / 65535.0f : static_cast<float>(value);
        }
        case TINYGLTF_COMPONENT_TYPE_INT: {
            int32_t value{};
            std::memcpy(&value, data, sizeof(value));
            if (!normalized) return static_cast<float>(value);
            const double result = static_cast<double>(value) / 2147483647.0;
            return static_cast<float>(result < -1.0 ? -1.0 : result);
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            uint32_t value{};
            std::memcpy(&value, data, sizeof(value));
            return normalized
                ? static_cast<float>(static_cast<double>(value) / 4294967295.0)
                : static_cast<float>(value);
        }
        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
            float value{};
            std::memcpy(&value, data, sizeof(value));
            return value;
        }
        case TINYGLTF_COMPONENT_TYPE_DOUBLE: {
            double value{};
            std::memcpy(&value, data, sizeof(value));
            return static_cast<float>(value);
        }
        default:
            throw std::runtime_error("Unsupported glTF vertex component type");
    }
}

const float* decodeVertexAccessor(const tinygltf::Model& model,
                                  int accessorIndex,
                                  uint32_t expectedComponents,
                                  std::vector<float>& decoded) {
    const auto& accessor = model.accessors.at(accessorIndex);
    if (accessor.sparse.isSparse || accessor.bufferView < 0) {
        throw std::runtime_error("Sparse glTF vertex accessors are not supported");
    }
    const auto& bufferView = model.bufferViews.at(accessor.bufferView);
    const auto& buffer = model.buffers.at(bufferView.buffer);
    const auto componentCount = tinygltf::GetNumComponentsInType(accessor.type);
    const auto componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    const auto byteStride = accessor.ByteStride(bufferView);
    if (componentCount != static_cast<int32_t>(expectedComponents) || componentSize <= 0 || byteStride <= 0) {
        throw std::runtime_error("Invalid glTF vertex accessor layout");
    }

    const auto dataOffset = bufferView.byteOffset + accessor.byteOffset;
    const auto packedElementSize = static_cast<size_t>(componentSize) * expectedComponents;
    const auto requiredSize = accessor.count
        ? dataOffset + (accessor.count - 1) * static_cast<size_t>(byteStride) + packedElementSize
        : dataOffset;
    if (requiredSize > buffer.data.size()) {
        throw std::runtime_error("glTF vertex accessor exceeds its source buffer");
    }

    decoded.resize(accessor.count * expectedComponents);
    const auto* source = buffer.data.data() + dataOffset;
    for (size_t vertex = 0; vertex < accessor.count; ++vertex) {
        const auto* element = source + vertex * byteStride;
        for (uint32_t component = 0; component < expectedComponents; ++component) {
            decoded[vertex * expectedComponents + component] = readAccessorComponent(
                element + component * componentSize, accessor.componentType, accessor.normalized);
        }
    }
    return decoded.data();
}
} // namespace

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

    // genmipmap turn image layout to TRANSFER_SRC_OPTIMAL, if mipcount > 1
    barrierInfo.oldLayout = info.mipCount > 1 ? rhi::ImageLayout::TRANSFER_SRC_OPTIMAL : rhi::ImageLayout::TRANSFER_DST_OPTIMAL;
    barrierInfo.newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    barrierInfo.srcAccessFlag = rhi::AccessFlags::TRANSFER_READ | rhi::AccessFlags::TRANSFER_WRITE;
    barrierInfo.dstAccessFlag = rhi::AccessFlags::SHADER_READ;
    barrierInfo.srcStage = rhi::PipelineStage::TRANSFER;
    barrierInfo.dstStage = rhi::PipelineStage::VERTEX_SHADER | rhi::PipelineStage::FRAGMENT_SHADER;
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
    std::string index{name};
    textures[std::stoi(index)] = {index, scene::Texture{img, imgView}};

    cmdBuffer->onComplete([stagingBuffer, img, imgView]() mutable {
        stagingBuffer.reset();
        img.reset();
        imgView.reset();
    });
}

void texturePreprocess(const std::filesystem::path& cachePath,
                       const tinygltf::Model& rawModel) {
    auto texCachePath = cachePath / "textures";
    const auto& mats = rawModel.materials;
    uint32_t count{0};
    for (const auto& res : rawModel.images) {
        if (!std::filesystem::exists(texCachePath)) {
            std::filesystem::create_directories(texCachePath);
        }

        std::string resName = std::to_string(count++);

        auto imgPath = texCachePath / resName;
        imgPath.replace_extension(".bin");
        OutputArchive ar(imgPath);
        ar << res.width;
        ar << res.height;
        ar << res.image;

        if (res.bits != 8 || res.component != 4) {
            throw std::runtime_error("Only 8-bit RGBA glTF images are supported");
        }
    }
}

void materialPreprocess(
    const std::filesystem::path& cachePath,
    const tinygltf::Model& rawModel,
    int32_t index) {
    const auto& rawTextures = rawModel.textures;
    const tinygltf::Material defaultMaterial{};
    if (index >= static_cast<int32_t>(rawModel.materials.size())) {
        throw std::runtime_error("glTF primitive refers to an invalid material");
    }
    const auto& res = index >= 0 ? rawModel.materials[index] : defaultMaterial;

    const auto& ef = res.emissiveFactor;
    raum_check(ef.size() == 3, "Unexpected emissive factor components!");

    std::vector<float> mrno(12); // metallic, roughness, normalscale, occlusionscale

    const auto& pmr = res.pbrMetallicRoughness;
    const auto& bcf = pmr.baseColorFactor;

    // glTF spec: all factors are always present regardless of texture existence
    mrno[0] = bcf[0];
    mrno[1] = bcf[1];
    mrno[2] = bcf[2];
    mrno[3] = bcf[3];

    mrno[4] = ef[0];
    mrno[5] = ef[1];
    mrno[6] = ef[2];
    mrno[7] = 1.0f;

    mrno[8]  = pmr.metallicFactor;
    mrno[9]  = pmr.roughnessFactor;
    mrno[10] = 1.0f; // normalScale default
    mrno[11] = 1.0f; // occlusionStrength default

    const auto& bc = pmr.baseColorTexture;
    int bcSourceIndex{0};
    int bcuvIndex{-1};
    if (bc.index != -1) {
        bcSourceIndex = rawTextures[bc.index].source;
        bcuvIndex = pmr.baseColorTexture.texCoord;
    }

    const auto& mr = pmr.metallicRoughnessTexture;
    int mrSourceIndex{0};
    int mruvIndex{-1};
    if (mr.index != -1) {
        mrSourceIndex = rawTextures[mr.index].source;
        mruvIndex = pmr.metallicRoughnessTexture.texCoord;
    }

    const auto& nt = res.normalTexture;
    int ntSourceIndex{0};
    int ntuIndex{-1};
    if (nt.index != -1) {
        mrno[10] = nt.scale;
        ntSourceIndex = rawTextures[nt.index].source;
        ntuIndex = nt.texCoord;
    }

    const auto& ot = res.occlusionTexture;
    int otSourceIndex{0};
    int otuvIndex{-1};
    if (ot.index != -1) {
        mrno[11] = ot.strength;
        otSourceIndex = rawTextures[ot.index].source;
        otuvIndex = ot.texCoord;
    }

    const auto& et = res.emissiveTexture;
    int etSourceIndex{0};
    int etuvIndex{-1};
    if (et.index != -1) {
        etSourceIndex = rawTextures[et.index].source;
        etuvIndex = et.texCoord;
    }

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

void applyNodeTransform(const Mat4& transform, graph::SceneNode& node) {
    node.node.setTransform(transform);
}

void meshPreprocess(
    const std::filesystem::path& cachePath,
    const tinygltf::Model& rawModel,
    uint32_t nodeIndex,
    graph::SceneGraph& sg,
    std::string_view parentName,
    const Mat4& worldTransform) {
    const auto& rawNode = rawModel.nodes[nodeIndex];
    const auto& rawMesh = rawModel.meshes[rawNode.mesh];
    auto meshName = std::to_string(nodeIndex);

    auto& modelNode = sg.addModel(meshName, parentName);
    auto& sceneNode = sg.get(meshName);

    applyNodeTransform(worldTransform, sceneNode);

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
    ar << worldTransform;
    ar << rawMesh.primitives.size();

    // TODO: Morph.
    for (const auto& prim : rawMesh.primitives) {
        auto mesh = std::make_shared<scene::Mesh>();

        auto& aabb = mesh->aabb();
        aabb.maxBound = Vec3f{-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
        aabb.minBound = Vec3f{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};

        auto& meshData = mesh->meshData();
        const float* position{nullptr};
        const float* normal{nullptr};
        const float* uv{nullptr};
        const float* tangent{nullptr};
        const float* color{nullptr};
        bool color4{true};
        std::vector<float> positionScratch;
        std::vector<float> normalScratch;
        std::vector<float> uvScratch;
        std::vector<float> tangentScratch;
        std::vector<float> colorScratch;
        for (const auto& [attrName, accessorIndex] : prim.attributes) {
            if (attrName == "POSITION") {
                const auto& accessor = accessors.at(accessorIndex);
                position = decodeVertexAccessor(rawModel, accessorIndex, 3, positionScratch);
                meshData.vertexCount = accessor.count;

                Vec3f localMin{std::numeric_limits<float>::max()};
                Vec3f localMax{std::numeric_limits<float>::lowest()};
                if (accessor.maxValues.size() >= 3 && accessor.minValues.size() >= 3) {
                    localMin = {accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]};
                    localMax = {accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]};
                } else {
                    for (size_t vertex = 0; vertex < accessor.count; ++vertex) {
                        const Vec3f point{position[vertex * 3], position[vertex * 3 + 1], position[vertex * 3 + 2]};
                        localMin = glm::min(localMin, point);
                        localMax = glm::max(localMax, point);
                    }
                }
                for (uint32_t corner = 0; corner < 8; ++corner) {
                    const Vec4f local{
                        (corner & 1) ? localMax.x : localMin.x,
                        (corner & 2) ? localMax.y : localMin.y,
                        (corner & 4) ? localMax.z : localMin.z,
                        1.0f,
                    };
                    const Vec4f transformed = sceneNode.node.transform() * local;
                    const Vec3f transformedPoint{transformed};
                    aabb.maxBound = glm::max(aabb.maxBound, transformedPoint);
                    aabb.minBound = glm::min(aabb.minBound, transformedPoint);
                }
            } else if (attrName == "NORMAL") {
                normal = decodeVertexAccessor(rawModel, accessorIndex, 3, normalScratch);
            } else if (attrName == "TEXCOORD_0") {
                uv = decodeVertexAccessor(rawModel, accessorIndex, 2, uvScratch);
            } else if (attrName == "TANGENT") {
                tangent = decodeVertexAccessor(rawModel, accessorIndex, 4, tangentScratch);
            } else if (attrName == "COLOR_0" || attrName == "COLOR") {
                color4 = accessors.at(accessorIndex).type == TINYGLTF_TYPE_VEC4;
                color = decodeVertexAccessor(rawModel, accessorIndex, color4 ? 4 : 3, colorScratch);
            } else {
                raum_warn("ignored vertex attribute: {}", attrName);
            }
        }

        if (!position || !meshData.vertexCount) {
            throw std::runtime_error("glTF primitive has no usable POSITION attribute");
        }
        const auto validateAttributeCount = [vertexCount = meshData.vertexCount](
                                                const std::vector<float>& values,
                                                uint32_t components) {
            if (!values.empty() && values.size() != static_cast<size_t>(vertexCount) * components) {
                throw std::runtime_error("glTF primitive vertex attributes have mismatched counts");
            }
        };
        validateAttributeCount(normalScratch, 3);
        validateAttributeCount(uvScratch, 2);
        validateAttributeCount(tangentScratch, 4);
        validateAttributeCount(colorScratch, color4 ? 4 : 3);

        std::vector<float> data;
        const auto eleNum = 3U + (normal ? 3U : 0U) + (uv ? 2U : 0U) +
                            (tangent ? 4U : 0U) + (color ? 4U : 0U);
        data.resize(eleNum * meshData.vertexCount);
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
                offset += 4;
            }
            if (color) {
                const uint32_t colorStride = color4 ? 4 : 3;
                data[i * eleNum + offset] = color[i * colorStride];
                data[i * eleNum + offset + 1] = color[i * colorStride + 1];
                data[i * eleNum + offset + 2] = color[i * colorStride + 2];
                data[i * eleNum + offset + 3] = color4 ? color[i * colorStride + 3] : 1.0f;
                offset += 4;
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
                rhi::Format::RG32_SFLOAT,
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

        auto& bufferAttribute = vertexLayout.vertexBufferAttrs.emplace_back();
        bufferAttribute.binding = 0;
        bufferAttribute.rate = rhi::InputRate::PER_VERTEX;
        bufferAttribute.stride = stride * static_cast<uint32_t>(sizeof(float));

        rhi::BufferSourceInfo indexBufferSource{
            .bufferUsage = rhi::BufferUsage::INDEX,
        };
        std::vector<uint16_t> u16arr;
        if (prim.indices >= 0) {
            const auto& indicesAccessor = accessors.at(prim.indices);
            if (indicesAccessor.sparse.isSparse || indicesAccessor.bufferView < 0 ||
                indicesAccessor.type != TINYGLTF_TYPE_SCALAR) {
                throw std::runtime_error("Invalid or sparse glTF index accessor");
            }
            meshData.indexCount = indicesAccessor.count;
            const auto& indexBufferView = rawBufferViews.at(indicesAccessor.bufferView);
            const auto& rawIndexBuffer = rawBuffers.at(indexBufferView.buffer);
            const auto sourceIndexSize = tinygltf::GetComponentSizeInBytes(indicesAccessor.componentType);
            const auto sourceOffset = indicesAccessor.byteOffset + indexBufferView.byteOffset;
            if (sourceIndexSize <= 0 ||
                sourceOffset + meshData.indexCount * static_cast<size_t>(sourceIndexSize) > rawIndexBuffer.data.size()) {
                throw std::runtime_error("glTF index accessor exceeds its source buffer");
            }
            const auto* indexBufferData = rawIndexBuffer.data.data() + sourceOffset;
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
                default:
                    throw std::runtime_error("Unsupported glTF index component type");
            }
        } else {
            meshData.indexCount = 0;
            meshData.indexBuffer.type = rhi::IndexType::FULL;
        }

        std::vector<char> indexData;
        if (indexBufferSource.size) {
            const auto* serializedIndexData = static_cast<const char*>(indexBufferSource.data);
            indexData.assign(serializedIndexData, serializedIndexData + indexBufferSource.size);
        }

        auto localMatIndex = prim.material;
        materialPreprocess(cachePath, rawModel, localMatIndex);

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
        ar << (prim.mode < 0 ? TINYGLTF_MODE_TRIANGLES : prim.mode);
        ar << aabb;
    }
}

void loadCamera(const tinygltf::Model& rawModel,
                const tinygltf::Node& rawNode,
                graph::SceneGraph& sg,
                std::string_view nodeName,
                std::string_view parentName,
                const Mat4& worldTransform) {
    const auto& rawCam = rawModel.cameras[rawNode.camera];
    auto& camNode = sg.addCamera(nodeName, parentName);
    auto& sceneNode = sg.get(nodeName);
    applyNodeTransform(worldTransform, sceneNode);
    if (rawCam.type == "perspective") {
        const auto& cam = rawCam.perspective;
        camNode.camera = std::make_shared<scene::Camera>(
            scene::PerspectiveFrustum{
                utils::Degree{glm::degrees(static_cast<float>(cam.yfov))},
                cam.aspectRatio > 0.0 ? static_cast<float>(cam.aspectRatio) : 1.0f,
                static_cast<float>(cam.znear),
                cam.zfar > 0.0 ? static_cast<float>(cam.zfar) : 1000.0f});

    } else if (rawCam.type == "orthographic") {
        const auto& cam = rawCam.orthographic;
        camNode.camera = std::make_shared<scene::Camera>(
            scene::OrthoFrustum{
                -static_cast<float>(cam.xmag),
                static_cast<float>(cam.xmag),
                -static_cast<float>(cam.ymag),
                static_cast<float>(cam.ymag),
                static_cast<float>(cam.znear),
                static_cast<float>(cam.zfar)});
    }
    if (camNode.camera) {
        Mat3 rotation{1.0f};
        for (uint32_t column = 0; column < 3; ++column) {
            rotation[column] = glm::normalize(Vec3f{worldTransform[column]});
        }
        const auto gltfCameraCorrection = glm::angleAxis(glm::pi<float>(), Vec3f{0.0f, 1.0f, 0.0f});
        auto& eye = camNode.camera->eye();
        eye.setPosition(Vec3f{worldTransform[3]});
        eye.setOrientation(glm::normalize(glm::quat_cast(rotation) * gltfCameraCorrection));
        camNode.camera->update();
    }
}

void loadLights(const tinygltf::Model& rawModel,
                uint32_t index,
                graph::SceneGraph& sg,
                std::string_view nodeName,
                std::string_view parentName,
                const Mat4& worldTransform) {
    const auto& rawLight = rawModel.lights[index];
    auto& lightNode = sg.addLight(nodeName, parentName);
    auto& sceneNode = sg.get(nodeName);
    applyNodeTransform(worldTransform, sceneNode);
    // TODO: map tinygltf::Light parameters to scene::Light.
}

void loadEmpty(std::string_view nodeName, const Mat4& worldTransform, graph::SceneGraph& sg) {
    auto& sceneNode = sg.get(nodeName);
    applyNodeTransform(worldTransform, sceneNode);
}

void scenePreprocess(const std::filesystem::path& cachePath,
                     graph::SceneGraph& sg,
                     const tinygltf::Model& rawModel,
                     uint32_t index) {
    raum_check(index < rawModel.scenes.size(), "incorrect index of scene");
    const auto& scene = rawModel.scenes[index];
    constexpr std::string_view rootName{"Scene"};
    auto& root = sg.addEmpty(rootName);

    texturePreprocess(cachePath, rawModel);
    std::function<void(uint32_t, int32_t, const Mat4&)> loadNodes;
    loadNodes = [&](uint32_t nodeIndex, int32_t parent, const Mat4& parentTransform) {
        const auto& node = rawModel.nodes[nodeIndex];
        const auto nodeName = std::to_string(nodeIndex);
        std::string parentName;
        if (parent == -1) {
            parentName = rootName;
        } else {
            parentName = std::to_string(parent);
        }
        const Mat4 worldTransform = parentTransform * nodeLocalTransform(node);
        if (node.mesh != -1) {
            meshPreprocess(cachePath, rawModel, nodeIndex, sg, parentName, worldTransform);
        } else if (node.light != -1) {
            loadLights(rawModel, node.light, sg, nodeName, parentName, worldTransform);
        } else if (node.camera != -1) {
            loadCamera(rawModel, node, sg, nodeName, parentName, worldTransform);
        } else {
            sg.addEmpty(nodeName, parentName);
            loadEmpty(nodeName, worldTransform, sg);
        }
        for (auto child : node.children) {
            loadNodes(child, static_cast<int32_t>(nodeIndex), worldTransform);
        }
    };

    for (const auto& node : scene.nodes) {
        loadNodes(node, -1, Mat4{1.0f});
    }
}

void assetPreprocess(graph::SceneGraph& sg, const std::filesystem::path& filePath) {
    std::filesystem::path cachePath = raum::utils::resourceDirectory() / "cache" / filePath.stem();

    std::string err;
    std::string warn;
    tinygltf::Model rawModel;
    tinygltf::TinyGLTF loader;
    bool res = loader.LoadASCIIFromFile(&rawModel, &err, &warn, filePath.string());
    raum_check(res, "failed to load scene from {}, tinyGLTF: {}", filePath.string(), err);
    if (!res) {
        throw std::runtime_error("Failed to load glTF scene '" + filePath.string() + "': " + err);
    }

    if constexpr (raum_debug) {
        if (!warn.empty()) {
            raum_warn("tinyGLTF: {}", warn);
        }
    }
    if (rawModel.scenes.empty()) {
        throw std::runtime_error("glTF contains no scenes");
    }
    const auto sceneIndex = rawModel.defaultScene >= 0 ? rawModel.defaultScene : 0;
    scenePreprocess(cachePath, sg, rawModel, sceneIndex);
    writeCacheMetadata(cachePath, filePath);
}

void loadTexturesFromCache(
    const std::filesystem::path& cachePath,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    rhi::DevicePtr device,
    rhi::CommandBufferPtr cmdBuffer) {
    const auto texCachePath = cachePath / "textures";
    // raum_check(std::filesystem::exists(texCachePath), "textures cache not found: %s", texCachePath.string());
    if (std::filesystem::exists(texCachePath)) {
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

        textures.resize(files.size());

        synchronized_pool_resource pool{std::pmr::get_default_resource()};
        auto& threadPool = getIOThreadPool();
        auto sched = threadPool.get_scheduler();

        std::mutex taskMutex;

        auto imgTask = [&](int i) {
            // load image data
            auto& entry = files[i];
            std::string texName = entry.filename().string();
            std::string texIndex = texName.substr(0, texName.find_last_of('.'));
            InputArchive ar(entry);
            PmrVector<uint8_t> imgData{&pool};
            uint32_t width{0}, height{0};
            ar >> width;
            ar >> height;
            ar >> imgData;

            std::lock_guard<std::mutex> lock(taskMutex);
            loadTexture(texIndex, width, height, imgData.data(), device, cmdBuffer, textures);
        };

        auto sender = stdexec::schedule(sched) | stdexec::bulk(files.size(), std::move(imgTask));
        stdexec::sync_wait(std::move(sender));
    }
}

void loadLightsFromCache() {}

void loadCamerasFromCache() {}

scene::TechniquePtr loadMaterialFromCache(
    std::filesystem::path cachePath,
    std::string_view modelName,
    int32_t matIndex,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    scene::MaterialTemplatePtr matTemplate,
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
        auto& colorBlend = bs.attachmentBlends.back();
        colorBlend.blendEnable = true;
        colorBlend.srcColorBlendFactor = rhi::BlendFactor::SRC_ALPHA;
        colorBlend.dstColorBlendFactor = rhi::BlendFactor::ONE_MINUS_SRC_ALPHA;
        colorBlend.srcAlphaBlendFactor = rhi::BlendFactor::ONE;
        colorBlend.dstAlphaBlendFactor = rhi::BlendFactor::ZERO;
    }
    pbrMat->setDoubleSided(doubleSided);
    pbrMat->setAlphaCutoff(alphaCutoff);

    std::vector<float> mrno; // metallic, roughness, normalscale, occlusionscale
    ar >> mrno;

    if (mrno.size() != 12) {
        throw std::runtime_error("Cached glTF material parameters have an unexpected size");
    }
    pbrMat->setBaseColorFactor(mrno[0], mrno[1], mrno[2], mrno[3]);
    pbrMat->setEmissiveFactor(mrno[4], mrno[5], mrno[6]);
    pbrMat->setMetallicFactor(mrno[8]);
    pbrMat->setRoughnessFactor(mrno[9]);
    pbrMat->setNormalScale(mrno[10]);
    pbrMat->setOcclusionStrength(mrno[11]);

    int32_t bcSourceIndex{0};
    int32_t bcuvIndex{-1};
    ar >> bcSourceIndex;
    ar >> bcuvIndex;
    if (baseColorIndex != -1) {
        auto imageIndex = bcSourceIndex;
        auto& baseColor = textures[imageIndex].second;
        baseColor.uvIndex = bcuvIndex;
        pbrMat->set("albedoMap", baseColor);
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
        .maxLod = 16.0f,
    };
    pbrMat->set("linearSampler", {linearInfo});
    pbrMat->set("pointSampler", {rhi::SamplerInfo{.maxLod = 16.0f}});

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
    return tech;
}

void loadMeshFromCache(
    const std::filesystem::path& cachePath,
    std::string_view parentName,
    graph::SceneGraph& sg,
    std::vector<std::pair<std::string, scene::Texture>>& textures,
    rhi::CommandBufferPtr cmdBuffer,
    rhi::DevicePtr device) {
    const auto& meshCachePath = cachePath / "mesh";
    if (!std::filesystem::exists(meshCachePath)) {
        throw std::runtime_error("glTF mesh cache was not found: " + meshCachePath.string());
    }

    for (const auto& entry : std::filesystem::directory_iterator(meshCachePath)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".mesh") {
            continue;
        }

        const auto meshName = entry.path().stem().string();
        std::filesystem::path resPath = cachePath / "mesh" / meshName;
        resPath.replace_extension(".mesh");
        InputArchive ar(resPath);

        if (!std::filesystem::exists(resPath.parent_path())) {
            std::filesystem::create_directories(resPath.parent_path());
        }

        Mat4 worldTransform{1.0f};
        ar >> worldTransform;

        auto& modelNode = sg.addModel(meshName, parentName);
        auto& sceneNode = sg.get(meshName);

        applyNodeTransform(worldTransform, sceneNode);

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
            if (meshData.indexCount) {
                const auto expectedIndexDataSize = indexBufferSource.size;
                if (indexData.size() != expectedIndexDataSize) {
                    throw std::runtime_error("Cached glTF index data has an unexpected size");
                }
                meshData.indexBuffer.buffer = rhi::BufferPtr(device->createBuffer(indexBufferSource));
            }

            scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/gltfpbr");
            if (test(meshData.shaderAttrs, scene::ShaderAttribute::NORMAL)) {
                matTemplate->addDefine("VERTEX_NORMAL");
            }
            if (test(meshData.shaderAttrs, scene::ShaderAttribute::UV)) {
                matTemplate->addDefine("VERTEX_UV");
            }
            if (test(meshData.shaderAttrs, scene::ShaderAttribute::TANGENT)) {
                matTemplate->addDefine("VERTEX_TANGENT");
            }
            if (test(meshData.shaderAttrs, scene::ShaderAttribute::COLOR)) {
                matTemplate->addDefine("VERTEX_COLOR");
            }
            int32_t localMatIndex{0};
            ar >> localMatIndex;
            int primMode{0};
            ar >> primMode;
            ar >> mesh->aabb();

            auto tech = loadMaterialFromCache(
                cachePath, cachePath.filename().string(), localMatIndex, textures, matTemplate, device);
            if (primMode == TINYGLTF_MODE_TRIANGLES) {
                tech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
            } else if (primMode == TINYGLTF_MODE_TRIANGLE_STRIP) {
                tech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_STRIP);
            } else if (primMode == TINYGLTF_MODE_POINTS) {
                tech->setPrimitiveType(rhi::PrimitiveType::POINT_LIST);
            } else if (primMode == TINYGLTF_MODE_LINE) {
                tech->setPrimitiveType(rhi::PrimitiveType::LINE_LIST);
            } else if (primMode == TINYGLTF_MODE_LINE_STRIP) {
                tech->setPrimitiveType(rhi::PrimitiveType::LINE_STRIP);
            } else {
                throw std::runtime_error("Unsupported glTF primitive mode: " + std::to_string(primMode));
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
}

// TODO: hierarchy
void loadSceneFromCache(const std::filesystem::path& cachePath,
                        graph::SceneGraph& sg,
                        rhi::CommandBufferPtr cmdBuffer,
                        rhi::DevicePtr device) {
    auto& root = sg.addEmpty("Scene");
    std::vector<std::pair<std::string, scene::Texture>> textures;
    loadTexturesFromCache(cachePath, textures, device, cmdBuffer);

    loadMeshFromCache(cachePath, "Scene", sg, textures, cmdBuffer, device);
}

void loadFromCache(graph::SceneGraph& sg, const std::filesystem::path& cachePath, rhi::DevicePtr device) {
    const auto graphicsQueueIndex = device->getQueue({rhi::QueueType::GRAPHICS})->index();
    auto commandPool = rhi::CommandPoolPtr(device->createCoomandPool({graphicsQueueIndex}));
    auto commandBuffer = rhi::CommandBufferPtr(commandPool->makeCommandBuffer({}));
    auto* queue = device->getQueue({rhi::QueueType::GRAPHICS});
    commandBuffer->enqueue(queue);
    commandBuffer->begin({});

    loadSceneFromCache(cachePath, sg, commandBuffer, device);

    commandBuffer->commit();

    commandBuffer->onComplete([commandBuffer, commandPool]() mutable {
        commandBuffer.reset();
        commandPool.reset();
    });
    queue->submit(false);
}

void load(graph::SceneGraph& sg, const std::filesystem::path& filePath, rhi::DevicePtr device) {
    std::filesystem::path cachePath = raum::utils::resourceDirectory() / "cache" / filePath.stem();
    if (!cacheIsCurrent(cachePath, filePath)) {
        if (std::filesystem::exists(cachePath)) {
            std::filesystem::remove_all(cachePath);
        }
        graph::SceneGraph offlineSg;
        assetPreprocess(offlineSg, filePath);
    }
    raum_expect(std::filesystem::exists(cachePath), "Failed to store cache file");
    loadFromCache(sg, cachePath, device);
}

void load(graph::SceneGraph& sg, const std::filesystem::path& filePath, std::string_view sceneName, rhi::DevicePtr device) {
    const auto cacheKey = filePath.stem().string() + "_" + std::to_string(std::hash<std::string_view>{}(sceneName));
    const auto cachePath = raum::utils::resourceDirectory() / "cache" / cacheKey;
    if (cacheIsCurrent(cachePath, filePath)) {
        loadFromCache(sg, cachePath, device);
        return;
    }

    std::string err;
    std::string warn;
    tinygltf::Model rawModel;
    tinygltf::TinyGLTF loader;
    bool res = loader.LoadASCIIFromFile(&rawModel, &err, &warn, filePath.string());
    raum_check(res, "failed to load scene from {}, tinyGLTF: {}", filePath.string(), err);
    if (!res) {
        throw std::runtime_error("Failed to load glTF scene '" + filePath.string() + "': " + err);
    }

    if constexpr (raum_debug) {
        if (!warn.empty()) {
            raum_warn("tinyGLTF: {}", warn);
        }
    }

    if (std::filesystem::exists(cachePath)) {
        std::filesystem::remove_all(cachePath);
    }
    graph::SceneGraph offlineScene;
    bool found{false};
    for (const auto& s : rawModel.scenes) {
        if (s.name == sceneName) {
            scenePreprocess(cachePath, offlineScene, rawModel, &s - &rawModel.scenes[0]);
            found = true;
            break;
        }
    }
    if (!found) {
        throw std::runtime_error("glTF scene was not found: " + std::string{sceneName});
    }
    writeCacheMetadata(cachePath, filePath);
    loadFromCache(sg, cachePath, device);
}

} // namespace raum::asset::serialize
