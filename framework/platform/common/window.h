#pragma once
#include <functional>
#include <vector>
#include "core/data.h"
#include "core/utils/utils.h"

namespace raum::platform {
using TickFunction = utils::TickFunction<std::chrono::milliseconds>;

class Window {
public:
    Window(int argc, char** argv, uint32_t width, uint32_t height, void* inst);
    ~Window();

    uintptr_t handle() const { return _hwnd; }
    Size size() const { return _size; }

    void registerPollEvents(TickFunction* tickFunc);
    void removePollEvent(TickFunction* tickFunc);

    void show();

private:

    Size _size{};
    uintptr_t _hwnd;
    std::vector<TickFunction*> _tickFuncs;
    void* _surface{nullptr};
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace platform
