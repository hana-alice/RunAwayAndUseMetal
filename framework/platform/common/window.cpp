#include "window.h"
#include <QElapsedTimer>
#include <QTimer>
#include <QWindow>
#include <QWidget>
#include <QResizeEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCloseEvent>
#include <unordered_map>
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

struct KeyStatus {
    bool pressed{false};
};

// key pressed state, referenced by getKeyPressedNative()
std::unordered_map<Keyboard, KeyStatus> keyMap;

static Keyboard mapQtKeyToKeyboard(int key) {
    switch (key) {
    case Qt::Key_W: return Keyboard::W;
    case Qt::Key_A: return Keyboard::A;
    case Qt::Key_S: return Keyboard::S;
    case Qt::Key_D: return Keyboard::D;
    case Qt::Key_Up: return Keyboard::ArrowUp;
    case Qt::Key_Down: return Keyboard::ArrowDown;
    case Qt::Key_Left: return Keyboard::ArrowLeft;
    case Qt::Key_Right: return Keyboard::ArrowRight;
    default: return Keyboard::OTHER;
    }
}

static MouseButton mapQtMouseButton(Qt::MouseButton btn) {
    switch (btn) {
    case Qt::LeftButton: return MouseButton::LEFT;
    case Qt::RightButton: return MouseButton::RIGHT;
    case Qt::MiddleButton: return MouseButton::WHEEL;
    case Qt::ForwardButton: return MouseButton::FORWARD;
    case Qt::BackButton: return MouseButton::BACKWARD;
    default: return MouseButton::OTHER;
    }
}

namespace {
void closeEvent() {
    EventDispatcher<CloseEventTag>::get()->broadcast({});
}

void resizeEvent(int w, int h) {
    EventDispatcher<ResizeEventTag>::get()->broadcast(
        std::forward_as_tuple(w, h));
}

void dispatchKeyEvent(Keyboard key, bool pressed) {
    if (key == Keyboard::OTHER) {
        return;
    }
    auto& status = keyMap[key];
    if (status.pressed == pressed) {
        return;
    }
    status.pressed = pressed;
    EventDispatcher<KeyboardEventTag>::get()->broadcast({});
}

void dispatchMouseButtonEvent(const QPointF& pos, Qt::MouseButton btn, ButtonStatus status) {
    MouseButton button = mapQtMouseButton(btn);
    EventDispatcher<MouseButtonEventTag>::get()->broadcast(
        std::forward_as_tuple(
            static_cast<float>(pos.x()),
            static_cast<float>(pos.y()),
            button,
            status));
}

void dispatchMouseMoveEvent(const QPointF& pos, const QPointF& lastPos) {
    EventDispatcher<MouseMotionEventTag>::get()->broadcast(
        std::forward_as_tuple(
            static_cast<float>(pos.x()),
            static_cast<float>(pos.y()),
            static_cast<float>(pos.x() - lastPos.x()),
            static_cast<float>(pos.y() - lastPos.y())));
}

void dispatchMouseWheelEvent(const QPointF& pos, const QPoint& angleDelta) {
    EventDispatcher<MouseWheelEventTag>::get()->broadcast(
        std::forward_as_tuple(
            static_cast<float>(pos.x()),
            static_cast<float>(pos.y()),
            static_cast<float>(angleDelta.y())));
}

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
        if (_container) {
            return;
        }
        _container = QWidget::createWindowContainer(this);
        _container->setFocusPolicy(Qt::StrongFocus);
        _container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
        QWindow::resizeEvent(event);
        const auto& size = event->size();
        _resizeFunc(static_cast<uint32_t>(size.width()), static_cast<uint32_t>(size.height()));
    }

    void keyPressEvent(QKeyEvent* event) override {
        Keyboard key = mapQtKeyToKeyboard(event->key());
        dispatchKeyEvent(key, true);
        QWindow::keyPressEvent(event);
    }

    void keyReleaseEvent(QKeyEvent* event) override {
        Keyboard key = mapQtKeyToKeyboard(event->key());
        dispatchKeyEvent(key, false);
        QWindow::keyReleaseEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        dispatchMouseButtonEvent(event->pos(), event->button(), ButtonStatus::PRESS);
        QWindow::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        dispatchMouseButtonEvent(event->pos(), event->button(), ButtonStatus::RELEASE);
        QWindow::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        auto pos = event->pos();
        dispatchMouseMoveEvent(pos, _lastPos);
        QWindow::mouseMoveEvent(event);
        _lastPos = pos;
    }

    void wheelEvent(QWheelEvent* event) override {
        dispatchMouseWheelEvent(event->pos(), event->angleDelta());
        QWindow::wheelEvent(event);
    }

private:
    QTimer _timer;
    QElapsedTimer _elapsedTimer;
    qint64 _lastns{0};
    TickFunction _tickFunc;
    ResizeFunction _resizeFunc;
    QWidget* _container{nullptr};
    QPoint _lastPos{0, 0};
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

    const auto& size = EmbededWindow->size();
    _size = {
        static_cast<uint32_t>(size.width()),
        static_cast<uint32_t>(size.height())
    };

    _surface = EmbededWindow;
    raum_info("window size: {} {}, dpr: {}",
        EmbededWindow->size().width(),
        EmbededWindow->size().height(),
        EmbededWindow->devicePixelRatio());
}

void* Window::container() {
    if (!_container) {
        EmbededWindow->prepare();
        _container = EmbededWindow->container();
    }
    return _container;
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

}
} // namespace raum::platform

namespace raum::framework {
bool getKeyPressedNative(Keyboard key) {
    return platform::keyMap[key].pressed;
}

} // namespace raum::framework

#include "window.moc"
