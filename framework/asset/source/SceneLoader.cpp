#include <numeric>
#include "SceneLoader.h"
#include "RHIUtils.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "define.h"

namespace raum::framework::asset {

namespace {
void loadMesh(const aiScene* scene, const aiNode* node, std::vector<MeshData>& meshes) {
    for (size_t i = 0; i < node->mNumMeshes; ++i) {
        bool useVertexColor{false};
        const auto* mesh = scene->mMeshes[node->mMeshes[i]];

        auto& meshData = meshes.emplace_back();
        meshData.materialIndex = mesh->mMaterialIndex;

        auto& meshBuffer = meshData.data;
        uint32_t stride{0};
        // pos
        stride += 3;

        const aiVector3D * normal{nullptr};
        if(mesh->HasNormals()) {
            stride += 3;
            normal = mesh->mNormals;
        }
        if(mesh->HasTextureCoords(0)) {
            stride += 2;
        }
        if(mesh->HasTangentsAndBitangents()) {
            stride += 3 + 3;
        }
        const aiColor4D * color{nullptr};
        if (mesh->HasVertexColors(0)) {
            color = mesh->mColors[0];
            stride += 4;
        }
        meshBuffer.resize(stride * mesh->mNumVertices);

        for (size_t j = 0; j < mesh->mNumVertices; ++j) {
            uint32_t accStride{3};
            auto* vertData = meshBuffer.data() + j * stride;

            const auto& vertex = mesh->mVertices[j];
            vertData[0] = vertex.x;
            vertData[1] = vertex.y;
            vertData[2] = vertex.z;

            if(normal) {
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

            if(mesh->HasTangentsAndBitangents()) {
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
            auto* indexData = meshData.indices.data();
            uint32_t indexNum{0};
            indexNum = std::accumulate(mesh->mFaces, mesh->mFaces + mesh->mNumFaces, indexNum, [](uint32_t curr, const aiFace& face) {
                return curr + face.mNumIndices;
            });

            if (indexNum >= std::numeric_limits<rhi::HalfIndexType>::max()) {
                meshData.indexType = rhi::IndexType::FULL;
                meshData.indices.resize(indexNum * 4);
            } else {
                meshData.indices.resize(indexNum * 2);
            }

            uint32_t count{0};
            for (size_t j = 0; j < mesh->mNumFaces; ++j) {
                const auto& face = mesh->mFaces[j];
                for (size_t k = 0; k < face.mNumIndices; ++k) {
                    if (meshData.indexType == rhi::IndexType::FULL) {
                        reinterpret_cast<rhi::FullIndexType*>(indexData)[++count] = face.mIndices[k];
                    } else {
                        reinterpret_cast<rhi::HalfIndexType*>(indexData)[++count] = face.mIndices[k];
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < node->mNumChildren; ++i) {
        loadMesh(scene, node->mChildren[i], meshes);
    }
}

void loadMaterial(const aiScene* scene) {
    for (size_t i = 0; i < scene->mNumMaterials; ++i) {
        const auto* material = scene->mMaterials[i];
//        material->GetT
    }
}

}

void SceneLoader::load(const std::filesystem::path& filePath) {
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

    loadMesh(scene, scene->mRootNode, _data.meshes);
}

} // namespace raum::framework::asset