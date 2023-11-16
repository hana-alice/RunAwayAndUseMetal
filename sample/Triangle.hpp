#pragma once
#include "window.h"

namespace ruam_sample {

class Triangle {
public:
    Triangle();
    ~Triangle();
    Triangle(const Triangle&) = delete;
    Triangle(Triangle&&) = delete;

    void show();

private:
    platform::NativeWindow* _window{nullptr};
};

Triangle::Triangle() {
    _window = new platform::NativeWindow(800, 600);
}

Triangle::~Triangle() {
    delete _window;
}

void Triangle::show() {
    _window->mainLoop();
};

} // namespace ruam_sample
