#pragma once
#include <filesystem>
// #include "SceneGraph.h"
#include "cereal/cereal.hpp"
#include "ArchiveTypes.h"
#include <fstream>

namespace raum::utils {

template<class... Args>
void func(Args&&... args) {
}

template<>
void func(int a) {
}

class InputArchive {
public:
    InputArchive() = default;
    explicit InputArchive(const std::filesystem::path& filePath);

    template <typename T>
    InputArchive& operator>>(T&& arg) {
        read(std::forward<T&&>(arg));
        return *this;
    }

    template <typename... Args>
    void operator()(Args&&... args) {
        read(std::forward<Args&&>(args)...);
    }

    template <typename ...Args>
    void read(Args&&... args) {;
        (*iarchive)(std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<cereal::BinaryInputArchive> iarchive;
    std::ifstream is;
};

// template <>
// inline void InputArchive::read(uint8_t* data, uint32_t size) {
//     auto& ar = *iarchive;
//     ar(cereal::binary_data(data, size));
// }

} // namespace raum::asset::serialize

class OutputArchive {
public:
    OutputArchive() = default;
    explicit OutputArchive(const std::filesystem::path& filePath);

    template <typename T>
    OutputArchive& operator<<(const T& arg) {
        write(arg);
        return *this;
    }

    template <typename... Args>
    void operator()(Args&&... args) {
        write(std::forward<Args>(args)...);
    }

    template <typename ...Args>
    void write(Args&&... args) {
        (*oarchive)(std::forward<Args>(args)...);
    }

private:
    std::shared_ptr<cereal::BinaryOutputArchive> oarchive;
    std::ofstream os;
};

template <>
inline void OutputArchive::write(const uint8_t* data, uint32_t size) {
    auto& ar = *oarchive;
    ar << cereal::binary_data(data, size);
}

} // namespace raum::asset::serialize