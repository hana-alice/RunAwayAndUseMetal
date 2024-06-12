#include "Serialization.h"
#include <boost/json/src.hpp>
#include <fstream>
#include "RHIUtils.h"
#include "boost/algorithm/string.hpp"
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"

namespace raum::graph {
using BindingMap = std::array<std::map<uint32_t, std::string>, rhi::BindingRateCount>;
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
        raum_error("{} is not a valid usage.", 1);
    }
    buffer.type = bufferBindingMap.at(usageStr);
    buffer.count = obj.at("count").to_number<uint32_t>();

    const auto& eleData = obj.at("elements").as_array();
    auto& elements = buffer.elements;
    elements.reserve(eleData.size());
    for(const auto& ele : eleData) {
        const auto& typeStr = ele.at("type").as_string();
        rhi::DataType type{rhi::DataType::UNKNOWN};
        if(!typeStr.empty()) {
            type = rhi::str2type.at(typeStr);
        }
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
        raum_error("{} is not a valid usage.", usageStr.c_str());
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

void deserializeBinding(const object& obj, const rhi::ShaderStage stage, ShaderResource& resource, const BindingMap& bindingMap) {
    if(!obj.contains("bindings")) {
        return;
    }
    raum_check(obj.at("bindings").is_array(), "layout parsing error: bindings not written in array");
    const auto& bindings = obj.at("bindings").as_array();
    for(const auto& binding : bindings) {
        auto rate = value_to<Rate>(binding);
        const auto& slotValue = binding.at("slot");
        auto slot = slotValue.to_number<uint32_t>();
        const auto& name = bindingMap[static_cast<uint32_t>(rate)].at(slot);
        auto& resDesc = resource.bindings[name.data()];
        resDesc.binding = slot;
        resDesc.visibility = resDesc.visibility | stage;
        resDesc.type = value_to<BindingType>(binding);
        resDesc.rate = rate;
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

const std::filesystem::path deserialize(const std::filesystem::path &path, ShaderResource& resource, const BindingMap& bindingMap) {
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
    if(data.contains("mesh")) {
        const auto& comp = data.at("mesh");
        deserializeBinding(comp.as_object(), rhi::ShaderStage::MESH, resource, bindingMap);
    }
    if(data.contains("task")) {
        const auto& comp = data.at("task");
        deserializeBinding(comp.as_object(), rhi::ShaderStage::TASK, resource, bindingMap);
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

//void reflectBuffer(const std::string& source, BindingMap& bindingMap) {
//    const char* pattern = R"(layout\(set\s*=\s*(\d)\s*,\s*binding\s*=\s*(\d+)[^;{]+(\{[^}]+\})\s*(\w+)\s*;)";
//    boost::regex reg(pattern);
//
//    boost::cregex_iterator it(source.data(), source.data() + source.length(), reg);
//    boost::cregex_iterator end;
//
//    for (; it != end; ++it) {
//        std::string_view matchStr((*it)[0].begin(), (*it)[0].end());
//        const char* setPatt = R"(set\s*=\s*\d)";
//        boost::regex setReg(setPatt);
//        boost::cregex_iterator setIt(matchStr.data(), matchStr.data() + matchStr.length(), setReg);
//
//        std::string_view bindingStr((*it)[2].begin(), (*it)[2].end());
//        std::string_view nameStr((*it)[4].begin(), (*it)[4].end());
//        std::string_view setStr((*it)[1].begin(), (*it)[1].end());
//        auto set = boost::lexical_cast<uint32_t>(setStr);
//        auto binding = boost::lexical_cast<uint32_t>(bindingStr);
//        bindingMap[set].emplace(binding, nameStr);
//    }
//}

void reflect(const std::string& source, BindingMap& bindingMap) {
    const char* pattern = R"(\s*layout\s*\([^\)]*binding\s*=\s(\d+)\).*?(\w+)\s*[{;])";
    boost::regex reg(pattern);

    boost::sregex_iterator it(source.begin(), source.end(), reg);
    boost::sregex_iterator end;

    for (; it != end; ++it) {
        std::string_view matchStr((*it)[0].begin(), (*it)[0].end());
        const char* setPatt = R"(set\s*=\s*(\d))";
        boost::regex setReg(setPatt);
        boost::cregex_iterator setIt(matchStr.data(), matchStr.data() + matchStr.length(), setReg);
        auto setStr = std::string_view((*setIt)[1].begin(), (*setIt)[1].end());
        //if (setIt == end) continue; // skip if not set 0

        std::string_view bindingStr((*it)[1].begin(), (*it)[1].end());
        std::string_view nameStr((*it)[2].begin(), (*it)[2].end());
        auto set = boost::lexical_cast<uint32_t>(setStr);
        auto binding = boost::lexical_cast<uint32_t>(bindingStr);
        bindingMap[set].emplace(binding, nameStr);
    }
}

BindingMap reflect(const std::map<std::string, std::string>& sources) {
    BindingMap bindingMap;
    for(auto source : sources) {
        reflect(source.second, bindingMap);
    }
    return bindingMap;
}

std::unordered_map<std::string_view, rhi::ShaderStage> str2ShaderStage = {
    {".vert", rhi::ShaderStage::VERTEX},
    {".frag", rhi::ShaderStage::FRAGMENT},
    {".comp", rhi::ShaderStage::COMPUTE},
    {".mesh", rhi::ShaderStage::MESH},
    {".task", rhi::ShaderStage::TASK},
};

void deserialize(const std::filesystem::path &path, std::string_view name, ShaderGraph& shaderGraph) {
    auto layoutPath = (path / name ).concat(".layout");
    assert(exists(layoutPath));

    auto shaderSrc = loadResource(path, name);
    const auto& bindingMap = reflect(shaderSrc);
    
    ShaderResource resource{};
    const auto& logicPath = deserialize(layoutPath, resource, bindingMap);
    for(auto& src : shaderSrc) {
        resource.shaderSources.emplace(str2ShaderStage.at(src.first), std::move(src.second));
    }
    shaderGraph.addVertex(logicPath, std::move(resource));
    
}
}
