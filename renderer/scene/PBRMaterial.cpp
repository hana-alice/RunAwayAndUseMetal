#include "PBRMaterial.h"

#include <tiny_gltf.h>

namespace raum::scene {

//void PBRMaterial::setBaseColorSlot(std::string_view slot) {
//    setSlot(TextureType::BASE_COLOR, slot);
//}
//
//void PBRMaterial::setNormalSlot(std::string_view slot) {
//    setSlot(TextureType::NORMAL, slot);
//}
//
//void PBRMaterial::setEmissiveSlot(std::string_view slot) {
//    setSlot(TextureType::EMISSIVE, slot);
//}
//
//void PBRMaterial::setMetallicRoughnessSlot(std::string_view slot) {
//    setSlot(TextureType::METALLIC_ROUGHNESS, slot);
//}
//
//void PBRMaterial::setAmbientOcclusionSlot(std::string_view slot) {
//    setSlot(TextureType::AO, slot);
//}
//
//void PBRMaterial::setSlot(TextureType type, std::string_view slot) {
//    _pbrTextures[static_cast<uint32_t>(type)] = slot;
//    _textures[static_cast<uint32_t>(type)].name = slot;
//}

void PBRMaterial::setAlphaMode(AlphaMode mode) {
    _alphaMode = mode;
}

void PBRMaterial::setDoubleSided(bool doubleSided) {
    _doubleSided = doubleSided;
}

void PBRMaterial::setAlphaCutoff(float cutoff) {
    _alphaCutoff = cutoff;
}

void PBRMaterial::setEmissiveFactor(float r, float g, float b) {
    _emissiveFactor = {r, g, b};
}

void PBRMaterial::setBaseColorFactor(float r, float g, float b, float a) {
    _baseColorFactor = {r, g, b, a};
}

void PBRMaterial::setMetallicFactor(float factor) {
    _metallicFactor = factor;
}

void PBRMaterial::setRoughnessFactor(float factor) {
    _roughnessFactor = factor;
}

void PBRMaterial::setNormalScale(float scale) {
    _normalScale = scale;
}

void PBRMaterial::setOcclusionStrength(float strength) {
    _occlusionStrength = strength;
}

PBRMaterial::AlphaMode PBRMaterial::alphaMode() const {
    return _alphaMode;
}

bool PBRMaterial::doubleSided() const {
    return _doubleSided;
}

float PBRMaterial::alphaCutoff() const {
    return _alphaCutoff;
}

const std::array<float, 3>& PBRMaterial::emissiveFactor() const {
    return _emissiveFactor;
}

const std::array<float, 4>& PBRMaterial::baseColorFactor() const {
    return _baseColorFactor;
}

float PBRMaterial::metallicFactor() const {
    return _metallicFactor;
}

float PBRMaterial::roughnessFactor() const {
    return _roughnessFactor;
}

float PBRMaterial::normalScale() const {
    return _normalScale;
}

float PBRMaterial::occlusionStrength() const {
    return _occlusionStrength;
}

} // namespace raum::scene