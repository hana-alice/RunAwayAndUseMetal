#pragma once
#include <stdint.h>
#include <vector>
#include "Scene.h"
#include "RHIDefine.h"
#include "Common.h"
namespace raum::scene {

enum class ShaderAttribute: uint8_t {
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

class Mesh : public Renderobject {
public:
//    void attachToMaterial();
//
//private:
    uint32_t materialID{0};
    MeshData data;
};

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

struct AABB {
    Vec3f minBound{};
    Vec3f maxBound{};
};

class Model : public Renderobject {
public:
    std::vector<Mesh> meshes;
    std::vector<MaterialData> materials;
    AABB aabb{};
};



} // namespace raum::scene