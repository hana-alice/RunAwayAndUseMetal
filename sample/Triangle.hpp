#pragma once
#include "VKBuffer.h"
#include "window.h"
namespace ruam::sample {

using namespace ::raum::rhi;

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
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f};
    uint16_t indices[] = {
        0, 1, 2};

    BufferSourceInfo vertexInfo{};
    vertexInfo.bufferUsage = BufferUsage::VERTEX;
    vertexInfo.data = reinterpret_cast<uint8_t*>(&vertices[0]);

    // VertexBuffer vb(,);
}
Triangle::~Triangle() {
}

void Triangle::show(){};

} // namespace ruam::sample
