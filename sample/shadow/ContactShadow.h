#pragma once

#include <array>
#include <string>

#include "../CameraSample.h"

namespace raum::sample {

class ContactShadowSample final : public CameraSample {
public:
    explicit ContactShadowSample(framework::Director* director)
    : CameraSample(director, "ContactShadowSample") {}

private:
    static constexpr uint32_t ShadowMapWidth = 1024;
    static constexpr uint32_t ShadowMapHeight = 1024;

    void onInitialize() override;
    void onRender() override;

    scene::CameraPtr _shadowCamera;
    std::array<std::string, 8> _waveOffsetBuffers;
};

} // namespace raum::sample
