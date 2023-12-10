#pragma once
namespace raum::framework::asset {

class Loader {};

class ModelLoader : public Loader {
public:
    ModelLoader() = delete;
    ModelLoader(const char *filePath);
    ~ModelLoader();
};
} // namespace raum::framework::asset