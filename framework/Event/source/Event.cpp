#include "Event.h"

namespace raum::framework {

Event::Event(EventType type):_evtType(type) {
}

InputEvent::InputEvent(InputEventType type):Event(EventType::INPUT), _type(type) {
}

OutputEvent::OutputEvent(): Event(EventType::OUTPUT) {}


}