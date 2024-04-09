#include "Material.h"
#include "PBRMaterial.h"
namespace raum::scene {

static std::unordered_map<std::string, MaterialTemplatePtr, hash_string, std::equal_to<>> materialTemplates;

MaterialTemplatePtr getOrCreateMaterialTemplate(std::string_view shaderPath) {
    if(!materialTemplates.contains(shaderPath)) {
        materialTemplates.emplace(shaderPath, std::make_shared<MaterialTemplate>(shaderPath));
    }
    return materialTemplates.at(shaderPath.data());
}

MaterialTemplate::MaterialTemplate(std::string_view shaderPath) {
    _shaderPath = shaderPath;
}

MaterialPtr MaterialTemplate::instantiate(std::string_view defines) {
    return std::make_shared<Material>(_shaderPath);
}

MaterialPtr MaterialTemplate::instantiate(MaterialType type) {
    switch (type) {
        case MaterialType::PBR:
            return std::make_shared<PBRMaterial>(_shaderPath);
        case MaterialType::NPR:
        case MaterialType::CUSTOM:
            return std::make_shared<Material>(_shaderPath);
    }
    return nullptr;
    raum_unreachable();
}

static uint32_t materialID = 0;

Material::Material(const std::string &shader): _shaderName(shader) {
}

void Material::add(const Texture &tex) {
    _textures.emplace_back(tex);
}

void Material::add(const raum::scene::Buffer &buf) {
    _buffers.emplace_back(buf);
}

rhi::DescriptorSetPtr Material::descriptorSet() {
    return _descriptorSet;
}

}