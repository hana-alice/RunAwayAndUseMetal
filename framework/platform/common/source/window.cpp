#include "window.h"
#include <QGridLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QTimer>
#include <QWidget>
#include <QWindow>
#include "KeyboardEvent.h"
#include "MouseEvent.h"

namespace raum::platform {

using framework::ButtonStatus;
using framework::EventDispatcher;
using framework::Keyboard;
using framework::KeyboardEventTag;
using framework::KeyboardType;
using framework::MouseButton;
using framework::MouseEventTag;

MouseButton mapButton(Qt::MouseButton btn) {
    MouseButton button{MouseButton::OTHER};
    switch (btn) {
        case Qt::MouseButton::LeftButton:
            button = MouseButton::LEFT;
            break;
        case Qt::MouseButton::RightButton:
            button = MouseButton::RIGHT;
            break;
        case Qt::MouseButton::MiddleButton:
            button = MouseButton::WHEEL;
            break;
        case Qt::MouseButton::ForwardButton:
            button = MouseButton::FORWARD;
            break;
        case Qt::MouseButton::BackButton:
            button = MouseButton::BACKWARD;
            break;
        default:
            button = MouseButton::OTHER;
    }
    return button;
}

std::unordered_map<int, Keyboard> keyMap = {
    {Qt::Key_A, Keyboard::A},
    {Qt::Key_B, Keyboard::B},
    {Qt::Key_C, Keyboard::C},
    {Qt::Key_D, Keyboard::D},
    {Qt::Key_E, Keyboard::E},
    {Qt::Key_F, Keyboard::F},
    {Qt::Key_G, Keyboard::G},
    {Qt::Key_H, Keyboard::H},
    {Qt::Key_I, Keyboard::I},
    {Qt::Key_J, Keyboard::J},
    {Qt::Key_K, Keyboard::K},
    {Qt::Key_L, Keyboard::L},
    {Qt::Key_M, Keyboard::M},
    {Qt::Key_N, Keyboard::N},
    {Qt::Key_O, Keyboard::O},
    {Qt::Key_P, Keyboard::P},
    {Qt::Key_Q, Keyboard::Q},
    {Qt::Key_R, Keyboard::R},
    {Qt::Key_S, Keyboard::S},
    {Qt::Key_T, Keyboard::T},
    {Qt::Key_U, Keyboard::U},
    {Qt::Key_V, Keyboard::V},
    {Qt::Key_W, Keyboard::W},
    {Qt::Key_X, Keyboard::X},
    {Qt::Key_Y, Keyboard::Y},
    {Qt::Key_Z, Keyboard::Z},
    {Qt::Key_0, Keyboard::Digit0},
    {Qt::Key_1, Keyboard::Digit1},
    {Qt::Key_2, Keyboard::Digit2},
    {Qt::Key_3, Keyboard::Digit3},
    {Qt::Key_4, Keyboard::Digit4},
    {Qt::Key_5, Keyboard::Digit5},
    {Qt::Key_6, Keyboard::Digit6},
    {Qt::Key_7, Keyboard::Digit7},
    {Qt::Key_8, Keyboard::Digit8},
    {Qt::Key_9, Keyboard::Digit9},
    {Qt::Key_F1, Keyboard::F1},
    {Qt::Key_F2, Keyboard::F2},
    {Qt::Key_F3, Keyboard::F3},
    {Qt::Key_F4, Keyboard::F4},
    {Qt::Key_F5, Keyboard::F5},
    {Qt::Key_F6, Keyboard::F6},
    {Qt::Key_F7, Keyboard::F7},
    {Qt::Key_F8, Keyboard::F8},
    {Qt::Key_F9, Keyboard::F9},
    {Qt::Key_F10, Keyboard::F10},
    {Qt::Key_F11, Keyboard::F11},
    {Qt::Key_F12, Keyboard::F12},
    {Qt::Key_Escape, Keyboard::Escape},
    {Qt::Key_Tab, Keyboard::Tab},
    {Qt::Key_CapsLock, Keyboard::CapsLock},
    {Qt::Key_Shift, Keyboard::ShiftLeft},      // TODO: platform specific key code
    {Qt::Key_Shift, Keyboard::ShiftRight},     // TODO: platform specific key code
    {Qt::Key_Control, Keyboard::ControlLeft},  // TODO: platform specific key code
    {Qt::Key_Control, Keyboard::ControlRight}, // TODO: platform specific key code
    {Qt::Key_Alt, Keyboard::AltLeft},
    {Qt::Key_AltGr, Keyboard::AltRight},
    {Qt::Key_Meta, Keyboard::MetaLeft},
    {Qt::Key_Meta, Keyboard::MetaRight},
    {Qt::Key_BackForward, Keyboard::Backspace},
    {Qt::Key_Enter, Keyboard::Enter},
    {Qt::Key_Space, Keyboard::Space},
    {Qt::Key_Delete, Keyboard::Delete},
    {Qt::Key_Insert, Keyboard::Insert},
    {Qt::Key_Home, Keyboard::Home},
    {Qt::Key_End, Keyboard::End},
    {Qt::Key_PageUp, Keyboard::PageUp},
    {Qt::Key_PageDown, Keyboard::PageDown},
    {Qt::Key_Up, Keyboard::ArrowUp},
    {Qt::Key_Down, Keyboard::ArrowDown},
    {Qt::Key_Left, Keyboard::ArrowLeft},
    {Qt::Key_Right, Keyboard::ArrowRight},
    {Qt::Key_NumLock, Keyboard::NumLock},
    {Qt::Key_Period, Keyboard::Period},
    {Qt::Key_Slash, Keyboard::ForwardSlash},
    {Qt::Key_Backslash, Keyboard::BackwardSlash},
    {Qt::Key_Semicolon, Keyboard::Semicolon},
    {Qt::Key_Apostrophe, Keyboard::Apostrophe},
    {Qt::Key_BracketLeft, Keyboard::BracketLeft},
    {Qt::Key_BracketRight, Keyboard::BracketRight},
    {Qt::Key_Minus, Keyboard::Minus},
    {Qt::Key_Equal, Keyboard::Equal},
    {Qt::Key_QuoteLeft, Keyboard::Quote},
};

Keyboard mapKey(int key, bool pad) {
    Keyboard res{Keyboard::OTHER};
    if (!pad && keyMap.find(key) != keyMap.end()) {
        res = keyMap.at(key);
    }
    return res;
}

void NativeWindow::mouseMoveEvent(QMouseEvent* evt) {
    EventDispatcher<MouseEventTag>::get()->broadcast(
        std::forward_as_tuple(
            evt->pos().x(),
            evt->pos().y(),
            MouseButton::OTHER, // non sense
            ButtonStatus::PRESS // non sense
            ));
}

void NativeWindow::mousePressEvent(QMouseEvent* evt) {
    MouseButton button = mapButton(evt->button());
    EventDispatcher<MouseEventTag>::get()->broadcast(
        std::forward_as_tuple(
            evt->pos().x(),
            evt->pos().y(),
            button,
            ButtonStatus::PRESS));
}

void NativeWindow::mouseReleaseEvent(QMouseEvent* evt) {
    MouseButton button = mapButton(evt->button());
    EventDispatcher<MouseEventTag>::get()->broadcast(
        std::forward_as_tuple(
            evt->pos().x(),
            evt->pos().y(),
            button,
            ButtonStatus::RELEASE));
}

void NativeWindow::keyPressEvent(QKeyEvent* evt) {
    Keyboard key = mapKey(evt->key(), evt->modifiers().testFlag(Qt::KeypadModifier));
    EventDispatcher<KeyboardEventTag>::get()->broadcast(
        std::forward_as_tuple(
            key,
            KeyboardType::PRESS));
}

void NativeWindow::keyReleaseEvent(QKeyEvent* evt) {
    Keyboard key = mapKey(evt->key(), evt->modifiers().testFlag(Qt::KeypadModifier));
    EventDispatcher<KeyboardEventTag>::get()->broadcast(
        std::forward_as_tuple(
            key,
            KeyboardType::RELEASE));
}

Window::Window(int argc, char** argv, uint32_t w, uint32_t h) {
    _app = new QApplication(argc, argv);
    _window = new NativeWindow();
    _window->resize(w, h);
    _engineCanvas = QWidget::createWindowContainer(_window);
    _engineCanvas->resize(w, h);

    _timer = new QTimer();
    _timer->setInterval(std::chrono::milliseconds(16));
    QObject::connect(_timer, &QTimer::timeout, [&](){
        this->update();
    });
    _timer->start();
    _hwnd = _window->winId();
}

void Window::registerPollEvents(TickFunction&& tickFunc) {
    _tickFuncs.emplace_back(tickFunc);
}

void Window::update() {
    for (auto& callback : _tickFuncs) {
        callback();
    }
}

void Window::show() {
    _engineCanvas->show(); // container of engine, qt window
    _window->show();       // qt window
}

Size Window::pixelSize() {
    float ratio = _app->devicePixelRatio();
    return Size{
        static_cast<uint32_t>(_window->width() * ratio),
        static_cast<uint32_t>(_window->height() * ratio)};
}

void Window::mainLoop() {
    _app->exec();          // qt window loop
}

Window::~Window() {
    _app->quit();
    delete _window;
}

} // namespace raum::platform