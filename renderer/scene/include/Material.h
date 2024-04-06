#pragma once
#include <boost/container/flat_map.hpp>
#include "RHIDefine.h"
#include "RHIImageView.h"
#include "RHIShader.h"
namespace raum::scene {

enum class TextureType : uint32_t {
    DIFFUSE,
    SPECULAR,
    AMBIENT,
    EMISSIVE,
    HEIGHT,
    NORMALS,
    SHININESS,
    OPACITY,
    DISPLACEMENT,
    LIGHTMAP,
    REFLECTION,

    BASE_COLOR,
    NORMAL_CAMERA,
    EMISSION_COLOR,
    METALNESS,
    DIFFUSE_ROUGHNESS,
    AMBIENT_OCCLUSION,

    SHEEN,
    CLEARCOAT,
    TRANSMISSION,

    COUNT,
};

struct Texture {
    std::string name;
    rhi::ImagePtr texture;
    rhi::ImageViewPtr textureView;
};

enum class MaterialType: uint8_t {
    PBR,
    NPR,
    CUSTOM,
};

class Material {
public:
    Material() = delete;
    Material(const std::string& shader);
    void add(const Texture& tex);
    const std::string& shaderName() const { return _shaderName; }

protected:
    MaterialType _type{MaterialType::PBR};
    std::vector<Texture> _textures;
    const std::string& _shaderName;
};

using MaterialPtr = std::shared_ptr<Material>;

class MaterialTemplate {
public:
    MaterialTemplate() = delete;
    MaterialTemplate(std::string_view shaderPath);
    MaterialPtr instantiate(std::string_view defines);
    MaterialPtr instantiate(MaterialType type);

private:
    std::string _shaderPath{};
};

using MaterialTemplatePtr = std::shared_ptr<MaterialTemplate>;

MaterialTemplatePtr getOrCreateMaterialTemplate(std::string_view shaderPath);

} // namespace raum::scene