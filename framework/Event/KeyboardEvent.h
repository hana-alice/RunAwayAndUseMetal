#pragma once
#include "Event.h"

namespace raum::framework {

// gpt
enum class Keyboard {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    ForwardSlash,
    BackwardSlash,
    Comma,
    Period,
    Semicolon,
    Apostrophe,
    BracketLeft,
    BracketRight,
    Minus,
    Equal,
    Quote,

    Escape,
    Tab,
    CapsLock,
    ShiftLeft,
    ShiftRight,
    ControlLeft,
    ControlRight,
    AltLeft,
    AltRight,
    MetaLeft,  // Windows key
    MetaRight, // Windows key

    Backspace,
    Enter,
    Space,
    Delete,
    Insert,
    Home,
    End,
    PageUp,
    PageDown,

    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,

    NumLock,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    NumpadAdd,
    NumpadSubtract,
    NumpadMultiply,
    NumpadDivide,
    NumpadDecimal,

    OTHER,
};

enum class KeyboardType : uint8_t {
    PRESS,
    RELEASE,
};

bool keyPressed(Keyboard key);

// EventType, Key, type
using KeyboardEventTag = EventTag<InputEventType::KEYBOARD>;
EventDispatcher<KeyboardEventTag>;

} // namespace raum::framework