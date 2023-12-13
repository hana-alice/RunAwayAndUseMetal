#pragma once
#include "RHIBuffer.h"
#include "window.h"
#include "RHIDevice.h"
namespace ruam::sample {

using namespace ::raum::rhi;

class Triangle {
public:
    Triangle(RHIDevice* device);
    ~Triangle();
    Triangle(const Triangle&) = delete;
    Triangle(Triangle&&) = delete;

    void show();

private:
    RHIDevice* _device{nullptr};
};

Triangle::Triangle(RHIDevice* device) :_device(device) {
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.0f, 0.5f, 0.0f,
    };
    uint16_t indices[] = {
        0, 1, 2,
    };

    BufferSourceInfo vertexInfo{};
    vertexInfo.bufferUsage = BufferUsage::VERTEX;
    vertexInfo.data = &vertices[0];
    vertexInfo.size = sizeof(vertices);

    BufferSourceInfo indexInfo{};
    indexInfo.bufferUsage = BufferUsage::INDEX;
    indexInfo.data = &indices[0];
    indexInfo.size = sizeof(indices);

    RHIBuffer* vertBuffer = device->createBuffer(vertexInfo);
    RHIBuffer* indexBuffer = device->createBuffer(indexInfo);
    // VertexBuffer vb(,);
}
Triangle::~Triangle() {
}

void Triangle::show(){
};

} // namespace ruam::sample
