#include <array>
#include <cstdlib>
#include <filesystem>
#include <optional>

#include <QApplication>
#include <QFont>
#include <QMessageBox>

#include "core/utils/utils.h"
#include "ui.h"

namespace {

bool isResourceDirectory(const std::filesystem::path& path) {
    std::error_code error;
    return std::filesystem::is_directory(path / "shader", error) &&
           std::filesystem::is_directory(path / "models", error) &&
           std::filesystem::is_regular_file(path / "models" / "sponza" / "sponza.gltf", error);
}

std::optional<std::filesystem::path> findResourceDirectory() {
    const std::filesystem::path executableDirectory{
        QCoreApplication::applicationDirPath().toStdWString()};
    const std::array candidates{
        executableDirectory / "files",
        executableDirectory.parent_path() / "files",
        raum::utils::resourceDirectory(),
        std::filesystem::current_path() / "files",
    };

    for (const auto& candidate : candidates) {
        if (isResourceDirectory(candidate)) {
            std::error_code error;
            const auto canonicalPath = std::filesystem::weakly_canonical(candidate, error);
            return error ? candidate : canonicalPath;
        }
    }
    return std::nullopt;
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);

    QFont uiFont(QStringLiteral("Segoe UI"));
    uiFont.setStyleHint(QFont::SansSerif);
    app.setFont(uiFont);

    const auto resourceDirectory = findResourceDirectory();
    if (!resourceDirectory) {
        QMessageBox::critical(
            nullptr,
            QStringLiteral("Raum Renderer Lab"),
            QStringLiteral("Required renderer assets were not found.\n\n"
                           "Keep the 'files' directory next to the executable, or one directory above it."));
        return EXIT_FAILURE;
    }
    raum::utils::setResourceDirectory(*resourceDirectory);

    raum::sample::UI ui(argc, argv);
    ui.show();
    return app.exec();
}
