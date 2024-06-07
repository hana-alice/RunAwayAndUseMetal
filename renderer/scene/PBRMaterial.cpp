#include "PBRMaterial.h"

namespace raum::scene {

void PBRMaterial::setDiffuse(std::string_view name) {
    pbrTextures[static_cast<uint32_t>(TextureType::DIFFUSE)] = name;
}

void PBRMaterial::set(raum::scene::TextureType type, std::string_view slot) {
    pbrTextures[static_cast<uint32_t>(type)] = slot;
    _textures[static_cast<uint32_t>(type)].name = slot;
}

} // namespace raum::scene