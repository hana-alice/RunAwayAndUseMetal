#include "Loader.h"
#include <filesystem>
#include "log.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
namespace raum::framework::asset {
ModelLoader::ModelLoader(const char* filePath) {
    std::filesystem::path fpath(filePath);
    RAUM_ERROR_IF(!std::filesystem::exists(fpath), "Invalid model path");

    tinyobj::ObjReaderConfig rConfig;
    rConfig.mtl_search_path = "";
    tinyobj::ObjReader reader;

    if (reader.ParseFromFile(filePath, rConfig)) {
        RAUM_WARN_IF(!reader.Warning().empty(), reader.Warning().c_str());

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();
        //

    } else {
        RAUM_ERROR("error loading model");
    }
}

ModelLoader::~ModelLoader() {
}
} // namespace raum::framework::asset