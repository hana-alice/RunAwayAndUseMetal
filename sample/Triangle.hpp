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
};

Triangle::Triangle() {
}

Triangle::~Triangle() {
}

void Triangle::show(){};

} // namespace ruam_sample
