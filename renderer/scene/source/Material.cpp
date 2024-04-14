#include "Material.h"
#include "PBRMaterial.h"
#include "RHIDevice.h"
#include "RHIUtils.h"
namespace raum::scene {

static std::unordered_map<std::string, MaterialTemplatePtr, hash_string, std::equal_to<>> materialTemplates;

MaterialTemplatePtr getOrCreateMaterialTemplate(std::string_view shaderPath) {
    if (!materialTemplates.contains(shaderPath)) {
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
    raum_unreachable();
    return nullptr;
}

static uint32_t materialID = 0;

Material::Material(const std::string &shader) : _shaderName(shader) {
}

void Material::add(const Texture &tex) {
    _textures.emplace_back(tex);
    _dirty = true;
}

void Material::add(const raum::scene::Buffer &buf) {
    _buffers.emplace_back(buf);
    _dirty = true;
}

void Material::initBindGroup(
    const boost::container::flat_map<std::string_view, uint32_t> &bindings,
    rhi::DescriptorSetLayoutPtr layout,
    rhi::DevicePtr device) {
    _bindGroup = std::make_shared<BindGroup>(bindings, layout, device);
}

void Material::update() {
    if (_dirty) {
        for (const auto &texture : _textures) {
            _bindGroup->bindImage(texture.name, 0, texture.textureView, rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        }
        for(const auto& buffer : _buffers) {
            _bindGroup->bindBuffer(buffer.name, 0, buffer.buffer);
        }
        _bindGroup->update();
    }
    _dirty = false;
}

} // namespace raum::scene