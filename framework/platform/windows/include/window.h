#pragma once
#include <functional>
#include <vector>
#include <QApplication>
#include "core/data.h"

namespace raum::platform {
using TickFunction = std::function<void(void)>;

class NativeWindow {
public:
    NativeWindow(int argc, char** argv, uint32_t width, uint32_t height);
    ~NativeWindow();

    uintptr_t handle() { return _hwnd; }
    void registerPollEvents(TickFunction&& tickFunc);

    void mainLoop();

    Size pixelSize();

private:
    void update();
    QWindow* _window{nullptr};
    QWidget* _engineCanvas{nullptr};
    uintptr_t _hwnd;

    std::vector<TickFunction> _tickFunc;
    QApplication* _app{nullptr};
    QTimer* _timer{nullptr};
};

using NativeWindowPtr = std::shared_ptr<NativeWindow>;

} // namespace platform
