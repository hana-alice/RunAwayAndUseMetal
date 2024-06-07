#pragma once
#include <map>
#include "RHIDefine.h"
#include "GraphTypes.h"
#include <filesystem>
#include "ShaderGraph.h"
namespace raum::graph{

void deserialize(const std::filesystem::path &path, std::string_view name, ShaderGraph& shaderGraph);

}