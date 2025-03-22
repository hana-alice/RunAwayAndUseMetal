#include <meshoptimizer.h>
#include <filesystem>
namespace raum::tools {

class MeshOpt {
public:

    struct Options {
        bool optimizeVertexCache{true};
        bool optimizeOverdraw{true};
        bool optimizeVertexFetch{true};
        bool verbose{false};
        uint32_t asyncFactor{0}; // 0 means auto
    };

    explicit MeshOpt(const Options&);
    ~MeshOpt() = default;

    MeshOpt(const MeshOpt&) = delete;
    MeshOpt& operator=(const MeshOpt&) = delete;
    MeshOpt(MeshOpt&&) = delete;

    void run(const std::filesystem::path& filePath);
private:
    Options _options;

};

}