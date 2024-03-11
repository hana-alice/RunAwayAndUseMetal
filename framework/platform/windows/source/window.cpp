#include "window.h"

namespace raum::platform {
NativeWindow::NativeWindow(int argc, char** argv, uint32_t w, uint32_t h) {
    _app = new QApplication(argc, argv);
    _window = new QMainWindow(nullptr, Qt::Window);
    _window->addToolBar("raum");
    _window->resize(w,h);
    _hwnd = _window->winId();
}

void NativeWindow::registerPollEvents(TickFunction&& tickFunc) {
    _tickFunc.emplace_back(std::forward<TickFunction>(tickFunc));
}

Size NativeWindow::pixelSize() {
    float ratio = _app->devicePixelRatio();
    return Size{
        static_cast<uint32_t>(_window->width() * ratio),
        static_cast<uint32_t>(_window->height() * ratio)
    };
}

void NativeWindow::mainLoop() {
    _window->show();
    _app->exec();
}

NativeWindow::~NativeWindow() {
    _app->quit();
    delete _window;
}

} // namespace platform