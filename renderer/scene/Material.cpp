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

void MaterialTemplate::addDefine(std::string_view define) {
    _defines.insert(define.data());
}

MaterialPtr MaterialTemplate::instantiate(std::string_view name, MaterialType type) {
    switch (type) {
        case MaterialType::PBR:
            return std::make_shared<PBRMaterial>(name, _shaderPath, type, _defines);
        case MaterialType::NPR:
        case MaterialType::CUSTOM:
            return std::make_shared<Material>(name, _shaderPath, type, _defines);
    }
    raum_unreachable();
    return nullptr;
}

static uint32_t materialID = 0;

Material::Material(std::string_view matName,
                   const std::string &shader,
                   const MaterialType materialType,
                   const std::set<std::string> &defines)
: _shaderName(shader), _matName(matName), _defines(defines), _type(materialType) {
}

void Material::set(std::string_view name, const Texture &tex) {
    _textures[name.data()] = tex;
    _dirty = true;
}

void Material::set(std::string_view name, const raum::scene::Buffer &buf) {
    _buffers[name.data()] = buf;
    _dirty = true;
}

void Material::set(std::string_view name, const Sampler &info) {
    _samplers[name.data()] = info;
    _dirty = true;
}

void Material::initBindGroup(
    const SlotMap &bindings,
    rhi::DescriptorSetLayoutPtr layout,
    rhi::DevicePtr device) {
    _bindGroup = std::make_shared<BindGroup>(bindings, layout, device);
}

void Material::update() {
    if (_dirty) {
        for (const auto &[name, texture] : _textures) {
            _bindGroup->bindImage(name, 0, texture.textureView, rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        }
        for (const auto &[name, buffer] : _buffers) {
            _bindGroup->bindBuffer(name, 0, buffer.buffer);
        }
        for (const auto &[name, sampler] : _samplers) {
            _bindGroup->bindSampler(name, 0, sampler.info);
        }
        _bindGroup->update();
        _dirty = false;
    }
}

BindGroupPtr Material::bindGroup() {
    return _bindGroup;
}

} // namespace raum::scene