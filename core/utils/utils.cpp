#include "utils.h"

#include <string_view>
#include <utility>

namespace raum::utils {
namespace {

std::filesystem::path s_resourceDirectory;

std::filesystem::path defaultResourceDirectory() {
#if defined(RAUM_DEFAULT_ASSET_DIR)
    constexpr std::string_view configuredDirectory{RAUM_DEFAULT_ASSET_DIR};
    if (!configuredDirectory.empty()) {
        return std::filesystem::path{configuredDirectory} / "files";
    }
#endif
    return std::filesystem::current_path() / "files";
}

} // namespace

void setResourceDirectory(std::filesystem::path path) {
    s_resourceDirectory = std::move(path);
}

std::filesystem::path resourceDirectory() {
    if (!s_resourceDirectory.empty()) {
        return s_resourceDirectory;
    }
    return defaultResourceDirectory();
}

} // namespace raum::utils
