#pragma once
#include <QLayout>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QWidget>
#include <QWindow>
#include "sample.h"
namespace raum::sample {

class UI {
public:
    UI(int argc, char** argv) {
        _sample = new Sample(argc, argv);
        _window = _sample->window();

        QWidget* widget = static_cast<QWidget*>(_window->container());
        auto* root = new QWidget;
        auto* layout = new QHBoxLayout(root);
        layout->setContentsMargins(0,0,0,0);

        layout->addWidget(widget, /*stretch*/ 1);

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

        raum_info("sz: {} {} {} {} {} {}",
            widget->size().width(), widget->size().height(),
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