#pragma once
#include "Material.h"

namespace raum::scene {

class PBRMaterial : public Material {
public:
    using Material::Material;

    void setDiffuse(std::string_view slot);

    void set(TextureType type, std::string_view slot);

private:
    std::array<std::string, static_cast<uint32_t>(TextureType::COUNT)> pbrTextures;
};

} // namespace raum::scene