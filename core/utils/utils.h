#pragma once
#include <filesystem>
#include <functional>
#include <numbers>

#include <boost/container_hash/hash.hpp>

namespace raum::utils {

void setResourceDirectory(std::filesystem::path path);
std::filesystem::path resourceDirectory();

template <typename... Args>
class TickFunction {
public:
    TickFunction() = default;
    TickFunction(const TickFunction&) = default;
    TickFunction(TickFunction&&) = default;
    TickFunction& operator=(const TickFunction&) = default;
    TickFunction& operator=(TickFunction&&) = default;

    template <class F>
        requires(
            !std::same_as<std::decay_t<F>, TickFunction> &&
            std::constructible_from<std::function<void(Args...)>, F>)
    TickFunction(F&& f) : _tickFunc(std::forward<F>(f)) {}

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
