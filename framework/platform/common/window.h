#pragma once
#include <functional>
#include <vector>
#include "core/data.h"
#include "core/utils/utils.h"

namespace raum::platform {
using TickFunction = utils::TickFunction<const std::chrono::milliseconds&>;
using TickID = uint64_t;
using ResizeID = uint64_t;
using ResizeFunction = std::function<void(uint32_t, uint32_t)>;

struct TickEntry {
    TickID id;
    TickFunction tickFunc;
};

class Window {
public:
    Window(int argc, char** argv, uint32_t width, uint32_t height);
    ~Window();

    uintptr_t handle() const { return _hwnd; }
    Size size() const { return _size; }

    TickID addTick(TickFunction&& tickFunc);
    void removeTick(TickID);

    TickID addResize(ResizeFunction&& resizeFunc);
    void removeResize(ResizeID);

    void show();
    void* surface() const { return _surface; };
    void* container() const { return _container; }

private:
    void tick(const std::chrono::milliseconds&);
    void resize(uint32_t width, uint32_t height);
    Size _size{};
    uintptr_t _hwnd{0};
    std::vector<TickEntry> _tickFuncs;
    void* _surface{nullptr};
    void* _container{nullptr};
    TickID _tickID{0};
};

using WindowPtr = std::shared_ptr<Window>;

} // namespace platform
