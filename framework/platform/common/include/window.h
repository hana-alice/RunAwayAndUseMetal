#pragma once
#include <functional>
#include <vector>
#include <QApplication>
#include <QWindow>
#include "core/data.h"
#include <QTimer>

namespace raum::platform {
using TickFunction = std::function<void(void)>;

class NativeWindow : public QWindow {
    Q_OBJECT
public:
    NativeWindow() = default;
    void mousePressEvent(QMouseEvent* evt) override;
    void mouseReleaseEvent(QMouseEvent* evt) override;
    void mouseMoveEvent(QMouseEvent* evt) override;

    void keyPressEvent(QKeyEvent* evt) override;
    void keyReleaseEvent(QKeyEvent* evt) override;

};

class Window {
public:
    Window(int argc, char** argv, uint32_t width, uint32_t height);
    ~Window();

    uintptr_t handle() { return _hwnd; }
    void registerPollEvents(TickFunction&& tickFunc);

    void show();

    void mainLoop();

    Size pixelSize();

private:
    void update();

    NativeWindow* _window{nullptr};
    QWidget* _engineCanvas{nullptr};
    uintptr_t _hwnd;

    QApplication* _app{nullptr};

    QTimer* _timer{nullptr};
    std::vector<TickFunction> _tickFuncs;
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace platform
