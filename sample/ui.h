#pragma once
#include <Qt6/QtWidgets/QDockWidget>
#include <Qt6/QtWidgets/QListWidget>
#include <Qt6/QtWidgets/QStatusBar>
#include <Qt6/QtWidgets/QVBoxLayout>
#include <Qt6/QtWidgets/QWidget>
#include "sample.h"
namespace raum::sample {

class UI {
public:
    UI(int argc, char** argv) {
        _sample = new Sample(argc, argv);
        _mainWindow = new QMainWindow();
        _window = _sample->window();

        _tickFunction = platform::TickFunction{[&](std::chrono::milliseconds miliSec) {
            static uint32_t count{0};
            static uint64_t duration{0};
            constexpr auto updateInterval = 60;

            if (count == updateInterval - 1) {
                _mainWindow->statusBar()->showMessage(QString("%1 fps / %2 ms.").arg(1000 / (duration / count)).arg(miliSec.count()));
                duration = 0;
            }
            duration += miliSec.count();
            count = (count + 1) % updateInterval;
            this->show(miliSec);
        }};
        _window->registerPollEvents(&_tickFunction);

        auto* widget = static_cast<QWidget*>(_window->nativeWindow());
        _mainWindow->setCentralWidget(widget);
        _mainWindow->setMinimumWidth(1080);
        _mainWindow->setMinimumHeight(720);
        _mainWindow->statusBar()->setVisible(true);

        auto* controlPanel = new QDockWidget(_mainWindow);
        //        controlPanel->setFloating(true);
        //        _mainWindow->addDockWidget(Qt::LeftDockWidgetArea, controlPanel);
        //        controlPanel->setVisible(true);
        controlPanel->setContentsMargins(0, 0, 0, 0);

        QListWidget* list = new QListWidget(controlPanel);
        controlPanel->setWidget(list);
        for (auto& sample : _sample->samples()) {
            list->addItem(QString::fromStdString(sample->name()));
        }
        auto changeFunc = [&, list](QListWidgetItem* item) {
            _sample->changeSample(list->row(item));
        };
        list->setVisible(true);
        QObject::connect(list, &QListWidget::itemDoubleClicked, changeFunc);
    }
    ~UI() {
        _window->removePollEvent(&_tickFunction);
        delete _sample;
        delete _mainWindow;
    }

    void show(std::chrono::milliseconds miliSec) {
        if (_mainWindow)
            _mainWindow->show();
    }

    void mainLoop() {
        _window->mainLoop();
    }

private:
    platform::WindowPtr _window;
    platform::TickFunction _tickFunction;
    Sample* _sample{nullptr};
    QMainWindow* _mainWindow{nullptr};
};

} // namespace raum::sample