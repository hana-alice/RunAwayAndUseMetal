#pragma once
#include <functional>
#include <vector>
namespace platform {
using TickFunction = std::function<void(void)>;

class NativeWindow {
public:
    NativeWindow(uint32_t width, uint32_t height);
    ~NativeWindow();

    void registerPollEvents(TickFunction&& tickFunc);

    void mainLoop();

private:
    void* _window{nullptr};

    std::vector<TickFunction> _tickFunc;
};
} // namespace platform
