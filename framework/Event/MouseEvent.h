#pragma once
#include "Event.h"

namespace raum::framework {

enum class MouseButton : uint8_t {
    LEFT,
    RIGHT,
    WHEEL,
    FORWARD,
    BACKWARD,
    OTHER,
};

enum class ButtonStatus : uint8_t {
    PRESS,
    RELEASE,
};

// type, posX(relative to mainwindow), posY(relative to mainwindow) or delta of wheel, button, button status.
using MouseEventTag = EventTag<InputEventType::MOUSE, int32_t, int32_t, MouseButton, ButtonStatus>;
EventDispatcher<MouseEventTag>;


} // namespace raum::framework