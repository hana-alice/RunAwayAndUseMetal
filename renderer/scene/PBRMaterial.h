#pragma once
#include "Material.h"

namespace raum::scene {

class PBRMaterial : public Material {
public:
    enum class AlphaMode : uint8_t {
        AM_OPAQUE,
        AM_MASK,
        AM_BLEND,
    };

    using Material::Material;

    // set shader slot
    void setBaseColorSlot(std::string_view slot);
    void setNormalSlot(std::string_view slot);
    void setEmissiveSlot(std::string_view slot);
    void setMetallicRoughnessSlot(std::string_view  slot);
    void setAmbientOcclusionSlot(std::string_view slot);
    void setSlot(TextureType type, std::string_view slot);

    void setAlphaMode(AlphaMode mode);
    void setDoubleSided(bool doubleSided);
    void setAlphaCutoff(float cutoff);
    void setEmissiveFactor(float, float, float);
    void setBaseColorFactor(float, float, float, float);
    void setMetallicFactor(float);
    void setRoughnessFactor(float);
    void setNormalScale(float);
    void setOcclusionStrength(float);

    AlphaMode alphaMode() const;
    bool doubleSided() const;
    float alphaCutoff() const;
    const std::array<float, 3>& emissiveFactor() const;
    const std::array<float, 4>& baseColorFactor() const;
    float metallicFactor() const;
    float roughnessFactor() const;
    float normalScale() const;
    float occlusionStrength() const;

private:
    // decomposed gltf attributes
    AlphaMode _alphaMode{AlphaMode::AM_OPAQUE};
    bool _doubleSided{false};
    float _alphaCutoff{0.5};
    std::array<float, 3> _emissiveFactor{0.0, 0.0, 0.0};
    std::array<float, 4> _baseColorFactor{0.0, 0.0, 0.0};
    float _metallicFactor{1.0};
    float _roughnessFactor{1.0};
    float _normalScale{1.0};
    float _occlusionStrength{1.0};
    std::array<std::string, static_cast<uint32_t>(TextureType::COUNT)> _pbrTextures;
};

} // namespace raum::scene