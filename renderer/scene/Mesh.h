#pragma once
#include <stdint.h>
#include <vector>
#include "Common.h"
#include "RHIDefine.h"
#include "Technique.h"

namespace raum::scene {

enum class ShaderAttribute : uint8_t {
    NONE = 0,
    POSITION = 1,
    NORMAL = 1 << 1,
    UV = 1 << 2,
    TANGENT = 1 << 3,
    COLOR = 1 << 4,
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

class Mesh {
public:
    Mesh() = default;

    MeshData& meshData();
    const MeshData& meshData() const;

private:
    MeshData _data;
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
    void setTransform(const Mat4& transform);

    void setTransformSlot(std::string_view name);

    const MeshPtr& mesh() const;
    TechniquePtr technique(uint32_t index);
    const DrawInfo& drawInfo() const;
    std::vector<TechniquePtr>& techniques();
    BindGroupPtr bindGroup();

    void prepare(const SlotMap& bindings,
                 rhi::DescriptorSetLayoutPtr layout,
                 rhi::DevicePtr device);

    void update(rhi::CommandBufferPtr cmdBuffer);

    
    bool hasInstanceBinding() const;
    bool hasPassBinding() const;

private:
    bool _dirty{false};
    std::string _localSLotName;
    MeshPtr _mesh;
    std::vector<TechniquePtr> _techs;
    DrawInfo _drawInfo{};
    Mat4 _transform{1.0};
    BindGroupPtr _bindGroup;
    rhi::BufferPtr _localBuffer;
};
using MeshRendererPtr = std::shared_ptr<MeshRenderer>;

} // namespace raum::scene