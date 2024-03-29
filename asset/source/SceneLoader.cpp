#include "SceneLoader.h"
#include <numeric>
#include "ImageLoader.h"
#include "RHIUtils.h"
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

void loadMesh(const aiScene* scene, const aiNode* node, std::vector<scene::MeshPtr>& meshes, scene::AABB& aabb, rhi::DevicePtr device) {
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        uint32_t location{0};
        const auto* mesh = scene->mMeshes[node->mMeshes[i]];
        expand(aabb, mesh->mAABB);

        auto& newMesh = meshes.emplace_back(scene::makeMesh());
        newMesh->materialID = mesh->mMaterialIndex;

        auto& meshData = newMesh->data;

        std::vector<float> rawData;
        auto& meshVert = rawData;
        uint32_t stride{0};
        // pos
        stride += 3;
        meshData.attributes.emplace_back(rhi::VertexAttribute{
            location++,
            0,
            rhi::Format::RGB32_SFLOAT,
        });

        meshData.shaderAttrs = scene::ShaderAttribute::POSITION;

        const aiVector3D* normal{nullptr};
        if (mesh->HasNormals()) {
            stride += 3;
            normal = mesh->mNormals;
            meshData.attributes.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
            });
            meshData.shaderAttrs |= scene::ShaderAttribute::NORMAL;
        }
        if (mesh->HasTextureCoords(0)) {
            stride += 2;
            meshData.attributes.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RG32_SFLOAT,
            });
            meshData.shaderAttrs |= scene::ShaderAttribute::UV;
        }
        if (mesh->HasTangentsAndBitangents()) {
            stride += 3 + 3;
            meshData.attributes.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
            });
            meshData.attributes.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGB32_SFLOAT,
            });
            meshData.shaderAttrs |= scene::ShaderAttribute::BI_TANGENT;
        }
        const aiColor4D* color{nullptr};
        if (mesh->HasVertexColors(0)) {
            color = mesh->mColors[0];
            stride += 4;
            meshData.attributes.emplace_back(rhi::VertexAttribute{
                location++,
                0,
                rhi::Format::RGBA32_SFLOAT,
            });
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

        if (mesh->HasFaces()) {
            std::vector<uint8_t> indices;
            uint32_t indexNum{0};
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

            rhi::BufferInfo bufferInfo{
                .memUsage = rhi::MemoryUsage::DEVICE_ONLY,
                .bufferUsage = rhi::BufferUsage::INDEX,
                .size = static_cast<uint32_t>(indices.size()),
            };
            meshData.indexBuffer.buffer = device->createBuffer(bufferInfo);
        }
        auto& bufferAttribute = meshData.bufferAttribute;
        bufferAttribute.binding = 0;
        bufferAttribute.rate = rhi::InputRate::PER_VERTEX;
        bufferAttribute.stride = stride;

        rhi::BufferInfo bufferInfo {
            .memUsage = rhi::MemoryUsage::DEVICE_ONLY,
            .bufferUsage = rhi::BufferUsage::VERTEX,
            .size = static_cast<uint32_t>(meshVert.size()),
        };

        meshData.vertexBuffer.buffer = device->createBuffer(bufferInfo);
    }

    for (size_t i = 0; i < node->mNumChildren; ++i) {
        loadMesh(scene, node->mChildren[i], meshes, aabb, device);
    }
}

void loadMaterial(const aiScene* scene,
                  std::vector<scene::MaterialDataPtr>& mats,
                  std::filesystem::path file,
                  rhi::DevicePtr& device) {
    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        const auto* material = scene->mMaterials[i];
        auto& matData = mats.emplace_back(scene::makeMaterialData());
        matData->name = material->GetName().C_Str();
        if (matData->name.empty()) {
            matData->name = (file / std::to_string(i)).string();
        }
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
                    if (imgAsset.channels == 4) {
                        info.format = rhi::Format::RGBA8_UNORM;
                    } else if (imgAsset.channels == 3) {
                        info.format = rhi::Format::RGB8_UNORM;
                    } else {
                        raum_check(false, "unsupported image format");
                    }

                    matData->images[i] = std::shared_ptr<rhi::RHIImage>(device->createImage(info));
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

                    matData->images[i] = std::shared_ptr<rhi::RHIImage>(device->createImage(info));
                }
            }
        }

    }
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

    _data = scene::makeModel();
    loadMesh(scene, scene->mRootNode, _data->meshes, _data->aabb, _device);
    loadMaterial(scene, _data->materials, filePath, _device);
}

} // namespace