#pragma once
#include <filesystem>
#include "cereal/cereal.hpp"
#include "ArchiveTypes.h"
#include <fstream>

namespace raum::utils {

class InputArchive {
public:
    InputArchive() = default;
    explicit InputArchive(const std::filesystem::path& filePath);
    explicit InputArchive(const std::filesystem::path& filePath, std::ios::openmode stdFileMode);

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

    void read(uint8_t* data, uint32_t size);

private:
    std::shared_ptr<cereal::BinaryInputArchive> iarchive;
    std::ifstream is;
};

class OutputArchive {
public:
    OutputArchive() = default;
    explicit OutputArchive(const std::filesystem::path& filePath);
    explicit OutputArchive(const std::filesystem::path& filePath, std::ios::openmode stdFileMode);

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

    void write(const uint8_t* data, uint32_t size);

private:
    std::shared_ptr<cereal::BinaryOutputArchive> oarchive;
    std::ofstream os;
};

}