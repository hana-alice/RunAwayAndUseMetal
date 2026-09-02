#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "Mesh.h"

namespace raum::asset::serialize {

// Bump this whenever one of the cache records below changes.
inline constexpr uint32_t SceneCacheVersion{5};
inline constexpr std::string_view SceneCacheMetadataFile{".raum-cache"};

struct SceneCacheMetadata {
    uint32_t version{0};
    int64_t sourceTimestamp{0};
};

enum class TextureColorSpace : uint8_t {
    LINEAR,
    SRGB,
};

struct TextureCache {
    int32_t sourceImageIndex{-1};
    uint32_t width{0};
    uint32_t height{0};
    uint32_t mipCount{1};
    TextureColorSpace colorSpace{TextureColorSpace::LINEAR};
    bool preservesAlphaCoverage{false};
    float alphaCutoff{0.5f};
    std::vector<uint8_t> pixels;
};

struct MaterialTextureCache {
    int32_t textureIndex{-1};
    int32_t imageIndex{-1};
    int32_t uvIndex{-1};

    bool enabled() const {
        return textureIndex >= 0;
    }
};

struct MaterialCache {
    std::string alphaMode{"OPAQUE"};
    bool doubleSided{false};
    double alphaCutoff{0.5};

    Vec4f baseColorFactor{1.0f};
    Vec3f emissiveFactor{0.0f};
    float metallicFactor{1.0f};
    float roughnessFactor{1.0f};
    float normalScale{1.0f};
    float occlusionStrength{1.0f};

    MaterialTextureCache baseColorTexture;
    MaterialTextureCache metallicRoughnessTexture;
    MaterialTextureCache normalTexture;
    MaterialTextureCache occlusionTexture;
    MaterialTextureCache emissiveTexture;
};

struct MeshPrimitiveCache {
    uint32_t vertexCount{0};
    std::vector<float> vertexData;
    uint32_t indexCount{0};
    rhi::IndexType indexType{rhi::IndexType::FULL};
    std::vector<uint8_t> indexData;
    scene::ShaderAttribute shaderAttributes{scene::ShaderAttribute::NONE};
    rhi::VertexLayout vertexLayout;
    int32_t materialIndex{-1};
    int32_t primitiveMode{0};
    scene::AABB bounds;
};

struct MeshCache {
    Mat4 worldTransform{1.0f};
    std::vector<MeshPrimitiveCache> primitives;
};

} // namespace raum::asset::serialize
