#pragma once
#include <memory>
#include "RHIDevice.h"
#include "RHISwapchain.h"
namespace raum::sample {

class SampleBase {
public:
    virtual ~SampleBase(){};
    virtual void show() = 0;
};
} // namespace raum::sample