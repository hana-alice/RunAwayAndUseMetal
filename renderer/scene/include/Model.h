#pragma once
#include <stdint.h>
#include <vector>
#include "Scene.h"
#include "RHIDefine.h"
namespace raum::scene {

enum class VertexAttribute: uint8_t {
    POSITION,
    NORMAL,
    UV,
    TANGENT,
    BITANGENT,
    COLOR,
};

struct MeshBuffer {
    uint32_t stride{0};
    uint32_t offset{0};
    uint32_t size{0};
    rhi::RHIBuffer* buffer{nullptr};
};

class Mesh : public Renderobject {
public:
    void attachToMaterial();

private:
    uint32_t materialID{0};
    std::vector<MeshBuffer> _meshBuffers;
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

class Model : public Renderobject {
public:
    std::vector<MeshBuffer> meshes;
};

} // namespace raum::scene