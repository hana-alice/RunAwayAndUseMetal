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
    BI_TANGENT = 1 << 3,
};
OPERABLE(ShaderAttribute);

struct MeshData {
    rhi::VertexBuffer vertexBuffer;
    rhi::IndexBuffer indexBuffer;
    ShaderAttribute shaderAttrs{ShaderAttribute::NONE};
    std::vector<rhi::VertexAttribute> attributes;
    rhi::VertexBufferAttribute bufferAttribute;
    uint32_t vertexCount{0};
    uint32_t indexCount{0};
};

class Mesh : public Renderable {
public:
    Mesh() = delete;
    Mesh(const MeshData& data);

    void addTechnique(TechniquePtr tech);
    void removeTechnique(uint32_t index);
    MeshData& meshData();
    const MeshData& meshData() const;
    MaterialPtr material() const;

private:
    MeshData _data;
    std::vector<TechniquePtr> _techs;
};

using MeshPtr = std::shared_ptr<Mesh>;

inline MeshPtr makeMesh(const MeshData& data) {
    return std::make_shared<Mesh>(data);
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

} // namespace raum::scene