#pragma once
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QWidget>
#include <QWindow>
#include <QResizeEvent>
#include <QEvent>
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
        _sample = new Sample(argc, argv);
        _window = _sample->window();

        QWidget* engineWidget = static_cast<QWidget*>(_window->container());
        engineWidget->setMinimumSize(static_cast<int>(s_width), static_cast<int>(s_height));
        auto* aspectContainer = new AspectRatioContainer(
            engineWidget,
            static_cast<float>(s_width) / static_cast<float>(s_height));
        auto* root = new QWidget;
        auto* layout = new QHBoxLayout(root);
        layout->setContentsMargins(0,0,0,0);

        layout->addWidget(aspectContainer, /*stretch*/ 1);

        auto* panel = new QWidget;
        auto* panelLayout = new QVBoxLayout(panel);

        panelLayout->addWidget(new QPushButton("Reload"));
        panelLayout->addWidget(new QSlider(Qt::Horizontal));
        panelLayout->addStretch(1);

        layout->addWidget(panel, /*stretch*/ 0);

        root->setLayout(layout);

        auto* main = new QMainWindow;
        main->setCentralWidget(root);

        main->menuBar()->addMenu("File");
        main->statusBar()->showMessage("Ready");

        const int initialW = static_cast<int>(s_width) + 300;
        const int initialH = static_cast<int>(s_height);
        main->resize(initialW, initialH);
        main->setMinimumSize(initialW, initialH);

        raum_info("sz: {} {} {} {} {} {}",
            engineWidget->size().width(), engineWidget->size().height(),
            root->size().width(), root->size().height(),
            main->size().width(), main->size().height()
        );

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
};

} // namespace raum::sample