#pragma once
#include <filesystem>
#include <optional>

namespace raum::utils {

static std::optional<std::filesystem::path> s_resourcePath;

std::filesystem::path resourceDirectory() {
    if(s_resourcePath.has_value()) {
        return s_resourcePath.value();
    } else {
        return std::filesystem::current_path() / "resources";
    }
}

}