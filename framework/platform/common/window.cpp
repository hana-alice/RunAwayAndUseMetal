#include "window.h"
#include <QElapsedTimer>
#include <QTimer>
#include <QWindow>
#include <QWidget>
#include <QResizeEvent>
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "WindowEvent.h"
#include "core/utils/log.h"

static constexpr uint32_t MAX_FPS = 60;

namespace raum::platform {
using framework::ButtonStatus;
using framework::CloseEventTag;
using framework::EventDispatcher;
using framework::Keyboard;
using framework::KeyboardEventTag;
using framework::KeyboardType;
using framework::MouseButton;
using framework::MouseButtonEventTag;
using framework::MouseMotionEventTag;
using framework::MouseWheelEventTag;
using framework::ResizeEventTag;

// MouseButton mapButton(SDL_MouseButtonFlags btn) {
//     MouseButton button{MouseButton::OTHER};
//     switch (btn) {
//         case SDL_BUTTON_LEFT: button = MouseButton::LEFT;
//             break;
//         case SDL_BUTTON_RIGHT: button = MouseButton::RIGHT;
//             break;
//         case SDL_BUTTON_MIDDLE: button = MouseButton::WHEEL;
//             break;
//         case SDL_BUTTON_X1: button = MouseButton::FORWARD;
//             break;
//         case SDL_BUTTON_X2: button = MouseButton::BACKWARD;
//             break;
//         default: button = MouseButton::OTHER;
//     }
//     return button;
// }
//
// struct KeyStatus {
//     SDL_Scancode key;
//     bool pressed{false};
// };
//
// std::unordered_map<Keyboard, KeyStatus> keyMap = {
//     {Keyboard::A, {SDL_SCANCODE_A}},
//     {Keyboard::B, {SDL_SCANCODE_B}},
//     {Keyboard::C, {SDL_SCANCODE_C}},
//     {Keyboard::B, {SDL_SCANCODE_B}},
//     {Keyboard::C, {SDL_SCANCODE_C}},
//     {Keyboard::D, {SDL_SCANCODE_D}},
//     {Keyboard::E, {SDL_SCANCODE_E}},
//     {Keyboard::F, {SDL_SCANCODE_F}},
//     {Keyboard::G, {SDL_SCANCODE_G}},
//     {Keyboard::H, {SDL_SCANCODE_H}},
//     {Keyboard::I, {SDL_SCANCODE_I}},
//     {Keyboard::J, {SDL_SCANCODE_J}},
//     {Keyboard::K, {SDL_SCANCODE_K}},
//     {Keyboard::L, {SDL_SCANCODE_L}},
//     {Keyboard::M, {SDL_SCANCODE_M}},
//     {Keyboard::N, {SDL_SCANCODE_N}},
//     {Keyboard::O, {SDL_SCANCODE_O}},
//     {Keyboard::P, {SDL_SCANCODE_P}},
//     {Keyboard::Q, {SDL_SCANCODE_Q}},
//     {Keyboard::R, {SDL_SCANCODE_R}},
//     {Keyboard::S, {SDL_SCANCODE_S}},
//     {Keyboard::T, {SDL_SCANCODE_T}},
//     {Keyboard::U, {SDL_SCANCODE_U}},
//     {Keyboard::V, {SDL_SCANCODE_V}},
//     {Keyboard::W, {SDL_SCANCODE_W}},
//     {Keyboard::X, {SDL_SCANCODE_X}},
//     {Keyboard::Y, {SDL_SCANCODE_Y}},
//     {Keyboard::Z, {SDL_SCANCODE_Z}},
//     {Keyboard::Digit0, {SDL_SCANCODE_0}},
//     {Keyboard::Digit1, {SDL_SCANCODE_1}},
//     {Keyboard::Digit2, {SDL_SCANCODE_2}},
//     {Keyboard::Digit3, {SDL_SCANCODE_3}},
//     {Keyboard::Digit4, {SDL_SCANCODE_4}},
//     {Keyboard::Digit5, {SDL_SCANCODE_5}},
//     {Keyboard::Digit6, {SDL_SCANCODE_6}},
//     {Keyboard::Digit7, {SDL_SCANCODE_7}},
//     {Keyboard::Digit8, {SDL_SCANCODE_8}},
//     {Keyboard::Digit9, {SDL_SCANCODE_9}},
//     {Keyboard::F1, {SDL_SCANCODE_F1}},
//     {Keyboard::F2, {SDL_SCANCODE_F2}},
//     {Keyboard::F3, {SDL_SCANCODE_F3}},
//     {Keyboard::F4, {SDL_SCANCODE_F4}},
//     {Keyboard::F5, {SDL_SCANCODE_F5}},
//     {Keyboard::F6, {SDL_SCANCODE_F6}},
//     {Keyboard::F7, {SDL_SCANCODE_F7}},
//     {Keyboard::F8, {SDL_SCANCODE_F8}},
//     {Keyboard::F9, {SDL_SCANCODE_F9}},
//     {Keyboard::F10, {SDL_SCANCODE_F10}},
//     {Keyboard::F11, {SDL_SCANCODE_F11}},
//     {Keyboard::F12, {SDL_SCANCODE_F12}},
//     {Keyboard::Escape, {SDL_SCANCODE_ESCAPE}},
//     {Keyboard::Tab, {SDL_SCANCODE_TAB}},
//     {Keyboard::CapsLock, {SDL_SCANCODE_CAPSLOCK}},
//     {Keyboard::ShiftLeft, {SDL_SCANCODE_LSHIFT}}, // TODO: platform specific key code
//     {Keyboard::ShiftRight, {SDL_SCANCODE_RSHIFT}}, // TODO: platform specific key code
//     {Keyboard::ControlLeft, {SDL_SCANCODE_LCTRL}}, // TODO: platform specific key code
//     {Keyboard::ControlRight, {SDL_SCANCODE_RCTRL}}, // TODO: platform specific key code
//     {Keyboard::AltLeft, {SDL_SCANCODE_LALT}},
//     {Keyboard::AltRight, {SDL_SCANCODE_RALT}},
//     {Keyboard::MetaLeft, {SDL_SCANCODE_LGUI}},
//     {Keyboard::MetaRight, {SDL_SCANCODE_RGUI}},
//     {Keyboard::Backspace, {SDL_SCANCODE_BACKSPACE}},
//     {Keyboard::Enter, {SDL_SCANCODE_RETURN}},
//     {Keyboard::Space, {SDL_SCANCODE_SPACE}},
//     {Keyboard::Delete, {SDL_SCANCODE_DELETE}},
//     {Keyboard::Insert, {SDL_SCANCODE_INSERT}},
//     {Keyboard::Home, {SDL_SCANCODE_HOME}},
//     {Keyboard::End, {SDL_SCANCODE_END}},
//     {Keyboard::PageUp, {SDL_SCANCODE_PAGEUP}},
//     {Keyboard::PageDown, {SDL_SCANCODE_PAGEDOWN}},
//     {Keyboard::ArrowUp, {SDL_SCANCODE_UP}},
//     {Keyboard::ArrowDown, {SDL_SCANCODE_DOWN}},
//     {Keyboard::ArrowLeft, {SDL_SCANCODE_LEFT}},
//     {Keyboard::ArrowRight, {SDL_SCANCODE_RIGHT}},
//     {Keyboard::NumLock, {SDL_SCANCODE_NUMLOCKCLEAR}},
//     {Keyboard::Period, {SDL_SCANCODE_PERIOD}},
//     {Keyboard::ForwardSlash, {SDL_SCANCODE_SLASH}},
//     {Keyboard::BackwardSlash, {SDL_SCANCODE_BACKSLASH}},
//     {Keyboard::Semicolon, {SDL_SCANCODE_SEMICOLON}},
//     {Keyboard::Apostrophe, {SDL_SCANCODE_APOSTROPHE}},
//     {Keyboard::BracketLeft, {SDL_SCANCODE_LEFTBRACKET}},
//     {Keyboard::BracketRight, {SDL_SCANCODE_RIGHTBRACKET}},
//     {Keyboard::Minus, {SDL_SCANCODE_MINUS}},
//     {Keyboard::Equal, {SDL_SCANCODE_EQUALS}},
// };

namespace {
void closeEvent() {
    EventDispatcher<CloseEventTag>::get()->broadcast({});
}

void resizeEvent(int w, int h) {
    EventDispatcher<ResizeEventTag>::get()->broadcast(
        std::forward_as_tuple(w, h));
}

// void keyEvent() {
//     auto* sysKeyStatus = SDL_GetKeyboardState(NULL);
//     for (auto& [key, status] : keyMap) {
//         if (status.pressed ^ sysKeyStatus[status.key]) {
//             status.pressed = sysKeyStatus[status.key];
//         }
//     }
//     EventDispatcher<KeyboardEventTag>::get()->broadcast({});
// }

// void mouseReleaseEvent(const SDL_MouseButtonEvent& btn) {
//     MouseButton button = mapButton(btn.button);
//     EventDispatcher<MouseButtonEventTag>::get()->broadcast(
//         std::forward_as_tuple(
//             btn.x,
//             btn.y,
//             button,
//             ButtonStatus::RELEASE));
// }
//
// void mousePressEvent(const SDL_MouseButtonEvent& btn) {
//     MouseButton button = mapButton(btn.button);
//     EventDispatcher<MouseButtonEventTag>::get()->broadcast(
//         std::forward_as_tuple(
//             btn.x,
//             btn.y,
//             button,
//             ButtonStatus::PRESS));
// }
//
// void mouseButtonEvent(const SDL_MouseButtonEvent& btn) {
//     if (btn.down) {
//         mousePressEvent(btn);
//     } else {
//         mouseReleaseEvent(btn);
//     }
// }
//
// void mouseMoveEvent(const SDL_MouseMotionEvent& motion) {
//     EventDispatcher<MouseMotionEventTag>::get()->broadcast(
//         std::forward_as_tuple(
//             motion.x,
//             motion.y,
//             motion.xrel,
//             motion.yrel
//             ));
// }
//
// void mouseWheelEvent(const SDL_MouseWheelEvent& wheel) {
//     EventDispatcher<MouseWheelEventTag>::get()->broadcast(
//         std::forward_as_tuple(
//             wheel.mouse_x,
//             wheel.mouse_y,
//             wheel.y
//             ));
// }
}

class RUIEmbededWindow : public QWindow {
    Q_OBJECT
public:
    RUIEmbededWindow(TickFunction&& tickFunc) : _tickFunc(tickFunc) {
        setSurfaceType(QSurface::RasterSurface);
        setFlag(Qt::FramelessWindowHint, true);
        _timer.setInterval(int(1000.0 / MAX_FPS));
        connect(&_timer, &QTimer::timeout, this, &RUIEmbededWindow::tick);
    }

