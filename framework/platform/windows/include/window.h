#pragma once
#include <SDL2/SDL.h>
#include <functional>
#include <vector>

namespace platform {
using TickFunction = std::function<void(void)>;

class NativeWindow {
public:
    NativeWindow(uint32_t width, uint32_t height);
    ~NativeWindow();

    void* handle() { return _hwnd; }
    void registerPollEvents(TickFunction&& tickFunc);

    void mainLoop();

private:
    SDL_Window* _window{nullptr};
    void* _hwnd;

    std::vector<TickFunction> _tickFunc;
};
} // namespace platform
