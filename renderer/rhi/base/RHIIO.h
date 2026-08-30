#pragma once
namespace raum::rhi {
template <class Archive>
void serialize(Archive& ar, VertexAttribute& vertexAttr) {
    ar(vertexAttr.location, vertexAttr.binding, vertexAttr.format, vertexAttr.offset);
}

template <class Archive>
void serialize(Archive& ar, VertexBufferAttribute& attr) {
    ar(attr.binding, attr.stride, attr.rate);
}

template <class Archive>
void serialize(Archive& archive, VertexLayout& vertexLayout) {
    archive(vertexLayout.vertexAttrs, vertexLayout.vertexBufferAttrs);
}

} // namespace raum::rhi
