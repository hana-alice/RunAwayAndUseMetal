#pragma once
#include <filesystem>
#include "SceneGraph.h"
#include "cereal/cereal.hpp"
#include "ArchiveTypes.h"
#include <fstream>

namespace raum::asset::serialize {

class InputArchive {
public:
    InputArchive() = default;
    explicit InputArchive(const std::filesystem::path& filePath);

    template <typename T>
    InputArchive& operator>>(T&& arg) {
        (*iarchive)(std::forward<T>(arg));
        return *this;
    }

    template <typename... Args>
    void operator()(Args&&... args) {
        (*iarchive)(std::forward<Args>(args)...);
    }

    template <>
    InputArchive& operator>>(graph::SceneGraph& arg) {
        read(arg);
    }

    void read(graph::SceneGraph& arg);

    void read(const uint8_t* data, size_t size);

private:
    std::shared_ptr<cereal::BinaryInputArchive> iarchive;
    std::ifstream is;
};

class OutputArchive {
public:
    OutputArchive() = default;
    explicit OutputArchive(const std::filesystem::path& filePath);

    template <typename T>
    OutputArchive& operator<<(const T& arg) {
        (*oarchive)(arg);
        return *this;
    }

    template <typename... Args>
    void operator()(Args&&... args) {
        (*oarchive)(std::forward<Args>(args)...);
    }

    template <>
    OutputArchive& operator<<(const graph::SceneGraph& arg) {
        write(arg);
    }

    template <>
    OutputArchive& operator<<(const scene::Mesh& arg) {
        write(arg);
    }

    void write(const graph::SceneGraph& arg);
    void write(const uint8_t* data, uint32_t size);
    void write(const scene::Mesh& arg);

private:
    std::shared_ptr<cereal::BinaryOutputArchive> oarchive;
    std::ofstream os;
};

} // namespace raum::asset::serialize