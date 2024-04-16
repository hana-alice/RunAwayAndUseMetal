#include "window.h"
#include <QTimer>

namespace raum::platform {
NativeWindow::NativeWindow(int argc, char** argv, uint32_t w, uint32_t h) {
    _app = new QApplication(argc, argv);
    _window = new QMainWindow(nullptr, Qt::Window);
    _window->addToolBar("raum");
    _window->resize(w,h);
    _hwnd = _window->winId();

    _timer = new QTimer();
    _timer->setInterval(std::chrono::milliseconds(33));
    QObject::connect(_timer, &QTimer::timeout, [&](){
        this->update();
    });
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

void NativeWindow::update() {
    for(auto& callback : _tickFunc) {
        callback();
    }
}

void NativeWindow::mainLoop() {
    _window->show();
    _timer->start();
    _app->exec();
}

NativeWindow::~NativeWindow() {
    _app->quit();
    _timer->stop();
    delete _timer;
    delete _window;
}

} // namespace platform