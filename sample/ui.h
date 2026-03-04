#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <QApplication>
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QWidget>
#include <QWindow>
#include <QResizeEvent>
#include <QEvent>
#include <QGroupBox>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QString>
#include <QTimer>
#include "sample.h"
namespace raum::sample {

class AspectRatioContainer : public QWidget {
public:
    AspectRatioContainer(QWidget* child,
                         float aspect,
                         QWidget* parent = nullptr)
        : QWidget(parent)
        , _child(child)
        , _aspect(aspect) {
        Q_ASSERT(_child);
        _child->setParent(this);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        if (!_child) {
            return;
        }

        const int w = event->size().width();
        const int h = event->size().height();

        int targetW = w;
        int targetH = static_cast<int>(static_cast<float>(targetW) / _aspect);
        if (targetH > h) {
            targetH = h;
            targetW = static_cast<int>(static_cast<float>(targetH) * _aspect);
        }

        const int x = (w - targetW) / 2;
        const int y = (h - targetH) / 2;
        _child->setGeometry(x, y, targetW, targetH);
    }

private:
    QWidget* _child{nullptr};
    float _aspect{1.0f};
};

class UI {
public:
    UI(int argc, char** argv) {
        qApp->setStyleSheet(loadStyle());
        _sample = new Sample(argc, argv);
        _window = _sample->window();

        QWidget* engineWidget = static_cast<QWidget*>(_window->container());
        engineWidget->setMinimumSize(static_cast<int>(s_width), static_cast<int>(s_height));
        auto* aspectContainer = new AspectRatioContainer(
            engineWidget,
            static_cast<float>(s_width) / static_cast<float>(s_height));

        auto* panel = new QWidget;
        panel->setObjectName("sidePanel");
        auto* panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(14, 14, 14, 14);
        panelLayout->setSpacing(10);

        // Camera parameters UI (position and FOV)
        auto* camGroup = new QGroupBox("Camera");
        auto* camLayout = new QFormLayout(camGroup);

        _posX = new QDoubleSpinBox;
        _posY = new QDoubleSpinBox;
        _posZ = new QDoubleSpinBox;
        for (auto spin : {_posX, _posY, _posZ}) {
            spin->setRange(-10000.0, 10000.0);
            spin->setDecimals(3);
        }

        _fovSpin = new QDoubleSpinBox;
        _fovSpin->setRange(10.0, 120.0);
        _fovSpin->setDecimals(1);

        _yawSpin = new QDoubleSpinBox;
        _pitchSpin = new QDoubleSpinBox;
        _yawSpin->setRange(-360.0, 360.0);
        _pitchSpin->setRange(-89.0, 89.0);
        _yawSpin->setDecimals(1);
        _pitchSpin->setDecimals(1);

        camLayout->addRow("Pos X", _posX);
        camLayout->addRow("Pos Y", _posY);
        camLayout->addRow("Pos Z", _posZ);
        camLayout->addRow("FOV", _fovSpin);
        camLayout->addRow("Yaw", _yawSpin);
        camLayout->addRow("Pitch", _pitchSpin);

        panelLayout->addWidget(camGroup);
        panelLayout->addStretch(1);

        // Content row: rendering surface + camera panel
        auto* content = new QWidget;
        auto* contentLayout = new QHBoxLayout(content);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->addWidget(aspectContainer, /*stretch*/ 1);
        contentLayout->addWidget(panel, /*stretch*/ 0);

        // FPS label pinned to the bottom
        _fpsLabel = new QLabel("FPS: --");
        _fpsLabel->setObjectName("fpsLabel");
        _fpsLabel->setContentsMargins(8, 3, 8, 3);

        auto* root = new QWidget;
        auto* rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(0);
        rootLayout->addWidget(content, /*stretch*/ 1);
        rootLayout->addWidget(_fpsLabel, /*stretch*/ 0);

        auto* main = new QMainWindow;
        main->setCentralWidget(root);

        const int initialW = static_cast<int>(s_width) + 380;
        const int initialH = static_cast<int>(s_height) + _fpsLabel->sizeHint().height();
        main->resize(initialW, initialH);
        main->setMinimumSize(initialW, initialH);

        // Initialize camera UI from current sample if possible
        if (auto* current = _sample->currentSample()) {
            float x = 0.0f, y = 0.0f, z = 0.0f;
            if (current->getCameraPosition(x, y, z)) {
                _posX->setValue(x);
                _posY->setValue(y);
                _posZ->setValue(z);
            }
            float yaw = 0.0f, pitch = 0.0f;
            if (current->getCameraAngles(yaw, pitch)) {
                _yawSpin->setValue(yaw);
                _pitchSpin->setValue(pitch);
            }
            float fov = 0.0f;
            if (current->getCameraFov(fov)) {
                _fovSpin->setValue(fov);
            }
        }

        QObject::connect(_posX, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         [this](double) { syncCameraFromUi(); });
        QObject::connect(_posY, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         [this](double) { syncCameraFromUi(); });
        QObject::connect(_posZ, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         [this](double) { syncCameraFromUi(); });
        QObject::connect(_yawSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         [this](double) { syncCameraFromUi(); });
        QObject::connect(_pitchSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         [this](double) { syncCameraFromUi(); });
        QObject::connect(_fovSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                         [this](double) { syncCameraFromUi(); });

        // Periodically pull camera state and FPS
        auto* timer = new QTimer(main);
        timer->setInterval(50);
        QObject::connect(timer, &QTimer::timeout, [this]() {
            syncUiFromCamera();
            if (_sample && _fpsLabel) {
                float fps = _sample->getFps();
                if (fps > 0.0f) {
                    _fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
                }
            }
        });
        timer->start();

        main->show();
    }
    ~UI() {
        delete _sample;
    }

