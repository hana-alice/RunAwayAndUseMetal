#pragma once
#include <filesystem>
#include <optional>

namespace raum::utils {

static std::optional<std::filesystem::path> s_resourcePath;

static std::filesystem::path resourceDirectory() {
    if (s_resourcePath.has_value()) {
        return s_resourcePath.value();
    } else {
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

} // namespace raum::utils