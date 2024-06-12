#pragma once
#include <boost/container/flat_map.hpp>
#include <set>
#include "BindGroup.h"
namespace raum::scene {

enum class TextureType : uint32_t {
    BASE_COLOR,
    NORMAL,
    EMISSIVE,
    METALLIC_ROUGHNESS,
    AO,

    COUNT,
};

struct Texture {
    std::string name;
    rhi::ImagePtr texture;
    rhi::ImageViewPtr textureView;
    uint32_t uvIndex{0}; // indicates which set of uv is in use.
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
    Material(std::string_view matName,
             const std::string& shader,
             const std::set<std::string>& defines);
    void add(const Texture& tex);
    void add(const Buffer& buf);
    void add(const Sampler& info);
    const std::string& shaderName() const { return _shaderName; }
    const std::set<std::string>& defines() const { return _defines; }

    void initBindGroup(
        const boost::container::flat_map<std::string_view, uint32_t>& bindings,
        rhi::DescriptorSetLayoutPtr layout,
        rhi::DevicePtr device);

    void update();
    BindGroupPtr bindGroup();

protected:
    bool _dirty{false};
    MaterialType _type{MaterialType::PBR};
    std::vector<Texture> _textures;
    std::vector<Buffer> _buffers;
    std::vector<Sampler> _samplers;
    const std::string _shaderName;
    const std::string _matName;
    BindGroupPtr _bindGroup;
    const std::set<std::string> _defines;
};

using MaterialPtr = std::shared_ptr<Material>;

class MaterialTemplate {
public:
    MaterialTemplate() = delete;
    MaterialTemplate(std::string_view shaderPath);
    void addDefine(std::string_view define);
    MaterialPtr instantiate(std::string_view name, MaterialType type);

private:
    std::string _shaderPath{};
    std::set<std::string> _defines;
};

using MaterialTemplatePtr = std::shared_ptr<MaterialTemplate>;

MaterialTemplatePtr getOrCreateMaterialTemplate(std::string_view shaderPath);

} // namespace raum::scene