#pragma once
#include "window.h"

namespace ruam::sample {

class Triangle {
public:
    Triangle();
    ~Triangle();
    Triangle(const Triangle&) = delete;
    Triangle(Triangle&&) = delete;

    void show();

private:
};

Triangle::Triangle() {
}

Triangle::~Triangle() {
}

void Triangle::show(){};

} // namespace ruam::sample
