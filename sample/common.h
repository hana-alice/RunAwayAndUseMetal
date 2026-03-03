#pragma once
#include <memory>
#include "RHIDevice.h"
#include "RHISwapchain.h"
namespace raum::sample {

class SampleBase {
public:
    virtual ~SampleBase(){};
    virtual void init() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual const std::string& name() = 0;

    // Optional camera control hooks for UI
    virtual bool getCameraPosition(float& /*x*/, float& /*y*/, float& /*z*/) const { return false; }
    virtual void setCameraPosition(float /*x*/, float /*y*/, float /*z*/) {}

    virtual bool getCameraAngles(float& /*yawDeg*/, float& /*pitchDeg*/) const { return false; }
    virtual void setCameraAngles(float /*yawDeg*/, float /*pitchDeg*/) {}

    virtual bool getCameraFov(float& /*fovDeg*/) const { return false; }
    virtual void setCameraFov(float /*fovDeg*/) {}
};
} // namespace raum::sample