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
using MouseButtonEventTag = EventTag<InputEventType::MOUSE, float, float, MouseButton, ButtonStatus>;
EventDispatcher<MouseButtonEventTag>;

// posx, posy, delta of x, delta of y, button, button status.
using MouseMotionEventTag = EventTag<InputEventType::MOUSE, float, float, float, float>;
EventDispatcher<MouseMotionEventTag>;

// posx, posy, delta of wheel
using MouseWheelEventTag = EventTag<InputEventType::MOUSE, float, float, float>;
EventDispatcher<MouseWheelEventTag>;


} // namespace raum::framework