#pragma once
#include <filesystem>
#include <optional>
#include <numbers>

namespace raum::utils {

static std::optional<std::filesystem::path> s_resourcePath;

static std::filesystem::path resourceDirectory() {
    if (s_resourcePath.has_value()) {
        return s_resourcePath.value();
    } else {
#if defined(RAUM_DEFAULT_ASSET_DIR)
        return std::filesystem::path(RAUM_DEFAULT_ASSET_DIR) / "files";
#endif //  defined(RAUM_DEFAULT_ASSET_DIR)
        // Debug/Release
        return std::filesystem::current_path() / "files";
    }
}

template <typename... Args>
class TickFunction {
public:
    TickFunction() = default;
    TickFunction(std::function<void(Args... args)>&& tickFunc) {
        _tickFunc = tickFunc;
    }

    void operator()(Args... args) {
        _tickFunc(std::forward<Args>(args)...);
    }

private:
    std::function<void(Args... args)> _tickFunc;
};

struct Degree {
    float value{0.0};
};

struct Radian {
    float value{0.0};
};

inline Radian toRadian(const Degree& deg) {
    return Radian{deg.value * static_cast<float>(std::numbers::pi) / 180.0f};
};

inline Degree toDegree(const Radian& radian) {
    return Degree{radian.value * 180.0f / static_cast<float>(std::numbers::pi)};
}

} // namespace raum::utils