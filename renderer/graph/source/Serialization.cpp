#include "Serialization.h"
#include <boost/json/src.hpp>
#include <fstream>
#include "RHIUtils.h"
#include "boost/algorithm/string.hpp"
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"

namespace raum::graph {

using namespace ::boost::json;

std::unordered_map<std::string_view, rhi::DescriptorType, hash_string, std::equal_to<>> bufferBindingMap = {
    {"uniform", rhi::DescriptorType::UNIFORM_BUFFER},
    {"storage", rhi::DescriptorType::STORAGE_BUFFER},
    {"dynamic_uniform", rhi::DescriptorType::UNIFORM_BUFFER_DYNAMIC},
    {"dynamic_storage", rhi::DescriptorType::STORAGE_BUFFER_DYNAMIC},
    {"uniform_texel", rhi::DescriptorType::UNIFORM_TEXEL_BUFFER},
    {"storage_texel", rhi::DescriptorType::STORAGE_TEXEL_BUFFER},
};

std::unordered_map<std::string_view, rhi::DescriptorType, hash_string, std::equal_to<>> imageBindingMap = {
    {"sampled", rhi::DescriptorType::SAMPLED_IMAGE},
    {"storage", rhi::DescriptorType::STORAGE_IMAGE},
    {"input", rhi::DescriptorType::INPUT_ATTACHMENT},
};

std::unordered_map<std::string_view, Rate, hash_string, std::equal_to<>> rateMap = {
    {"per_pass", Rate::PER_PASS},
    {"per_batch", Rate::PER_BATCH},
    {"per_instance", Rate::PER_INSTANCE},
    {"per_draw", Rate::PER_DRAW},
};

BufferBinding tag_invoke( value_to_tag<BufferBinding>, value const& jv ) {
    object const& obj = jv.as_object();
    BufferBinding buffer{};
    const auto& usageStr = obj.at("usage").as_string();
    if(!bufferBindingMap.contains(usageStr)) {
        spdlog::error("{} is not a valid usage.", usageStr.c_str());
        error("{} is not a valid usage.", 1);
    }
    buffer.type = bufferBindingMap.at(usageStr);
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
    if(!imageBindingMap.contains(usageStr)) {
        error("{} is not a valid usage.", usageStr.c_str());
    }

    image.type = imageBindingMap.at(usageStr);
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

Rate tag_invoke( value_to_tag<Rate>, value const& jv ) {
    object const& obj = jv.as_object();
    Rate rate{Rate::PER_PASS};
    const auto& rateStr = obj.at("rate").as_string();
    if(rateStr == "per_pass") {
        rate = Rate::PER_PASS;
    } else if(rateStr == "per_batch") {
        rate = Rate::PER_BATCH;
    } else if(rateStr == "per_instance") {
        rate = Rate::PER_INSTANCE;
    } else if(rateStr == "per_draw") {
        rate = Rate::PER_DRAW;
    }
    return rate;
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
        resDesc.rate = value_to<Rate>(binding);
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

const std::filesystem::path deserialize(const std::filesystem::path &path, ShaderResource& resource, const std::map<uint32_t, std::string>& bindingMap) {
    raum_check(std::filesystem::exists(path), "failed to read file!");
    std::ifstream f(path);
    const auto& raw = parse(f);
    const auto& data = raw.as_object();

    if(!data.contains("path")) {
        raum_check(false, "layout doesn't contains a valid path.");
    }

    const auto& pathID = data.at("path").as_string();
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

std::map<std::string, std::string> loadResource(std::filesystem::path dir, std::string_view name) {
    std::map<std::string, std::string> shaderSrc;
    for(auto ext : {".vert", ".frag", ".comp", ".mesh", ".task"}) {
        auto fp = (dir / name).concat(ext);
        if(exists(fp)) {
            std::ifstream in(fp);
            std::stringstream buffer;
            buffer << in.rdbuf();
            shaderSrc.emplace(ext, buffer.str());
        }
    }
    return shaderSrc;
}

void reflect(const std::string& source, std::map<uint32_t, std::string>& bindingMap) {
    const char* pattern = R"(\s*layout\([^\)]*binding\s*=\s(\d+)\).*?(\w+)\s*[{;])";
    boost::regex reg(pattern);

    boost::sregex_iterator it(source.begin(), source.end(), reg);
    boost::sregex_iterator end;

    for (; it != end; ++it) {
        std::string matchStr((*it)[0].begin(), (*it)[0].end());
        const char* setPatt = R"(set\s*=\s*0)";
        boost::regex setReg(setPatt);
        boost::sregex_iterator setIt(matchStr.begin(), matchStr.end(), setReg);
        if (setIt == end) continue; // skip if not set 0

        std::string_view bindingStr((*it)[1].begin(), (*it)[1].end());
        std::string_view nameStr((*it)[2].begin(), (*it)[2].end());
        auto binding = boost::lexical_cast<uint32_t>(bindingStr);
        bindingMap.emplace(binding, nameStr);
    }
}

std::map<uint32_t, std::string> reflect(const std::map<std::string, std::string>& sources) {
    std::map<uint32_t, std::string> bindingMap;
    for(auto source : sources) {
        reflect(source.second, bindingMap);
    }
    return bindingMap;
}


void deserialize(const std::filesystem::path &path, std::string_view name, ShaderGraph& shaderGraph) {
    auto layoutPath = (path / name ).concat(".layout");
    assert(exists(layoutPath));

    auto shaderSrc = loadResource(path, name);
    const auto& bindingMap = reflect(shaderSrc);
    {
        ShaderResource resource{};
        const auto& logicPath = deserialize(layoutPath, resource, bindingMap);
        for(auto& src : shaderSrc) {
            resource.shaderSources.emplace(logicPath.string() + src.first, std::move(src.second));
        }
        shaderGraph.addVertex(logicPath, std::move(resource));
    }
}
}
