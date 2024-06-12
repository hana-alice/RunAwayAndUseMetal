#pragma once
#include "Event.h"

namespace raum::framework {
enum class WindowEvent : uint8_t {
	
};

// type, width, height
using ResizeEventTag = EventTag<InputEventType::WINDOW_RESIZE, int32_t, int32_t>;
EventDispatcher<ResizeEventTag>;

using CloseEventTag = EventTag<InputEventType::WINDOW_CLOSE>;
EventDispatcher<CloseEventTag>;

} // namespace raum::framework