    void prepare() {
        _container = QWidget::createWindowContainer(this);
        _container->setFocusPolicy(Qt::StrongFocus);
        _container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        _container->setFixedSize(width(), height());
    }

    ~RUIEmbededWindow() {
        _timer.stop();
    }

    void show() {
        QWindow::show();
        _timer.start();
    }

    void tick() {
        const qint64 now = _elapsedTimer.nsecsElapsed();
        const auto delta = now - _lastns;
        auto dt = std::chrono::nanoseconds(delta);
        auto dtms = std::chrono::duration_cast<std::chrono::milliseconds>(dt);
        _tickFunc(dtms);
        _lastns = now;
    }

    void setResizeEvent(ResizeFunction&& resizeFunc) {
        _resizeFunc = std::move(resizeFunc);
    }

    QWidget* container() const {
        return _container;
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        const auto& size = event->size();
        _resizeFunc(static_cast<uint32_t>(size.width()), static_cast<uint32_t>(size.height()));
    }

private:
    QTimer _timer;
    QElapsedTimer _elapsedTimer;
    qint64 _lastns{0};
    TickFunction _tickFunc;
    ResizeFunction _resizeFunc;
    QWidget* _container{nullptr};
};

static RUIEmbededWindow* EmbededWindow = nullptr;

Window::Window(int argc, char** argv, uint32_t w, uint32_t h) {
    EmbededWindow = new RUIEmbededWindow(
        [this](const std::chrono::milliseconds& ms) {
            this->tick(ms);
        });
    EmbededWindow->setResizeEvent([this](uint32_t w, uint32_t h) {
            this->resize(w, h);
        });
    EmbededWindow->setWidth(w);
    EmbededWindow->setHeight(h);
    EmbededWindow->setTitle("Run!");

    _hwnd = EmbededWindow->winId();
    EmbededWindow->prepare();

    const auto& size = EmbededWindow->size();
    _size = {
        static_cast<uint32_t>(size.width()),
        static_cast<uint32_t>(size.height())
    };

    _surface = EmbededWindow;
    _container = EmbededWindow->container();
    raum_info("window size: {} {}, widget size: {} {}, dpr: {}, {} {} {}",
        EmbededWindow->size().width(),
        EmbededWindow->size().height(),
        EmbededWindow->container()->size().width(),
        EmbededWindow->container()->size().height(),
        EmbededWindow->devicePixelRatio(),
        EmbededWindow->container()->devicePixelRatio(),
        EmbededWindow->container()->devicePixelRatioF(),
        EmbededWindow->container()->devicePixelRatioFScale());
}

void Window::tick(const std::chrono::milliseconds& ms) {
    if (_hwnd != EmbededWindow->winId()) {
        resize(EmbededWindow->width(), EmbededWindow->height());
    }
    for (auto& entry : _tickFuncs) {
        entry.tickFunc(ms);
    }
}

void Window::show() {
    EmbededWindow->show();
    _hwnd = EmbededWindow->winId();

    // if (_surface) {
    //     bool running = true;
    //     SDL_Event evt;
    //     while (running) {
    //         SDL_PollEvent(&evt);
    //
    //         // native events notification running before the main loop
    //         switch (evt.type) {
    //             case SDL_EVENT_QUIT: {
    //                 running = false;
    //                 break;
    //             }
    //             case SDL_EVENT_WINDOW_RESIZED: {
    //                 resizeEvent(evt.window.data1, evt.window.data2);
    //                 break;
    //             }
    //             case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
    //                 closeEvent();
    //                 break;
    //             }
    //             case SDL_EVENT_KEY_UP: {
    //                 keyEvent();
    //                 break;
    //             }
    //             case SDL_EVENT_KEY_DOWN: {
    //                 keyEvent();
    //                 break;
    //             }
    //             case SDL_EVENT_MOUSE_BUTTON_UP: {
    //                 mouseReleaseEvent(evt.button);
    //                 break;
    //             }
    //             case SDL_EVENT_MOUSE_BUTTON_DOWN: {
    //                 mousePressEvent(evt.button);
    //                 break;
    //             }
    //             case SDL_EVENT_MOUSE_MOTION: {
    //                 mouseMoveEvent(evt.motion);
    //                 break;
    //             }
    //             case SDL_EVENT_MOUSE_WHEEL: {
    //                 mouseWheelEvent(evt.wheel);
    //                 break;;
    //             }
    //         }
    //
    //         for (auto* callback : _tickFuncs) {
    //             (*callback)(std::chrono::milliseconds {});
    //         }
    //     }
    // }
}

TickID Window::addTick(TickFunction&& tickFunc) {
    TickID tickID = ++_tickID;
    _tickFuncs.push_back({tickID, std::move(tickFunc)});
    return tickID;
}

void Window::removeTick(TickID id) {
    std::erase_if(_tickFuncs, [id](const TickEntry& entry) {
        return entry.id == id;
    });
}

void Window::resize(uint32_t width, uint32_t height) {
    _size = {width, height};
    resizeEvent(width, height);
}

Window::~Window() {
    // if (_surface) {
    //     SDL_DestroyWindow(static_cast<SDL_Window*>(_surface));
    // }
}
} // namespace raum::platform

namespace raum::framework {
bool getKeyPressedNative(Keyboard key) {
    return false;//platform::keyMap[key].pressed;
}

} // namespace raum::framework

#include "window.moc"