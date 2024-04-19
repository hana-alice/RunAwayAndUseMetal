#include "SceneLoader.h"
#include <numeric>
#include "ImageLoader.h"
#include "Mesh.h"
#include "PBRMaterial.h"
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIUtils.h"
#include "Technique.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "core/define.h"

namespace raum::asset {

namespace {

void expand(scene::AABB& aabb, const aiAABB& src) {
    aabb.minBound.x = std::min(aabb.minBound.x, src.mMin.x);
    aabb.minBound.y = std::min(aabb.minBound.y, src.mMin.y);
    aabb.minBound.z = std::min(aabb.minBound.z, src.mMin.z);
    aabb.maxBound.x = std::max(aabb.maxBound.x, src.mMax.x);
    aabb.maxBound.y = std::max(aabb.maxBound.y, src.mMax.y);
    aabb.maxBound.z = std::max(aabb.maxBound.z, src.mMax.z);
}

void loadMesh(const aiScene* scene,
              const aiNode* node,
              scene::Model& model,
              const std::vector<scene::TechniquePtr>& techs,
              rhi::DevicePtr device) {
    auto& aabb = model.aabb();
    // auto& meshes = model.meshes();
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        auto newMesh = std::make_shared<scene::Mesh>();
        uint32_t location{0};
        const auto* mesh = scene->mMeshes[node->mMeshes[i]];
        expand(aabb, mesh->mAABB);
        auto localMatIndex = mesh->mMaterialIndex;

        auto& meshData = newMesh->meshData();
        meshData.vertexCount = mesh->mNumVertices;
        auto& vertexLayout = meshData.vertexLayout;

        std::vector<float> rawData;
        auto& meshVert = rawData;
        uint32_t stride{0};
        // pos
        vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
            location++,
            0,
            rhi::Format::RGB32_SFLOAT,
            stride * static_cast<uint32_t>(sizeof(float)),
        });
        stride += 3;

        meshData.shaderAttrs = scene::ShaderAttribute::POSITION;

        const aiVector3D* normal{nullptr};
        if (mesh->HasNormals()) {
            normal = mesh->mNormals;
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            meshData.shaderAttrs |= scene::ShaderAttribute::NORMAL;
            stride += 3;
        }
        if (mesh->HasTextureCoords(0)) {
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RG32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 2;
            meshData.shaderAttrs |= scene::ShaderAttribute::UV;
        }
        if (mesh->HasTangentsAndBitangents()) {
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 3 + 3;
            meshData.shaderAttrs |= scene::ShaderAttribute::BI_TANGENT;
        }
        const aiColor4D* color{nullptr};
        if (mesh->HasVertexColors(0)) {
            color = mesh->mColors[0];
            vertexLayout.vertexAttrs.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGBA32_SFLOAT,
                stride * static_cast<uint32_t>(sizeof(float)),
            });
            stride += 4;
        }
        meshVert.resize(stride * mesh->mNumVertices);

        for (size_t j = 0; j < mesh->mNumVertices; ++j) {
            uint32_t accStride{3};
            auto* vertData = meshVert.data() + j * stride;

            const auto& vertex = mesh->mVertices[j];
            vertData[0] = vertex.x;
            vertData[1] = vertex.y;
            vertData[2] = vertex.z;

            if (normal) {
                auto* normalData = vertData + accStride;
                normalData[0] = normal[j].x;
                normalData[1] = normal[j].y;
                normalData[2] = normal[j].z;
                accStride += 3;
            }

            if (mesh->mTextureCoords[0]) {
                const auto& uv = mesh->mTextureCoords[0][j];
                auto* uvData = vertData + accStride;
                uvData[0] = uv.x;
                uvData[1] = uv.y;
                accStride += 2;
            }

            if (mesh->HasTangentsAndBitangents()) {
                const auto& tangent = mesh->mTangents[j];
                auto* tangentData = vertData + accStride;
                tangentData[0] = tangent.x;
                tangentData[1] = tangent.y;
                tangentData[2] = tangent.z;
                accStride += 3;

                const auto& bitangent = mesh->mBitangents[j];
                auto* bitangentData = vertData + accStride;
                bitangentData[0] = bitangent.x;
                bitangentData[1] = bitangent.y;
                bitangentData[2] = bitangent.z;
                accStride += 3;
            }
        }

        uint32_t indexNum{0};
        if (mesh->HasFaces()) {
            std::vector<uint8_t> indices;
            indexNum = std::accumulate(mesh->mFaces, mesh->mFaces + mesh->mNumFaces, indexNum, [](uint32_t curr, const aiFace& face) {
                return curr + face.mNumIndices;
            });

            if (indexNum >= std::numeric_limits<rhi::HalfIndexType>::max()) {
                meshData.indexBuffer.type = rhi::IndexType::FULL;
                indices.resize(indexNum * 4);
            } else {
                meshData.indexBuffer.type = rhi::IndexType::HALF;
                indices.resize(indexNum * 2);
            }

            meshData.indexCount = indexNum;
            auto* indexData = indices.data();

            uint32_t count{0};
            for (size_t j = 0; j < mesh->mNumFaces; ++j) {
                const auto& face = mesh->mFaces[j];
                for (size_t k = 0; k < face.mNumIndices; ++k) {
                    if (meshData.indexBuffer.type == rhi::IndexType::FULL) {
                        reinterpret_cast<rhi::FullIndexType*>(indexData)[count++] = face.mIndices[k];
                    } else {
                        reinterpret_cast<rhi::HalfIndexType*>(indexData)[count++] = face.mIndices[k];
                    }
                }
            }

            rhi::BufferSourceInfo bufferSourceInfo{
                .bufferUsage = rhi::BufferUsage::INDEX,
                .size = static_cast<uint32_t>(indices.size()),
                .data = indices.data(),
            };
            meshData.indexBuffer.buffer = device->createBuffer(bufferSourceInfo);
        }
        auto& bufferAttribute = vertexLayout.vertexBufferAttrs.emplace_back();
        bufferAttribute.binding = 0;
        bufferAttribute.rate = rhi::InputRate::PER_VERTEX;
        bufferAttribute.stride = stride * static_cast<uint32_t>(sizeof(float));

        rhi::BufferSourceInfo bufferSourceInfo{
            .bufferUsage = rhi::BufferUsage::VERTEX,
            .size = static_cast<uint32_t>(meshVert.size() * sizeof(float)),
            .data = meshVert.data(),
        };

        meshData.vertexBuffer.buffer = device->createBuffer(bufferSourceInfo);

        auto meshRenderer = model.meshRenderers().emplace_back(std::make_shared<scene::MeshRenderer>(newMesh));
        meshRenderer->addTechnique(techs[localMatIndex]);
        meshRenderer->setVertexInfo(0, mesh->mNumVertices, indexNum);
    }

    for (size_t i = 0; i < node->mNumChildren; ++i) {
        loadMesh(scene, node->mChildren[i], model, techs, device);
    }
}

