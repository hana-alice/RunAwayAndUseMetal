#pragma once
#include <optional>
#include <string>
#include "core/math.h"

namespace raum::sample {

struct CameraControlState {
    Vec3f position{0.0f};
    float yawDegrees{0.0f};
    float pitchDegrees{0.0f};
    float verticalFovDegrees{60.0f};
};

class SampleBase {
public:
    virtual ~SampleBase(){};
    virtual void init() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual const std::string& name() = 0;

    // Samples without an editable camera can keep the default implementation.
    virtual std::optional<CameraControlState> cameraControlState() const { return std::nullopt; }
    virtual void applyCameraControlState(const CameraControlState&) {}
};
} // namespace raum::sample
