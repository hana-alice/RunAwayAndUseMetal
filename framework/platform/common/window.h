#pragma once
#include <functional>
#include <vector>
#include <QApplication>
#include <QMainWindow>
#include <QWindow>
#include "core/data.h"
#include <QTimer>
#include <QLabel>

namespace raum::platform {

class Window;
class NativeWindow : public QWindow {
    Q_OBJECT
public:
    NativeWindow() = default;
    void mousePressEvent(QMouseEvent* evt) override;
    void mouseReleaseEvent(QMouseEvent* evt) override;
    void mouseMoveEvent(QMouseEvent* evt) override;

    void keyPressEvent(QKeyEvent* evt) override;
    void keyReleaseEvent(QKeyEvent* evt) override;
    void resizeEvent(QResizeEvent* evt) override;

    void closeEvent(QCloseEvent* evt) override;

    class Window* window;
};

class TickFunction {
public:
    TickFunction() = default;
    TickFunction(std::function<void(std::chrono::milliseconds)>&& tickFunc) {
        _tickFunc = tickFunc;
    }

    void operator()(std::chrono::milliseconds miliSec) {
        _tickFunc(miliSec);
    }
private:
    std::function<void(std::chrono::milliseconds)> _tickFunc;
};

class Window {
public:
    Window(int argc, char** argv, uint32_t width, uint32_t height, void* inst);
    ~Window();

    uintptr_t handle() const { return _hwnd; }
    void* surface() const { return _surface; }
    void* nativeWindow() const { return _window; }

    void registerPollEvents(TickFunction* tickFunc);
    void removePollEvent(TickFunction* tickFunc);

    void show();
    void mainLoop();
    Size pixelSize() const;
    Position position() const;
    float devicePixelRatio() const;

    void updateSurface();

private:
    void update(std::chrono::milliseconds miliSec);

    NativeWindow* _engineCanvas{nullptr};
    QWidget* _window{nullptr};
    uintptr_t _hwnd;

    QApplication* _app{nullptr};

    QTimer* _timer{nullptr};
    std::vector<TickFunction*> _tickFuncs;

    void* _surface{nullptr};
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace platform