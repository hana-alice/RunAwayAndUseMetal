#pragma once
#include <map>
#include "RHIDefine.h"
#include "GraphTypes.h"
#include <filesystem>
namespace raum::graph{
const std::filesystem::path deserialize(const std::filesystem::path &path, ShaderResources& resources, const std::map<uint32_t, std::string>& bindingMap);
}