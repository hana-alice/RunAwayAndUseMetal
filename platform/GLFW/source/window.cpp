#include "window.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace platform {
NativeWindow::NativeWindow(uint32_t w, uint32_t h) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(w, h, "Vulkan", nullptr, nullptr);
}

void NativeWindow::registerPollEvents(TickFunction&& tickFunc) {
    _tickFunc.emplace_back(std::forward<TickFunction>(tickFunc));
}

void NativeWindow::mainLoop() {
    while (!glfwWindowShouldClose(static_cast<GLFWwindow*>(_window))) {
        glfwPollEvents();
        for (const auto& tick : _tickFunc) {
            tick();
        }
    }
}

NativeWindow::~NativeWindow() {
    glfwDestroyWindow(static_cast<GLFWwindow*>(_window));
    glfwTerminate();
}

} // namespace platform