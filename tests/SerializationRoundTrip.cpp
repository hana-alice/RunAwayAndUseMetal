#include <chrono>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include "asset/serialization/SceneCacheIO.h"
#include "asset/serialization/TextureProcessor.h"
#include "core/utils/Archive.h"
#include "renderer/graph/GraphIO.h"
#include "renderer/rhi/base/RHIUtils.h"

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
        .sourceImageIndex = 7,
        .width = 2,
        .height = 1,
        .mipCount = 1,
        .colorSpace = TextureColorSpace::SRGB,
        .preservesAlphaCoverage = true,
        .alphaCutoff = 0.4f,
        .pixels = {1, 2, 3, 4, 5, 6, 7, 8},
    };
    const auto loadedTexture = roundTrip(texture);
    require(loadedTexture.sourceImageIndex == texture.sourceImageIndex &&
                loadedTexture.width == texture.width &&
                loadedTexture.height == texture.height &&
                loadedTexture.mipCount == texture.mipCount &&
                loadedTexture.colorSpace == texture.colorSpace &&
                loadedTexture.preservesAlphaCoverage == texture.preservesAlphaCoverage &&
                same(loadedTexture.alphaCutoff, texture.alphaCutoff) &&
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

void testFormatAspectMasks() {
    using rhi::AspectMask;
    require(rhi::formatAspectMask(rhi::Format::RGBA8_UNORM) == AspectMask::COLOR,
            "Color format aspect inference failed");
    require(rhi::formatAspectMask(rhi::Format::D16_UNORM) == AspectMask::DEPTH,
            "Depth-only format aspect inference failed");
    require(rhi::formatAspectMask(rhi::Format::D32_SFLOAT) == AspectMask::DEPTH,
            "D32 format aspect inference failed");
    require(rhi::formatAspectMask(rhi::Format::S8_UINT) == AspectMask::STENCIL,
            "Stencil-only format aspect inference failed");
    require(rhi::formatAspectMask(rhi::Format::D24_UNORM_S8_UINT) ==
                (AspectMask::DEPTH | AspectMask::STENCIL),
            "Combined depth-stencil format aspect inference failed");
    require(rhi::formatAspectMask(rhi::Format::D16_UNORM_S8_UINT) ==
                (AspectMask::DEPTH | AspectMask::STENCIL),
            "D16/S8 format aspect inference failed");
    require(rhi::formatAspectMask(rhi::Format::D32_SFLOAT_S8_UINT) ==
                (AspectMask::DEPTH | AspectMask::STENCIL),
            "D32/S8 format aspect inference failed");
}

void testTextureMipProcessing() {
    std::vector<uint8_t> pixels(4 * 4 * 4, 255);
    for (uint32_t y = 0; y < 4; ++y) {
        for (uint32_t x = 0; x < 4; ++x) {
            pixels[(y * 4 + x) * 4 + 3] = x < 2 ? 255 : 0;
        }
    }

    const auto alphaChain = buildTextureMipChain(
        4, 4, pixels, TextureMipBuildInfo{.srgbColor = true, .alphaCutoff = 0.5f});
    require(alphaChain.mipCount == 3 && alphaChain.pixels.size() == (16 + 4 + 1) * 4,
            "Texture mip-chain dimensions failed");
    const std::span<const uint8_t> mipOne{alphaChain.pixels.data() + 16 * 4, 4 * 4};
    require(same(calculateAlphaCoverage(mipOne, 0.5f), 0.5f),
            "Alpha coverage was not preserved in the first reduced mip");

    std::vector<uint8_t> colorPixels{
        0, 0, 0, 255,
        255, 255, 255, 255,
        0, 0, 0, 255,
        255, 255, 255, 255,
    };
    const auto linearChain = buildTextureMipChain(
        2, 2, colorPixels, TextureMipBuildInfo{.srgbColor = false});
    const auto srgbChain = buildTextureMipChain(
        2, 2, colorPixels, TextureMipBuildInfo{.srgbColor = true});
    require(srgbChain.pixels[16] > linearChain.pixels[16],
            "sRGB-aware color downsampling failed");
    require(same(calculateAlphaCoverage(pixels, 1.1f), 0.0f),
            "An effective alpha cutoff above one must have zero coverage");
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
    std::string slicedStorage{"prefix/sliced/suffix"};
    source.addEmpty(std::string_view{slicedStorage}.substr(7, 6), "root");
    slicedStorage.clear();
    require(source.get("sliced").name == "sliced", "SceneGraph did not retain an interned node name");
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

    require(boost::num_vertices(loaded.impl()) == 3 && boost::num_edges(loaded.impl()) == 2,
            "SceneGraph topology round trip failed");
    const auto& loadedChild = loaded.get("child");
    require(!loadedChild.node.enabled() && same(loadedChild.node.transform(), transform),
            "SceneGraph node round trip failed");
}

void testArchiveFailurePolicy() {
    const auto missingPath = std::filesystem::temp_directory_path() /
                             "raum-archive-path-that-must-not-exist.bin";
    std::error_code ignored;
    std::filesystem::remove(missingPath, ignored);

    bool caught{false};
    try {
        utils::InputArchive archive(missingPath);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    require(caught, "The throwing Raum error policy did not report archive-open failure");
}

} // namespace

int main() {
    testSceneCacheRecords();
    testFormatAspectMasks();
    testTextureMipProcessing();
    testCameraRoundTrip();
    testSceneGraphRoundTrip();
    testArchiveFailurePolicy();
}
