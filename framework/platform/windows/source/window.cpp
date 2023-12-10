#include "window.h"

namespace platform {
NativeWindow::NativeWindow(uint32_t w, uint32_t h) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    _window = SDL_CreateWindow("raum", w, h, SDL_WINDOW_VULKAN);

    _hwnd = SDL_GetProperty(SDL_GetWindowProperties(_window), "SDL.window.win32.hwnd", nullptr);
}

void NativeWindow::registerPollEvents(TickFunction&& tickFunc) {
    _tickFunc.emplace_back(std::forward<TickFunction>(tickFunc));
}

void NativeWindow::mainLoop() {
    bool quit{false};
    SDL_Event event;
    while (!quit) {
        SDL_PollEvent(&event);
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;

            default:
                break;
        }
        for (const auto& tick : _tickFunc) {
            tick();
        }
    }
}

NativeWindow::~NativeWindow() {
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

} // namespace platform