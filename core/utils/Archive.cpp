// #include "Archive.h"
// #include <fstream>
//
// #include "cereal/access.hpp"
// #include "cereal/archives/binary.hpp"
// #include "core/utils/log.h"
//
//
// namespace raum::utils {
//
// using boost::edges;
// using boost::vertices;
//
// InputArchive::InputArchive(const std::filesystem::path& filePath) {
//     if (!std::filesystem::exists(filePath.parent_path())) {
//         std::filesystem::create_directories(filePath.parent_path());
//     }
//     is = std::ifstream(filePath.string(), std::ios::binary);
//     if (!is) {
//         raum_error("Could not find file: %s", filePath.string());
//     }
//     iarchive = std::make_shared<cereal::BinaryInputArchive>(is);
// }
//

//
// OutputArchive::OutputArchive(const std::filesystem::path& filePath) {
//     if (!std::filesystem::exists(filePath.parent_path())) {
//         std::filesystem::create_directories(filePath.parent_path());
//     }
//     os = std::ofstream(filePath.string(), std::ios::binary | std::ios::trunc);
//     if (!os) {
//         raum_error("Failed to open file: %s", filePath.string());
//     }
//     oarchive = std::make_shared<cereal::BinaryOutputArchive>(os);
// }
//
// void OutputArchive::write(const graph::SceneGraph& sg) {
//
// }
//

//
//
// } // namespace raum::asset::serialize