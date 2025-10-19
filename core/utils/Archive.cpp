#include "Archive.h"
#include <fstream>
#include "cereal/access.hpp"
#include "cereal/archives/binary.hpp"
#include "core/utils/log.h"


namespace raum::utils {

InputArchive::InputArchive(const std::filesystem::path& filePath) : InputArchive(filePath, std::ios::binary) {
}

InputArchive::InputArchive(const std::filesystem::path& filePath, std::ios::openmode stdFileMode) {
    if (!std::filesystem::exists(filePath.parent_path())) {
        std::filesystem::create_directories(filePath.parent_path());
    }
    is = std::ifstream(filePath.string(), stdFileMode);
    if (!is) {
        raum_error("Could not find file: {}", filePath.string());
    }
    iarchive = std::make_shared<cereal::BinaryInputArchive>(is);
}


void InputArchive::read(uint8_t* data, uint32_t size) {
    auto& ar = *iarchive;
    ar(cereal::binary_data(data, size));
}

OutputArchive::OutputArchive(const std::filesystem::path& filePath) : OutputArchive(filePath, std::ios::binary | std::ios::trunc) {
}

OutputArchive::OutputArchive(const std::filesystem::path& filePath, std::ios::openmode stdFileMode) {
    if (!std::filesystem::exists(filePath.parent_path())) {
        std::filesystem::create_directories(filePath.parent_path());
    }
    os = std::ofstream(filePath.string(), stdFileMode);
    if (!os) {
        raum_error("Failed to open file: %s", filePath.string());
    }
    oarchive = std::make_shared<cereal::BinaryOutputArchive>(os);
}

void OutputArchive::write(const uint8_t* data, uint32_t size) {
    auto& ar = *oarchive;
    ar << cereal::binary_data(data, size);
}


}