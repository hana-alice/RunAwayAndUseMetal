#pragma once
#include <stdint.h>
#include <vector>
#include "Common.h"
#include "RHIDefine.h"
#include "Scene.h"
namespace raum::scene {

enum class ShaderAttribute : uint8_t {
    NONE = 0,
    POSITION = 1,
    NORMAL = 1 << 1,
    UV = 1 << 2,
    BI_TANGENT = 1 << 3,
};
OPERABLE(ShaderAttribute);

struct MeshData {
    rhi::VertexBuffer vertexBuffer;
    rhi::IndexBuffer indexBuffer;
    ShaderAttribute shaderAttrs{ShaderAttribute::NONE};
    std::vector<rhi::VertexAttribute> attributes;
    rhi::VertexBufferAttribute bufferAttribute;
};

class Mesh : public Renderable {
public:
    uint32_t materialID{0};
    MeshData data;
};
using MeshPtr = std::shared_ptr<Mesh>;

inline MeshPtr makeMesh() {
    return std::make_shared<Mesh>();
}

class PCGMesh : public Mesh {
public:
    //    PCGMesh() {};
};

class MeshAllocator {
public:
    MeshAllocator() = delete;
    MeshAllocator(const MeshAllocator&) = delete;
    MeshAllocator(MeshAllocator&&) = delete;
    MeshAllocator& operator=(const MeshAllocator&) = delete;

    const Mesh& makeMesh();
    const PCGMesh& makePCGMesh();
};

using MeshRef = std::reference_wrapper<Mesh>;

enum class TextureType : uint32_t {
    DIFFUSE,
    SPECULAR,
    AMBIENT,
    EMISSIVE,
    HEIGHT,
    NORMALS,
    SHININESS,
    OPACITY,
    DISPLACEMENT,
    LIGHTMAP,
    REFLECTION,

    BASE_COLOR,
    NORMAL_CAMERA,
    EMISSION_COLOR,
    METALNESS,
    DIFFUSE_ROUGHNESS,
    AMBIENT_OCCLUSION,

    SHEEN,
    CLEARCOAT,
    TRANSMISSION,

    COUNT,
};

struct MaterialData {
    std::string name;
    std::array<rhi::ImagePtr, static_cast<uint32_t>(TextureType::COUNT)> images;
};
using MaterialDataPtr = std::shared_ptr<MaterialData>;

inline MaterialDataPtr makeMaterialData() {
    return std::make_shared<MaterialData>();
}

struct AABB {
    Vec3f minBound{};
    Vec3f maxBound{};
};

class Model {
public:
    std::vector<MeshPtr> meshes;
    std::vector<MaterialDataPtr> materials;
    AABB aabb{};
};
using ModelPtr = std::shared_ptr<Model>;

inline ModelPtr makeModel() {
    return std::make_shared<Model>();
}

} // namespace raum::scene