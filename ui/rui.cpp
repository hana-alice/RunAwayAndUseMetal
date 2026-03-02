#include "rui.h"
#include <QWindow>
#include <QTimer>
#include <QWidget>

#include "RHIDevice.h"

static constexpr uint32_t MAX_FPS = 60;

namespace raum::ui {

class RUIEmbededWindow : public QWindow {
    Q_OBJECT
    public:
    RUIEmbededWindow() {
        setSurfaceType(QSurface::RasterSurface);

        _timer.setInterval(1.0f / MAX_FPS);
        connect(&_timer, &QTimer::timeout, this, &RUIEmbededWindow::tick);

    }

    void show() {

    }

    void tick() {

    }

    QTimer _timer;
};

RUI::RUI(rhi::DevicePtr device, platform::WindowPtr window) {
    QWidget* central = new QWidget();

}


}
