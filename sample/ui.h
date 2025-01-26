#pragma once
#include "sample.h"
namespace raum::sample {

class UI {
public:
    UI(int argc, char** argv) {
        _sample = new Sample(argc, argv);
        _window = _sample->window();

        std::jthread j;

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