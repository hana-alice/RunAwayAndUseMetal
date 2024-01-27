#pragma once
#include <map>
#include "RHIDefine.h"
#include "GraphTypes.h"
#include <filesystem>
namespace raum::graph{
    void deserialize(const std::filesystem::path &path, std::unordered_map<std::string, ShaderResource, hash_string, std::equal_to<>>& resources, const std::map<uint32_t, std::string>& bindingMap);
}