void loadMaterial(const aiScene* scene,
                  scene::ModelPtr model,
                  std::vector<scene::TechniquePtr>& techs,
                  std::filesystem::path file,
                  rhi::CommandBufferPtr cmdBuffer,
                  rhi::DevicePtr& device) {
    auto blitEncoder = rhi::BlitEncoderPtr(cmdBuffer->makeBlitEncoder());
    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        const auto* material = scene->mMaterials[i];
        scene::MaterialTemplatePtr matTemplate = scene::getOrCreateMaterialTemplate("asset/layout/simple");
        scene::MaterialPtr mat = matTemplate->instantiate(scene::MaterialType::PBR);
        auto pbrMat = std::static_pointer_cast<scene::PBRMaterial>(mat);
        auto& tech = techs.emplace_back(std::make_shared<scene::Technique>(mat, "default"));
        auto& ds = tech->depthStencilInfo();
        ds.depthTestEnable = true;
        ds.depthWriteEnable = true;
        auto& rs = tech->rasterizationInfo();
        rs.cullMode = rhi::FaceMode::BACK;
        tech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);

        auto entry = file.parent_path();
        aiString texturePath;
        constexpr uint32_t enumOffset = 1;
        static_assert(static_cast<uint32_t>(scene::TextureType::AMBIENT_OCCLUSION) == (aiTextureType_AMBIENT_OCCLUSION - 1));
        for (size_t i = 0; i < static_cast<uint32_t>(aiTextureType_AMBIENT_OCCLUSION); ++i) {
            auto texType = static_cast<aiTextureType>(aiTextureType_DIFFUSE + i);
            if (material->GetTextureCount(texType) > 0) {
                if (material->GetTexture(texType, 0, &texturePath, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
                    std::string p(texturePath.C_Str());
                    if (p.substr(0, 2) == ".\\") {
                        p = p.substr(2, p.size() - 2);
                    }
                    ImageLoader imgLoader;
                    auto imgAsset = imgLoader.load(entry.string() + "/" + p);

                    rhi::ImageInfo info{};
                    info.extent = {imgAsset.width, imgAsset.height, 1};
                    info.usage = rhi::ImageUsage::TRANSFER_DST | rhi::ImageUsage::SAMPLED;
                    if (imgAsset.channels == 4) {
                        info.format = rhi::Format::RGBA8_UNORM;
                    } else if (imgAsset.channels == 3) {
                        info.format = rhi::Format::RGB8_UNORM;
                    } else {
                        raum_check(false, "unsupported image format");
                    }

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

                    auto bufferSize = imgAsset.width * imgAsset.height * rhi::getFormatSize(info.format);
                    rhi::BufferSourceInfo bufferInfo{
                        .bufferUsage = rhi::BufferUsage::TRANSFER_SRC,
                        .size = bufferSize,
                        .data = imgAsset.data,
                    };
                    auto stagingBuffer = rhi::BufferPtr(device->createBuffer(bufferInfo));
                    rhi::BufferImageCopyRegion region{
                        .bufferSize = bufferSize,
                        .bufferOffset = 0,
                        .bufferRowLength = 0,
                        .bufferImageHeight = 0,
                        .imageAspect = rhi::AspectMask::COLOR,
                        .imageExtent = {
                            imgAsset.width,
                            imgAsset.height,
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
                    if (texType == aiTextureType_DIFFUSE) {
                        pbrMat->add(scene::Texture{"mainTexture", img, imgView});
                        pbrMat->add(scene::Sampler{"mainSampler",
                                                   {
                                                       .magFilter = rhi::Filter::LINEAR,
                                                       .minFilter = rhi::Filter::LINEAR,
                                                   }});
                    }
                    cmdBuffer->onComplete([stagingBuffer, img, imgView]() mutable {
                        stagingBuffer.reset();
                        img.reset();
                        imgView.reset();
                    });

                    imgLoader.free(std::move(imgAsset));
                }
            }
        }

        for (size_t i = aiTextureType_SHEEN; i <= static_cast<uint32_t>(aiTextureType_TRANSMISSION); ++i) {
            auto texType = static_cast<aiTextureType>(aiTextureType_SHEEN + i);
            if (material->GetTextureCount(texType) > 0) {
                if (material->GetTexture(texType, 0, &texturePath, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS) {
                    std::string p(texturePath.C_Str());
                    if (p.substr(0, 2) == ".\\") {
                        p = p.substr(2, p.size() - 2);
                    }
                    ImageLoader imgLoader;
                    auto imgAsset = imgLoader.load(entry.string() + "/" + p);

                    rhi::ImageInfo info{};
                    info.extent = {imgAsset.width, imgAsset.height, 1};
                    if (imgAsset.channels == 4) {
                        info.format = rhi::Format::RGBA8_UNORM;
                    } else if (imgAsset.channels == 3) {
                        info.format = rhi::Format::RGB8_UNORM;
                    } else {
                        raum_check(false, "unsupported image format");
                    }

                    auto img = rhi::ImagePtr(device->createImage(info));

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
                    pbrMat->add(scene::Texture{"", img, imgView});
                }
            }
        }
    }
}

void defaultResourceTransition(rhi::CommandBufferPtr commandBuffer, rhi::DevicePtr device) {
    auto sampledImage = rhi::defaultSampledImage(device);
    rhi::ImageBarrierInfo transition {
        .image = sampledImage.get(),
        .dstStage = rhi::PipelineStage::VERTEX_SHADER,
        .newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        .dstAccessFlag = rhi::AccessFlags::SHADER_READ,
        .range = {
            .aspect = rhi::AspectMask::COLOR,
            .sliceCount = 1,
            .mipCount = 1,
        }
    };
    commandBuffer->appendImageBarrier(transition);

    auto storageImage = rhi::defaultStorageImage(device);
    transition.image = storageImage.get();
    transition.newLayout = rhi::ImageLayout::GENERAL;
    transition.dstAccessFlag = rhi::AccessFlags::SHADER_WRITE;
    transition.dstStage = rhi::PipelineStage::VERTEX_SHADER | rhi::PipelineStage::COMPUTE_SHADER;
    commandBuffer->appendImageBarrier(transition);
    commandBuffer->applyBarrier({});
}

} // namespace

SceneLoader::SceneLoader(rhi::DevicePtr device) : _device(device) {
}

void SceneLoader::loadFlat(const std::filesystem::path& filePath) {
    raum_check(exists(filePath), "{} not exists!", filePath.string());

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath.string(),
                                             aiProcess_CalcTangentSpace |
                                                 aiProcess_Triangulate |
                                                 aiProcess_JoinIdenticalVertices |
                                                 aiProcess_FixInfacingNormals |
                                                 aiProcess_RemoveRedundantMaterials |
                                                 aiProcess_OptimizeMeshes |
                                                 aiProcess_GenBoundingBoxes);

    raum_check(scene, "failed to load file {}", filePath.string());

    _data = std::make_shared<scene::Model>();

    auto commandPool = rhi::CommandPoolPtr(_device->createCoomandPool({}));
    auto commandBuffer = rhi::CommandBufferPtr(commandPool->makeCommandBuffer({}));
    auto* queue = _device->getQueue({rhi::QueueType::GRAPHICS});
    commandBuffer->enqueue(queue);
    commandBuffer->begin({});

    std::vector<scene::TechniquePtr> defaultTechs;
    loadMaterial(scene, _data, defaultTechs, filePath, commandBuffer, _device);
    loadMesh(scene, scene->mRootNode, *_data, defaultTechs, _device);

    defaultResourceTransition(commandBuffer, _device);

    commandBuffer->commit();
    queue->submit();
}

} // namespace raum::asset