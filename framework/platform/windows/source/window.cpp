#include "window.h"
#include "WindowEvent.h"
#include <QResizeEvent>
#include <windows.h>

namespace raum::platform {

using framework::EventDispatcher;
using framework::ResizeEventTag;

// https://community.khronos.org/t/dpi-vksurfacecapabilitieskhr-extents-not-matching-qt-surface-extents/7625
void NativeWindow::resizeEvent(QResizeEvent* event) {
    window->updateSurface();
    if (window->handle()) [[likely]] {
        RECT rect;
        GetClientRect(reinterpret_cast<HWND>(window->handle()), &rect);
        EventDispatcher<ResizeEventTag>::get()->broadcast(
            std::forward_as_tuple(
                rect.right - rect.left,
                rect.bottom - rect.top));
    }
}

// https://community.khronos.org/t/dpi-vksurfacecapabilitieskhr-extents-not-matching-qt-surface-extents/7625
Size Window::pixelSize() const {
    RECT rect;
    GetClientRect(reinterpret_cast<HWND>(_engineCanvas->winId()), &rect);
    return Size{
        static_cast<uint32_t>(rect.right - rect.left),
        static_cast<uint32_t>(rect.bottom - rect.top)};
}

}