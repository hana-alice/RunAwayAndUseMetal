#include "Archive.h"
#include <fstream>
#include "cereal/access.hpp"
#include "cereal/archives/binary.hpp"
#include "log.h"


namespace raum::utils {

InputArchive::InputArchive(const std::filesystem::path& filePath) : InputArchive(filePath, std::ios::binary) {
}

InputArchive::InputArchive(const std::filesystem::path& filePath, std::ios::openmode stdFileMode) {
    is = std::ifstream(filePath.string(), stdFileMode);
    if (!is) {
        raum_error("Failed to open archive for reading: {}", filePath.string());
    }
    iarchive = std::make_unique<cereal::BinaryInputArchive>(is);
}


void InputArchive::read(uint8_t* data, uint32_t size) {
    auto& ar = *iarchive;
    ar(cereal::binary_data(data, size));
}

OutputArchive::OutputArchive(const std::filesystem::path& filePath) : OutputArchive(filePath, std::ios::binary | std::ios::trunc) {
}

OutputArchive::OutputArchive(const std::filesystem::path& filePath, std::ios::openmode stdFileMode) {
    const auto& parentPath = filePath.parent_path();
    if (!parentPath.empty() && !std::filesystem::exists(parentPath)) {
        std::filesystem::create_directories(parentPath);
    }
    os = std::ofstream(filePath.string(), stdFileMode);
    if (!os) {
        raum_error("Failed to open archive for writing: {}", filePath.string());
    }
    oarchive = std::make_unique<cereal::BinaryOutputArchive>(os);
}

void OutputArchive::write(const uint8_t* data, uint32_t size) {
    auto& ar = *oarchive;
    ar(cereal::binary_data(data, size));
}


}
