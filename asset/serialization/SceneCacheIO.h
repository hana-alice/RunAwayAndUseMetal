#pragma once

#include "RHIIO.h"
#include "SceneCache.h"
#include "SceneIO.h"

namespace raum::asset::serialize {

template <class Archive>
void serialize(Archive& archive, SceneCacheMetadata& metadata) {
    archive(metadata.version, metadata.sourceTimestamp);
}

template <class Archive>
void serialize(Archive& archive, TextureCache& texture) {
    archive(texture.sourceImageIndex,
            texture.width,
            texture.height,
            texture.mipCount,
            texture.colorSpace,
            texture.preservesAlphaCoverage,
            texture.alphaCutoff,
            texture.pixels);
}

template <class Archive>
void serialize(Archive& archive, MaterialTextureCache& texture) {
    archive(texture.textureIndex, texture.imageIndex, texture.uvIndex);
}

template <class Archive>
void serialize(Archive& archive, MaterialCache& material) {
    archive(material.alphaMode,
            material.doubleSided,
            material.alphaCutoff,
            material.baseColorFactor,
            material.emissiveFactor,
            material.metallicFactor,
            material.roughnessFactor,
            material.normalScale,
            material.occlusionStrength,
            material.baseColorTexture,
            material.metallicRoughnessTexture,
            material.normalTexture,
            material.occlusionTexture,
            material.emissiveTexture);
}

template <class Archive>
void serialize(Archive& archive, MeshPrimitiveCache& primitive) {
    archive(primitive.vertexCount,
            primitive.vertexData,
            primitive.indexCount,
            primitive.indexType,
            primitive.indexData,
            primitive.shaderAttributes,
            primitive.vertexLayout,
            primitive.materialIndex,
            primitive.primitiveMode,
            primitive.bounds);
}

template <class Archive>
void serialize(Archive& archive, MeshCache& mesh) {
    archive(mesh.worldTransform, mesh.primitives);
}

} // namespace raum::asset::serialize
