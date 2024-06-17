#include "GeometryMesh.h"

namespace raum::scene {

rhi::VertexLayout Cube::vertexLayout = {
    .vertexAttrs = {
        {
            0,
            0,
            rhi::Format::RGB32_SFLOAT,
            0,
        },
        {
            1,
            0,
            rhi::Format::RG32_SFLOAT,
            12,
        }},
    .vertexBufferAttrs = {{
        0,
        20,
        rhi::InputRate::PER_VERTEX,
    }}};

rhi::VertexBuffer Cube::vertexBuffer;

}