# Sample infrastructure

Use `SampleBase` for lifecycle and resource management, or `CameraSample` when the sample needs a perspective fly camera.

- `onInitialize()` runs once. Load scene data and declare persistent resources here.
- `onRender()` runs once per active frame. Build the frame render graph here.
- `onActivated()` / `onDeactivated()` own callbacks or render tasks that must not run in the background.
- `resource("name")` returns a stable `<sample>/<name>` key. Use it whenever a pass refers to a resource.
- `loadScene(path)` namespaces and tracks every loaded scene node, so switching samples cannot overwrite or keep rendering another sample's models.
- `ensureViewportImage()` plus `resizeViewportResources()` handles swapchain-sized images.
- `ensureCameraResources()`, `uploadCamera()`, `ensureLightResource()` and `uploadLight()` cover the common uniform path.

Minimal camera sample:

```cpp
class TriangleSample final : public CameraSample {
public:
    explicit TriangleSample(framework::Director* director)
        : CameraSample(director, "Triangle") {}

private:
    void onInitialize() override {
        loadScene(utils::resourceDirectory() / "models" / "triangle.gltf");
        createPerspectiveCamera();
        enableFlyCamera();
        ensureSwapchain("present");
        ensureViewportImage(
            "depth",
            rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT,
            rhi::Format::D24_UNORM_S8_UINT);
        ensureCameraResources(false);
    }

    void onRender() override {
        auto& graph = pipeline().renderGraph();
        resizeViewportResources();

        auto upload = graph.addCopyPass("cameraUpdate");
        uploadCamera(upload, false);

        auto pass = graph.addRenderPass("forward");
        pass.addColor(resource("present"), graph::LoadOp::CLEAR, graph::StoreOp::STORE, {});
        pass.addDepthStencil(resource("depth"), graph::LoadOp::CLEAR, graph::StoreOp::STORE,
                             graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0f, 0);
        pass.addQueue("default")
            .setViewport(0, 0, viewportWidth(), viewportHeight(), 0.0f, 1.0f)
            .addCamera(&camera())
            .addUniformBuffer(cameraBuffer(), "Mat");
    }
};
```

Add the sample instance to `_samples` in `sample.h`; initialization remains lazy when switching.
