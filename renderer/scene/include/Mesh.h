#pragma once
#include <stdint.h>
#include <vector>
#include "Common.h"
#include "RHIDefine.h"
#include "Technique.h"
#include "BindGroup.h"

namespace raum::scene {

constexpr uint32_t MESHLET_VERTEX_COUNT{64};
constexpr uint32_t MESHLET_PRIM_COUNT{126};

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
    rhi::VertexLayout vertexLayout;
    uint32_t vertexCount{0};
    uint32_t indexCount{0};
};

struct MeshletDesc {
    uint32_t vertexCount{0};
    uint32_t primCount{0};
    uint32_t vertexBegin{0};
    uint32_t primBegin{0};
};

struct MeshletData {
    std::vector<MeshletDesc> meshlets;
    rhi::BufferPtr vertexBuffer;
    rhi::BufferPtr meshletCountBuffer;
    rhi::BufferPtr meshletsBuffer;
    rhi::BufferPtr primIndicesBuffer;
    rhi::BufferPtr vertexIndicesBuffer;
    ShaderAttribute shaderAttrs{ShaderAttribute::NONE};
    rhi::VertexLayout vertexLayout;
    uint32_t vertexCount{0};
    uint32_t indexCount{0};
    BindGroupPtr bindGroup;
};
using MeshletDataPtr = std::shared_ptr<MeshletData>;

enum class MeshType : uint8_t {
    VERTEX, // << vertex shader
    MESH,   // << mesh shader
};

class Mesh {
public:
    Mesh(MeshType type = MeshType::VERTEX);

    const MeshType type() const;

    MeshData& meshData();
    const MeshData& meshData() const;

    MeshletData& meshletData();
    const MeshletData& meshletData() const;

private:
    MeshType _type;
    MeshData _data;
    MeshletDataPtr _meshletsData;
};

using MeshPtr = std::shared_ptr<Mesh>;

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

struct DrawInfo {
    uint32_t firstVertex{0};
    uint32_t vertexCount{0};
    uint32_t firstInstance{0};
    uint32_t instanceCount{1};
    uint32_t indexCount{0};
    uint32_t vertexOffset{0};
};

class MeshRenderer : public Renderable {
public:
    MeshRenderer() = delete;
    MeshRenderer(MeshPtr mesh);

    void addTechnique(TechniquePtr tech);
    void removeTechnique(uint32_t index);
    void setMesh(MeshPtr mesh);
    void setVertexInfo(uint32_t firstVertex, uint32_t vertexCount, uint32_t indexCount);
    void setInstanceInfo(uint32_t firstInstance, uint32_t instanceCount);

    void prepare(rhi::DevicePtr device);

    const MeshPtr& mesh() const;
    TechniquePtr technique(uint32_t index);
    const DrawInfo& drawInfo() const;
    std::vector<TechniquePtr>& techniques();

private:
    MeshPtr _mesh;
    std::vector<TechniquePtr> _techs;
    DrawInfo _drawInfo{};
};
using MeshRendererPtr = std::shared_ptr<MeshRenderer>;

} // namespace raum::scene