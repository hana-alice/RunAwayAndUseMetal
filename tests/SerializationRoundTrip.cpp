#include <chrono>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include "asset/serialization/SceneCache.h"
#include "core/utils/Archive.h"
#include "renderer/graph/GraphIO.h"

namespace {

using namespace raum;
using namespace raum::asset::serialize;

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool same(float lhs, float rhs) {
    return std::abs(lhs - rhs) < 0.00001f;
}

bool same(const Vec3f& lhs, const Vec3f& rhs) {
    return same(lhs.x, rhs.x) && same(lhs.y, rhs.y) && same(lhs.z, rhs.z);
}

bool same(const Vec4f& lhs, const Vec4f& rhs) {
    return same(lhs.x, rhs.x) && same(lhs.y, rhs.y) && same(lhs.z, rhs.z) && same(lhs.w, rhs.w);
}

bool same(const Mat4& lhs, const Mat4& rhs) {
    for (size_t column = 0; column < 4; ++column) {
        if (!same(lhs[column], rhs[column])) {
            return false;
        }
    }
    return true;
}

bool same(const MaterialTextureCache& lhs, const MaterialTextureCache& rhs) {
    return lhs.textureIndex == rhs.textureIndex &&
           lhs.imageIndex == rhs.imageIndex &&
           lhs.uvIndex == rhs.uvIndex;
}

template <typename T>
T roundTrip(const T& source) {
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    {
        cereal::BinaryOutputArchive archive(stream);
        archive(source);
    }
    stream.seekg(0);

    T result;
    {
        cereal::BinaryInputArchive archive(stream);
        archive(result);
    }
    return result;
}

void testSceneCacheRecords() {
    const SceneCacheMetadata metadata{SceneCacheVersion, 123456789};
    const auto loadedMetadata = roundTrip(metadata);
    require(loadedMetadata.version == metadata.version &&
                loadedMetadata.sourceTimestamp == metadata.sourceTimestamp,
            "SceneCacheMetadata round trip failed");

    TextureCache texture{
        .width = 2,
        .height = 1,
        .pixels = {1, 2, 3, 4, 5, 6, 7, 8},
    };
    const auto loadedTexture = roundTrip(texture);
    require(loadedTexture.width == texture.width &&
                loadedTexture.height == texture.height &&
                loadedTexture.pixels == texture.pixels,
            "TextureCache round trip failed");

    MaterialCache material;
    material.alphaMode = "MASK";
    material.doubleSided = true;
    material.alphaCutoff = 0.25;
    material.baseColorFactor = {0.1f, 0.2f, 0.3f, 0.4f};
    material.emissiveFactor = {0.5f, 0.6f, 0.7f};
    material.metallicFactor = 0.8f;
    material.roughnessFactor = 0.9f;
    material.normalScale = 1.1f;
    material.occlusionStrength = 0.75f;
    material.baseColorTexture = {1, 2, 3};
    material.metallicRoughnessTexture = {4, 5, 6};
    material.normalTexture = {7, 8, 9};
    material.occlusionTexture = {10, 11, 12};
    material.emissiveTexture = {13, 14, 15};

    const auto loadedMaterial = roundTrip(material);
    require(loadedMaterial.alphaMode == material.alphaMode &&
                loadedMaterial.doubleSided == material.doubleSided &&
                loadedMaterial.alphaCutoff == material.alphaCutoff &&
                same(loadedMaterial.baseColorFactor, material.baseColorFactor) &&
                same(loadedMaterial.emissiveFactor, material.emissiveFactor) &&
                same(loadedMaterial.metallicFactor, material.metallicFactor) &&
                same(loadedMaterial.roughnessFactor, material.roughnessFactor) &&
                same(loadedMaterial.normalScale, material.normalScale) &&
                same(loadedMaterial.occlusionStrength, material.occlusionStrength) &&
                same(loadedMaterial.baseColorTexture, material.baseColorTexture) &&
                same(loadedMaterial.metallicRoughnessTexture, material.metallicRoughnessTexture) &&
                same(loadedMaterial.normalTexture, material.normalTexture) &&
                same(loadedMaterial.occlusionTexture, material.occlusionTexture) &&
                same(loadedMaterial.emissiveTexture, material.emissiveTexture),
            "MaterialCache round trip failed");

    MeshCache mesh;
    mesh.worldTransform[3] = Vec4f{4.0f, 5.0f, 6.0f, 1.0f};
    auto& primitive = mesh.primitives.emplace_back();
    primitive.vertexCount = 1;
    primitive.vertexData = {1.0f, 2.0f, 3.0f};
    primitive.indexCount = 3;
    primitive.indexType = rhi::IndexType::HALF;
    primitive.indexData = {0, 0, 0, 0, 0, 0};
    primitive.shaderAttributes = scene::ShaderAttribute::POSITION;
    primitive.vertexLayout.vertexAttrs.push_back({0, 0, rhi::Format::RGB32_SFLOAT, 0});
    primitive.vertexLayout.vertexBufferAttrs.push_back({0, 12, rhi::InputRate::PER_VERTEX});
    primitive.materialIndex = 7;
    primitive.primitiveMode = 4;
    primitive.bounds = {{-1.0f, -2.0f, -3.0f}, {1.0f, 2.0f, 3.0f}};

    const auto loadedMesh = roundTrip(mesh);
    require(same(loadedMesh.worldTransform, mesh.worldTransform) && loadedMesh.primitives.size() == 1,
            "MeshCache round trip failed");
    const auto& loadedPrimitive = loadedMesh.primitives.front();
    require(loadedPrimitive.vertexCount == primitive.vertexCount &&
                loadedPrimitive.vertexData == primitive.vertexData &&
                loadedPrimitive.indexCount == primitive.indexCount &&
                loadedPrimitive.indexType == primitive.indexType &&
                loadedPrimitive.indexData == primitive.indexData &&
                loadedPrimitive.shaderAttributes == primitive.shaderAttributes &&
                loadedPrimitive.vertexLayout.vertexAttrs.size() == 1 &&
                loadedPrimitive.vertexLayout.vertexAttrs.front().location ==
                    primitive.vertexLayout.vertexAttrs.front().location &&
                loadedPrimitive.vertexLayout.vertexAttrs.front().binding ==
                    primitive.vertexLayout.vertexAttrs.front().binding &&
                loadedPrimitive.vertexLayout.vertexAttrs.front().format ==
                    primitive.vertexLayout.vertexAttrs.front().format &&
                loadedPrimitive.vertexLayout.vertexAttrs.front().offset ==
                    primitive.vertexLayout.vertexAttrs.front().offset &&
                loadedPrimitive.vertexLayout.vertexBufferAttrs.size() == 1 &&
                loadedPrimitive.vertexLayout.vertexBufferAttrs.front().binding ==
                    primitive.vertexLayout.vertexBufferAttrs.front().binding &&
                loadedPrimitive.vertexLayout.vertexBufferAttrs.front().stride ==
                    primitive.vertexLayout.vertexBufferAttrs.front().stride &&
                loadedPrimitive.vertexLayout.vertexBufferAttrs.front().rate ==
                    primitive.vertexLayout.vertexBufferAttrs.front().rate &&
                loadedPrimitive.materialIndex == primitive.materialIndex &&
                loadedPrimitive.primitiveMode == primitive.primitiveMode &&
                same(loadedPrimitive.bounds.minBound, primitive.bounds.minBound) &&
                same(loadedPrimitive.bounds.maxBound, primitive.bounds.maxBound),
            "MeshPrimitiveCache round trip failed");
}

void testCameraRoundTrip() {
    const scene::OrthoFrustum frustum{-4.0f, 5.0f, -6.0f, 7.0f, 0.25f, 800.0f};
    auto camera = std::make_shared<scene::Camera>(frustum);
    camera->eye().setPosition({1.0f, 2.0f, 3.0f});
    camera->eye().setOrientation(Quaternion{0.9f, 0.1f, 0.2f, 0.3f});
    camera->disableCulling();

    const auto loadedCamera = roundTrip(camera);
    require(static_cast<bool>(loadedCamera), "Camera pointer round trip failed");
    const auto& sourceOrientation = camera->eye().getOrientation();
    const auto& loadedOrientation = loadedCamera->eye().getOrientation();
    require(loadedCamera->eye().projectionType() == scene::Projection::ORTHOGRAPHIC &&
                same(loadedCamera->eye().getPosition(), camera->eye().getPosition()) &&
                same(Vec4f{loadedOrientation.x, loadedOrientation.y, loadedOrientation.z, loadedOrientation.w},
                     Vec4f{sourceOrientation.x, sourceOrientation.y, sourceOrientation.z, sourceOrientation.w}) &&
                !loadedCamera->cullingEnabled(),
            "Camera round trip failed");
    const auto& loadedFrustum = loadedCamera->eye().getOrthoFrustum();
    require(same(loadedFrustum.left, frustum.left) &&
                same(loadedFrustum.right, frustum.right) &&
                same(loadedFrustum.bottom, frustum.bottom) &&
                same(loadedFrustum.top, frustum.top) &&
                same(loadedFrustum.near, frustum.near) &&
                same(loadedFrustum.far, frustum.far),
            "Camera frustum round trip failed");
}

void testSceneGraphRoundTrip() {
    graph::SceneGraph source;
    source.addEmpty("root");
    source.addEmpty("child", "root");
    source.get("child").node.disable();
    Mat4 transform{1.0f};
    transform[3] = Vec4f{7.0f, 8.0f, 9.0f, 1.0f};
    source.get("child").node.setTransform(transform);

    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto archivePath = std::filesystem::temp_directory_path() /
                             ("raum-serialization-" + std::to_string(suffix) + ".bin");
    {
        utils::OutputArchive archive(archivePath);
        archive << source;
    }

    graph::SceneGraph loaded;
    loaded.addEmpty("stale");
    {
        utils::InputArchive archive(archivePath);
        archive >> loaded;
    }
    std::error_code ignored;
    std::filesystem::remove(archivePath, ignored);

    require(boost::num_vertices(loaded.impl()) == 2 && boost::num_edges(loaded.impl()) == 1,
            "SceneGraph topology round trip failed");
    const auto& loadedChild = loaded.get("child");
    require(!loadedChild.node.enabled() && same(loadedChild.node.transform(), transform),
            "SceneGraph node round trip failed");
}

} // namespace

int main() {
    testSceneCacheRecords();
    testCameraRoundTrip();
    testSceneGraphRoundTrip();
}