    void show() {
        _sample->showWindow();
    }


private:
    platform::WindowPtr _window;
    Sample* _sample{nullptr};
    QDoubleSpinBox* _posX{nullptr};
    QDoubleSpinBox* _posY{nullptr};
    QDoubleSpinBox* _posZ{nullptr};
    QDoubleSpinBox* _fovSpin{nullptr};
    QDoubleSpinBox* _yawSpin{nullptr};
    QDoubleSpinBox* _pitchSpin{nullptr};
    QLabel* _fpsLabel{nullptr};
    bool _updatingFromCamera{false};

    static QString loadStyle() {
        namespace fs = std::filesystem;
        auto exeDir = fs::path(qApp->applicationFilePath().toStdString()).parent_path();
        for (auto& candidate : {exeDir / "style.qss", fs::path("style.qss")}) {
            std::ifstream f(candidate);
            if (f) {
                std::ostringstream ss;
                ss << f.rdbuf();
                return QString::fromStdString(ss.str());
            }
        }
        return {};
    }

    void syncCameraFromUi() {
        if (_updatingFromCamera) {
            return;
        }
        if (!_sample) {
            return;
        }
        auto* current = _sample->currentSample();
        if (!current) {
            return;
        }
        if (!_posX || !_posY || !_posZ) {
            return;
        }
        current->setCameraPosition(
            static_cast<float>(_posX->value()),
            static_cast<float>(_posY->value()),
            static_cast<float>(_posZ->value()));
        if (_yawSpin && _pitchSpin) {
            current->setCameraAngles(
                static_cast<float>(_yawSpin->value()),
                static_cast<float>(_pitchSpin->value()));
        }
        if (_fovSpin) {
            current->setCameraFov(static_cast<float>(_fovSpin->value()));
        }
    }

    void syncUiFromCamera() {
        if (!_sample) {
            return;
        }
        auto* current = _sample->currentSample();
        if (!current) {
            return;
        }
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float yaw = 0.0f, pitch = 0.0f;
        float fov = 0.0f;

        _updatingFromCamera = true;
        if (current->getCameraPosition(x, y, z) && _posX && _posY && _posZ) {
            _posX->setValue(x);
            _posY->setValue(y);
            _posZ->setValue(z);
        }
        if (current->getCameraAngles(yaw, pitch) && _yawSpin && _pitchSpin) {
            _yawSpin->setValue(yaw);
            _pitchSpin->setValue(pitch);
        }
        if (current->getCameraFov(fov) && _fovSpin) {
            _fovSpin->setValue(fov);
        }
        _updatingFromCamera = false;
    }
};

} // namespace raum::sample