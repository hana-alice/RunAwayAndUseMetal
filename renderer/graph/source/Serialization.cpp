#include "Serialization.h"
#include <boost/json/src.hpp>
#include <fstream>
#include "RHIUtils.h"
#include "boost/algorithm/string.hpp"

namespace raum::graph {

using namespace ::boost::json;

BufferBinding tag_invoke( value_to_tag<BufferBinding>, value const& jv ) {
    object const& obj = jv.as_object();
    BufferBinding buffer{};
    const auto& usageStr = obj.at("usage").as_string();
    auto& usage = buffer.usage;
    if(usageStr == "uniform") {
        buffer.usage = rhi::BufferUsage::UNIFORM;
    } else if(usageStr == "storage") {
        buffer.usage = rhi::BufferUsage::STORAGE;
    } else {
        raum_check(false, "unsupport buffer binding type.");
    }
    buffer.count = obj.at("count").to_number<uint32_t>();

    const auto& eleData = obj.at("elements").as_array();
    auto& elements = buffer.elements;
    elements.reserve(eleData.size());
    for(const auto& ele : eleData) {
        const auto& typeStr = ele.at("type").as_string();
        auto type = rhi::str2type.at(typeStr);
        uint32_t count = ele.at("count").to_number<uint32_t>();
        elements.emplace_back(type, count);
    }
    return buffer;
}

ImageBinding tag_invoke( value_to_tag<ImageBinding>, value const& jv ) {
    object const& obj = jv.as_object();
    ImageBinding image{};
    const auto& usageStr = obj.at("usage").as_string();
    auto& usage = image.usage;
    if(usageStr == "sample") {
        image.usage = rhi::ImageUsage::SAMPLED;
    } else if(usageStr == "storage") {
        image.usage = rhi::ImageUsage::STORAGE;
    } else if(usageStr == "shading_rate") {
        image.usage = rhi::ImageUsage::SHADING_RATE;
    } else if(usageStr == "input_color") {
        image.usage = rhi::ImageUsage::INPUT_ATTACHMENT | rhi::ImageUsage::COLOR_ATTACHMENT;
    } else if(usageStr == "input_depth") {
        image.usage = rhi::ImageUsage::INPUT_ATTACHMENT | rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT;
    } else {
        raum_check(false, "unsupport image binding type.");
    }
    image.arraySize = obj.at("count").to_number<uint32_t>();

    const auto& imageTypeStr = obj.at("type").as_string();
    if(imageTypeStr == "1d") {
        image.imageType = rhi::ImageType::IMAGE_1D;
    } else if(imageTypeStr == "2d") {
        image.imageType = rhi::ImageType::IMAGE_2D;
    } else if(imageTypeStr == "3d") {
        image.imageType = rhi::ImageType::IMAGE_3D;
    } else {
        raum_check(false, "unsupport image type.");
    }

    if(obj.contains("format")) {
        const auto& formatStr = obj.at("format").as_string();
        image.format = rhi::str2format.at(formatStr);
    }

    return image;
}

SamplerBinding tag_invoke( value_to_tag<SamplerBinding>, value const& jv ) {
    object const& obj = jv.as_object();
    SamplerBinding sampler{};
    sampler.count = obj.at("count").to_number<uint32_t>();
    sampler.immutable = obj.at("immutable").as_bool();
    return sampler;
}

BindingType tag_invoke( value_to_tag<BindingType>, value const& jv ) {
    object const& obj = jv.as_object();
    BindingType bt{BindingType::BUFFER};
    const auto& resStr = obj.at("resource").as_string();
    if(resStr == "buffer") {
        bt = BindingType::BUFFER;
    } else if(resStr == "image") {
        bt = BindingType::IMAGE;
    } else if(resStr == "sampler") {
        bt = BindingType::SAMPLER;
    } else if(resStr == "bindless") {
        bt = BindingType::BINDLESS;
    }
    return bt;
}

void deserializeBinding(const object& obj, const rhi::ShaderStage stage, ShaderResource& resource, const std::map<uint32_t, std::string>& bindingMap) {
    if(!obj.contains("bindings")) {
        return;
    }
    raum_check(obj.at("bindings").is_array(), "layout parsing error: bindings not written in array");
    const auto& bindings = obj.at("bindings").as_array();
    for(const auto& binding : bindings) {
        const auto& slotValue = binding.at("slot");
        auto slot = slotValue.to_number<uint32_t>();
        const auto& name = bindingMap.at(slot);
        auto& resDesc = resource.bindings[name];
        resDesc.binding = slot;
        resDesc.visibility = resDesc.visibility | stage;
        resDesc.type = value_to<BindingType>(binding);
        switch (resDesc.type) {
            case BindingType::BUFFER:
                resDesc.buffer = value_to<BufferBinding>(binding);
                break;
            case BindingType::IMAGE:
                resDesc.image = value_to<ImageBinding>(binding);
                break;
            case BindingType::SAMPLER:
                resDesc.sampler = value_to<SamplerBinding>(binding);
                break;
            default:
                raum_check(false, "unsupport binding type");
                break;
        }
    }
}

const std::filesystem::path deserialize(const std::filesystem::path &path, ShaderResources& resources, const std::map<uint32_t, std::string>& bindingMap) {
    raum_check(std::filesystem::exists(path), "failed to read file!");
    std::ifstream f(path);
    const auto& raw = parse(f);
    const auto& data = raw.as_object();

    if(!data.contains("path")) {
        raum_check(false, "layout doesn't contains a valid path.");
    }

    const auto& pathID = data.at("path").as_string();
    auto& resource = resources[pathID.c_str()];

    if(data.contains("vertex")) {
        const auto& vert = data.at("vertex");
        deserializeBinding(vert.as_object(), rhi::ShaderStage::VERTEX, resource, bindingMap);
    }
    if(data.contains("fragment")) {
        const auto& frag = data.at("fragment");
        deserializeBinding(frag.as_object(), rhi::ShaderStage::FRAGMENT, resource, bindingMap);
    }
    if(data.contains("compute")) {
        const auto& comp = data.at("compute");
        deserializeBinding(comp.as_object(), rhi::ShaderStage::COMPUTE, resource, bindingMap);
    }

    return std::filesystem::path(pathID.c_str());
}
}
