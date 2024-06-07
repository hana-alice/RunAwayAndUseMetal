#pragma once
#include <boost/container/flat_map.hpp>
#include "BindGroup.h"
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

struct Buffer {
    std::string name;
    rhi::BufferPtr buffer;
};

struct Sampler {
    std::string name;
    rhi::SamplerInfo info;
};

enum class MaterialType : uint8_t {
    PBR,
    NPR,
    CUSTOM,
};

class Material {
public:
    Material() = delete;
    Material(const std::string& shader);
    void add(const Texture& tex);
    void add(const Buffer& buf);
    void add(const Sampler& info);
    const std::string& shaderName() const { return _shaderName; }

    void initBindGroup(
        const boost::container::flat_map<std::string_view,uint32_t>& bindings,
        rhi::DescriptorSetLayoutPtr layout,
        rhi::DevicePtr device);

    void update();
    BindGroupPtr bindGroup();

protected:
    MaterialType _type{MaterialType::PBR};
    std::vector<Texture> _textures;
    std::vector<Buffer> _buffers;
    std::vector<Sampler> _samplers;
    const std::string& _shaderName;
    BindGroupPtr _bindGroup;
    bool _dirty{false};
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