# RunAwayAndUseMetal
https://vulkan-tutorial.com/Introduction

```
git submodule update --init --recursive
cd vcpkg
.\bootstrap-vcpkg.bat
```
using Visual Studio or Clion open this folder direclty, IDEs take care of the next step.
For Clion, you may need to choose cmake preset manually at first launch.
For the first time, you may need to set `VCPKG_MANIFEST_INSTALL` to "ON" in `CMakePresets.json` to properly prepare the built-essentials.


### Most recent work:
pcf shadow map:![pcf_shadowmap](./sample/shadow/pcf3x3.png)
with code snippet:
```cpp
 // shadow buffer upload pass
    {
        auto uploadPass = renderGraph.addCopyPass("shadowCamUpdate");
        auto& shadowEye = _shadowCam->eye();
        const auto& shadowViewMat = shadowEye.inverseAttitude();
        uploadPass.uploadBuffer(&shadowViewMat[0], 64, _shadowVPBuffer, 0);
        const auto& shadowProjMat = shadowEye.projection();
        uploadPass.uploadBuffer(&shadowProjMat[0], 64, _shadowVPBuffer, 64);
        constexpr float invSize[2] = {1.0f / shadowMapWidth, 1.0f / shadowMapHeight};
        uploadPass.uploadBuffer(&invSize[0], 8, _shadowVPBuffer, 128);
    }

    // shadow rendering pass
    {
        auto shadowPass = renderGraph.addRenderPass("shadowMap");
        shadowPass.addColor(_shadowMapRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {1.0f})
            .addDepthStencil(_shadowMapDS, graph::LoadOp::CLEAR, graph::StoreOp::DONT_CARE, graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0, 0);
        auto shadowQ = shadowPass.addQueue("shadowMap");
        shadowQ.setViewport(0, 0, shadowMapWidth, shadowMapHeight, 0.0f, 1.0f)
            .addCamera(_shadowCam.get())
            .addUniformBuffer(_shadowVPBuffer, "Mat");
    }

    // main camera upload pass
    {
        auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");
        auto& eye = _cam->eye();
        auto viewMat = eye.inverseAttitude();
        uploadPass.uploadBuffer(&viewMat[0], 64, _camBuffer, 0);
        const auto& projMat = eye.projection();
        uploadPass.uploadBuffer(&projMat[0], 64, _camBuffer, 64);

        uploadPass.uploadBuffer(&eye.getPosition()[0], 12, _camPose, 0);
        Vec4f color{1.0, 1.0, 1.0, 1.0};
        Vec4f lightPos{3.0, 3.0, 3.0, 1.0};
        uploadPass.uploadBuffer(&lightPos[0], 16, _light, 0);
        uploadPass.uploadBuffer(&color[0], 16, _light, 16);
    }

    // rendering
    {
        auto renderPass = renderGraph.addRenderPass("forward");

        renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.2, 0.4, 0.4, 1.0})
            .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::DONT_CARE, graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0, 0);
        auto queue = renderPass.addQueue("solidColor");

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
            .addCamera(_cam.get())
            .addUniformBuffer(_camBuffer, "Mat")
            .addUniformBuffer(_shadowVPBuffer, "ShadowView")
            .addSampledImage(_shadowMapRT, "shadowMap")
            .addSampler(_shadowSampler, "shadowSampler");
    }
```

### What is going on:
1. Seperate rendering thread for Editor(Human :,)
2. Bistro
3. SSAO - GTAO
4. Soft shadow

TODO:
 - [ ] InputSystem
 - [ ] std::container -> pmr
 - [ ] ECS
 - [ ] Mesh Shader
 - [ ] RayTracing pass
 - [ ] Multi Device Queue
 - [ ] too long to write down

