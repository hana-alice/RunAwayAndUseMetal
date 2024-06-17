#include "Skybox.h"
#include "GeometryMesh.h"
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHIRenderEncoder.h"
#include "RHIUtils.h"

namespace raum::asset {

constexpr auto vertSource = R"(

    layout (location = 0) in vec3 aPos;
    layout (location = 0) out vec3 WorldPos;

    layout( push_constant ) uniform constants
    {
        mat4 modelMat;
    } Mat;

    void main()
    {
        WorldPos = aPos;
        gl_Position = Mat.modelMat * vec4(WorldPos, 1.0);
    } 
)";

constexpr auto fragSource = R"(

    layout (location = 0) out vec4 FragColor;
    layout (location = 0) in vec3 WorldPos;

    layout (set = 1, binding = 0) uniform texture2D equirectangularMap;
    layout (set = 1, binding = 1) uniform sampler linearSampler;

    const vec2 invAtan = vec2(0.1591, 0.3183);
    vec2 SampleSphericalMap(vec3 v)
    {
        vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
        uv *= invAtan;
        uv += 0.5;
        return uv;
    }

    void main()
    {
        vec2 uv = SampleSphericalMap(normalize(WorldPos));
        uv.y = uv.y;
        vec3 color = texture(sampler2D(equirectangularMap, linearSampler), uv).rgb;

        FragColor = vec4(color, 1.0);
    }
)";

constexpr auto diffuseIrradianceSource = R"(
layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec3 WorldPos;

layout (set = 1, binding = 0) uniform textureCube environmentMap;
layout (set = 1, binding = 1) uniform sampler linearSampler;

void main() {
    const float PI = 3.14159265359;
    vec3 normal = normalize(WorldPos);
    vec3 irradiance = vec3(0.0);
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up         = cross(normal, right);

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            vec3 envColor = texture(samplerCube(environmentMap, linearSampler), sampleVec).rgb;
            vec3 mapped = vec3(1.0) - exp(-envColor * 1.0f);
            mapped = pow(mapped, vec3(1.0/2.2)); 

            irradiance += mapped * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    FragColor = vec4(irradiance, 1.0f);
}

)";

constexpr uint32_t resolution = 4096;

void sampleEquirectangular(rhi::BufferPtr data,
                           uint32_t width,
                           uint32_t height,
                           rhi::CommandBufferPtr cmdBuffer,
                           rhi::DevicePtr device,
                           graph::ShaderGraph& shaderGraph,
                           rhi::ImagePtr cubeImage,
                           scene::MeshRendererPtr meshRenderer) {
    rhi::ImageInfo imgInfo{
        .usage = rhi::ImageUsage::TRANSFER_DST | rhi::ImageUsage::SAMPLED,
        .format = rhi::Format::RGBA32_SFLOAT,
        .extent = {width, height, 1},
    };
    auto image = rhi::ImagePtr(device->createImage(imgInfo));

    rhi::ImageBarrierInfo transferDstBarrier{
        .image = image.get(),
        .dstStage = rhi::PipelineStage::TRANSFER,
        .newLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL,
        .dstAccessFlag = rhi::AccessFlags::TRANSFER_WRITE,
        .range = {
            .sliceCount = 1,
            .mipCount = 1,
        }};
    cmdBuffer->appendImageBarrier(transferDstBarrier);
    cmdBuffer->applyBarrier({});

    auto blit = rhi::BlitEncoderPtr(cmdBuffer->makeBlitEncoder());
    rhi::BufferImageCopyRegion region{
        .bufferSize = data->info().size,
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageAspect = rhi::AspectMask::COLOR,
        .imageExtent = {
            width,
            height,
            1,
        },
    };
    blit->copyBufferToImage(data.get(),
                            image.get(),
                            rhi::ImageLayout::TRANSFER_DST_OPTIMAL,
                            &region,
                            1);
    rhi::ImageBarrierInfo imgBarrierInfo{
        .image = image.get(),
        .srcStage = rhi::PipelineStage::TRANSFER,
        .dstStage = rhi::PipelineStage::FRAGMENT_SHADER,
        .oldLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL,
        .newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        .srcAccessFlag = rhi::AccessFlags::TRANSFER_WRITE,
        .dstAccessFlag = rhi::AccessFlags::SHADER_READ,
        .range = {
            .sliceCount = 1,
            .mipCount = 1,
        }};
    cmdBuffer->appendImageBarrier(imgBarrierInfo);
    cmdBuffer->applyBarrier({});

    rhi::ImageViewInfo imgViewInfo{
        .image = image.get(),
        .range = {
            .sliceCount = 1,
            .mipCount = 1,
        },
        .format = imgInfo.format,
    };

    scene::Texture texture{
        .name = "equirectangularMap",
        .texture = image,
        .textureView = rhi::ImageViewPtr(device->createImageView(imgViewInfo)),
    };

    scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("builtin-skybox_bake");
    auto mat = matTemplate->instantiate("builtin-skybox_bake", scene::MaterialType::CUSTOM);
    mat->add(texture);
    scene::Sampler sampler{
        "linearSampler",
        rhi::SamplerInfo{
            .magFilter = rhi::Filter::LINEAR,
            .minFilter = rhi::Filter::LINEAR,
        },
    };
    mat->add(sampler);

    boost::container::flat_map<std::string_view, uint32_t> bindings = {
        {"equirectangularMap", 0},
        {"linearSampler", 1}};

    rhi::DescriptorSetLayoutInfo descLayoutInfo;
    descLayoutInfo.descriptorBindings.emplace_back(0, rhi::DescriptorType::SAMPLED_IMAGE, 1, rhi::ShaderStage::FRAGMENT);
    descLayoutInfo.descriptorBindings.emplace_back(1, rhi::DescriptorType::SAMPLER, 1, rhi::ShaderStage::FRAGMENT);

    auto layout = rhi::getOrCreateDescriptorSetLayout(descLayoutInfo, device);

    const auto& meshData = meshRenderer->mesh()->meshData();
    scene::TechniquePtr tech = std::make_shared<scene::Technique>(mat, "default");
    tech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
    auto& ds = tech->depthStencilInfo();
    ds.depthTestEnable = false;
    ds.depthWriteEnable = true;
    ds.depthCompareOp = rhi::CompareOp::LESS_OR_EQUAL;
    auto& bs = tech->blendInfo();
    bs.attachmentBlends.emplace_back();

    rhi::PipelineLayoutInfo layoutInfo{};
    layoutInfo.pushConstantRanges = {
        {.size = 16 * sizeof(float)}};
    layoutInfo.setLayouts.emplace_back(layout.get()); // 0
    layoutInfo.setLayouts.emplace_back(layout.get()); // 1
    layoutInfo.setLayouts.emplace_back(layout.get()); // 2
    layoutInfo.setLayouts.emplace_back(layout.get()); // 3
    auto pplLayout = rhi::getOrCreatePipelineLayout(layoutInfo, device);

    rhi::RenderPassInfo rpInfo{};
    auto& attachment = rpInfo.attachments.emplace_back();
    attachment.format = rhi::Format::RGBA32_SFLOAT;
    attachment.initialLayout = rhi::ImageLayout::UNDEFINED;
    attachment.finalLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    auto& subpass = rpInfo.subpasses.emplace_back();
    subpass.colors.emplace_back(0, rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
    auto renderPass = rhi::RenderPassPtr(device->createRenderPass(rpInfo));

    boost::container::flat_map<rhi::ShaderStage, std::string> shaders = {
        {rhi::ShaderStage::VERTEX, vertSource},
        {rhi::ShaderStage::FRAGMENT, fragSource},
    };
    tech->bake(renderPass, pplLayout, meshData.vertexLayout, shaders, bindings, layout, device);
    meshRenderer->addTechnique(tech);
    meshRenderer->prepare({}, layout, device);
    meshRenderer->update(cmdBuffer);

    std::vector<rhi::ImageViewPtr> views;
    std::vector<rhi::FrameBufferPtr> frameBuffers;
    std::vector<Mat4> modelMats;
    std::array<float, 6> angles = {
        glm::half_pi<float>(),
        -glm::half_pi<float>(),
        -glm::half_pi<float>(),
        glm::half_pi<float>(),
        0.0f,
        glm::pi<float>(),
    };
    std::array<Vec3f, 6> axis = {
        Vec3f{0.0f, 1.0f, 0.0f},
        Vec3f{0.0f, 1.0f, 0.0f},
        Vec3f{1.0f, 0.0f, 0.0f},
        Vec3f{1.0f, 0.0f, 0.0f},
        Vec3f{0.0f, 0.0f, 1.0f},
        Vec3f{0.0f, 0.0f, 1.0f},
    };
    for (uint32_t i = 0; i < 6; ++i) {
        rhi::ImageViewInfo arrViewInfo{
            .type = rhi::ImageViewType::IMAGE_VIEW_2D,
            .image = cubeImage.get(),
            .range = {
                .firstSlice = i,
                .sliceCount = 1,
                .mipCount = 1,
            },
            .format = cubeImage->info().format,
        };
        auto view = views.emplace_back(rhi::ImageViewPtr(device->createImageView(arrViewInfo)));

        rhi::FrameBufferInfo fbInfo{
            .renderPass = renderPass.get(),
            .images = {view.get()},
            .width = resolution,
            .height = resolution,
            .layers = 1,
        };
        frameBuffers.emplace_back(rhi::FrameBufferPtr(device->createFrameBuffer(fbInfo)));

        auto matrix = glm::rotate(Mat4(1.0f), angles[i], axis[i]);
        modelMats.emplace_back(matrix);
    }

    auto renderEncoder = rhi::RenderEncoderPtr(cmdBuffer->makeRenderEncoder());
    rhi::ClearValue clear = {1.0f, 1.0f, 1.0f, 1.0f};
    rhi::Rect2D rect{0, 0, resolution, resolution};
    for (uint32_t i = 0; i < 6; ++i) {
        rhi::RenderPassBeginInfo beginInfo{
            .renderPass = renderPass.get(),
            .frameBuffer = frameBuffers[i].get(),
            .renderArea = rect,
            .clearColors = &clear,
        };
        renderEncoder->beginRenderPass(beginInfo);
        renderEncoder->setViewport({rect});
        renderEncoder->setScissor(rect);
        renderEncoder->bindPipeline(tech->pipelineState().get());
        renderEncoder->bindDescriptorSet(mat->bindGroup()->descriptorSet().get(), 1, nullptr, 0);
        renderEncoder->pushConstants(rhi::ShaderStage::VERTEX, 0, &modelMats[i], 16 * sizeof(float));
        renderEncoder->bindVertexBuffer(meshData.vertexBuffer.buffer.get(), 0);
        renderEncoder->draw(meshData.vertexCount, 1, 0, 0);
        renderEncoder->endRenderPass();
    }
    cmdBuffer->onComplete([views, frameBuffers, renderPass, image, mat]() mutable {
        mat.reset();
    });
}

scene::MeshRendererPtr initModel(scene::Model& model, rhi::DevicePtr device) {
    auto mesh = std::make_shared<scene::Mesh>();
    auto& meshData = mesh->meshData();
    rhi::BufferSourceInfo info{
        .bufferUsage = rhi::BufferUsage::VERTEX | rhi::BufferUsage::TRANSFER_DST,
        .size = static_cast<uint32_t>(scene::Cube::vertices.size() * sizeof(float)),
        .data = scene::Cube::vertices.data(),
    };

    meshData.vertexBuffer.buffer = rhi::BufferPtr(device->createBuffer(info));
    meshData.shaderAttrs = scene::ShaderAttribute::POSITION | scene::ShaderAttribute::UV;
    meshData.vertexLayout.vertexBufferAttrs.emplace_back(0, static_cast<uint32_t>(5 * sizeof(float)));
    meshData.vertexLayout.vertexAttrs.emplace_back(0, 0, rhi::Format::RGB32_SFLOAT);
    meshData.vertexLayout.vertexAttrs.emplace_back(1, 0, rhi::Format::RG32_SFLOAT, static_cast<uint32_t>(3 * sizeof(float)));
    meshData.vertexCount = 36;

    scene::Cube::vertexBuffer = meshData.vertexBuffer;

    auto meshRenderer = model.meshRenderers().emplace_back(std::make_shared<scene::MeshRenderer>(mesh));
    return meshRenderer;
}

void bakeIrradiance(rhi::ImagePtr& diffuse,
                    rhi::ImageViewPtr& diffuseView,
                    rhi::ImageViewPtr cubeView,
                    rhi::CommandBufferPtr cmdBuffer,
                    rhi::DevicePtr device) {
    rhi::ImageInfo diffuseInfo{
        .usage = rhi::ImageUsage::SAMPLED | rhi::ImageUsage::COLOR_ATTACHMENT,
        .imageFlag = rhi::ImageFlag::CUBE_COMPATIBLE,
        .format = rhi::Format::RGBA32_SFLOAT,
        .sliceCount = 6,
        .extent = {32, 32, 1},
    };
    diffuse = rhi::ImagePtr(device->createImage(diffuseInfo));

    rhi::ImageViewInfo dvInfo{
        .type = rhi::ImageViewType::IMAGE_VIEW_CUBE,
        .image = diffuse.get(),
        .range = {
            .sliceCount = 6,
            .mipCount = 1,
        },
        .format = diffuseInfo.format,
    };
    diffuseView = rhi::ImageViewPtr(device->createImageView(dvInfo));

    rhi::RenderPassInfo rpInfo{};
    auto& attachment = rpInfo.attachments.emplace_back();
    attachment.format = rhi::Format::RGBA32_SFLOAT;
    attachment.initialLayout = rhi::ImageLayout::UNDEFINED;
    attachment.finalLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    auto& subpass = rpInfo.subpasses.emplace_back();
    subpass.colors.emplace_back(0, rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
    auto renderPass = rhi::RenderPassPtr(device->createRenderPass(rpInfo));

    rhi::DescriptorSetLayoutInfo layoutInfo;
    auto& imgBindinng = layoutInfo.descriptorBindings.emplace_back();
    imgBindinng.binding = 0;
    imgBindinng.count = 1;
    imgBindinng.type = rhi::DescriptorType::SAMPLED_IMAGE;
    imgBindinng.visibility = rhi::ShaderStage::FRAGMENT;
    auto& samplerBinding = layoutInfo.descriptorBindings.emplace_back();
    samplerBinding.binding = 1;
    samplerBinding.count = 1;
    samplerBinding.type = rhi::DescriptorType::SAMPLER;
    samplerBinding.visibility = rhi::ShaderStage::FRAGMENT;
    auto layout = rhi::DescriptorSetLayoutPtr(device->createDescriptorSetLayout(layoutInfo));

    auto layout0 = rhi::DescriptorSetLayoutPtr(device->createDescriptorSetLayout(rhi::DescriptorSetLayoutInfo{}));
    rhi::PipelineLayoutInfo pplLayoutInfo;
    pplLayoutInfo.setLayouts.emplace_back(layout0.get());
    pplLayoutInfo.setLayouts.emplace_back(layout.get());
    pplLayoutInfo.pushConstantRanges = {
        {.size = 16 * sizeof(float)}};
    auto pplLayout = rhi::PipelineLayoutPtr(device->createPipelineLayout(pplLayoutInfo));

    rhi::DescriptorSetInfo setInfo;
    setInfo.layout = layout.get();
    auto& imgBd = setInfo.bindingInfos.imageBindings.emplace_back();
    imgBd.type = rhi::DescriptorType::SAMPLED_IMAGE;
    imgBd.binding = 0;
    imgBd.arrayElement = 0;
    imgBd.imageViews.emplace_back(rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL, cubeView.get());
    auto& samplerBd = setInfo.bindingInfos.samplerBindings.emplace_back();
    samplerBd.arrayElement = 0;
    samplerBd.binding = 1;
    rhi::SamplerInfo samplerInfo{
        .magFilter = rhi::Filter::LINEAR,
        .minFilter = rhi::Filter::LINEAR,
    };
    samplerBd.samplers.emplace_back(device->getSampler(samplerInfo));

    const auto& descPoolInfo = rhi::makeDescriptorPoolInfo(pplLayout->info().setLayouts);
    auto descPool = rhi::DescriptorPoolPtr(device->createDescriptorPool(descPoolInfo));
    auto descSet = rhi::DescriptorSetPtr(descPool->makeDescriptorSet(setInfo));
    descSet->updateImage(imgBd);
    descSet->updateSampler(samplerBd);

    rhi::GraphicsPipelineInfo pplInfo;
    pplInfo.colorBlendInfo.attachmentBlends.emplace_back();
    pplInfo.depthStencilInfo.depthTestEnable = false;
    pplInfo.depthStencilInfo.depthWriteEnable = false;
    pplInfo.primitiveType = rhi::PrimitiveType::TRIANGLE_LIST;
    pplInfo.vertexLayout = scene::Cube::vertexLayout;

    std::string vsrc{"#version 450 core\n"};
    vsrc.append(vertSource);
    rhi::ShaderSourceInfo vertSourceInfo{
        "ndc-pos",
        {rhi::ShaderStage::VERTEX, vsrc},
    };
    auto vertShader = rhi::ShaderPtr(device->createShader(vertSourceInfo));
    std::string fsrc{"#version 450 core\n"};
    fsrc.append(diffuseIrradianceSource);
    rhi::ShaderSourceInfo fragSourceInfo{
        "diffuseIrradianceSource",
        {rhi::ShaderStage::FRAGMENT, fsrc},
    };
    auto fragShader = rhi::ShaderPtr(device->createShader(fragSourceInfo));
    pplInfo.shaders.emplace_back(vertShader.get());
    pplInfo.shaders.emplace_back(fragShader.get());
    pplInfo.pipelineLayout = pplLayout.get();
    pplInfo.renderPass = renderPass.get();
    auto pso = rhi::GraphicsPipelinePtr(device->createGraphicsPipeline(pplInfo));

    auto renderEncoder = rhi::RenderEncoderPtr(cmdBuffer->makeRenderEncoder());
    rhi::ClearValue clear = {1.0f, 1.0f, 1.0f, 1.0f};
    rhi::Rect2D rect{0, 0, 32, 32};

    std::vector<rhi::ImageViewPtr> views;
    std::vector<rhi::FrameBufferPtr> frameBuffers;
    std::vector<Mat4> modelMats;
    std::array<float, 6> angles = {
        glm::half_pi<float>(),
        -glm::half_pi<float>(),
        -glm::half_pi<float>(),
        glm::half_pi<float>(),
        0.0f,
        glm::pi<float>(),
    };
    std::array<Vec3f, 6> axis = {
        Vec3f{0.0f, 1.0f, 0.0f},
        Vec3f{0.0f, 1.0f, 0.0f},
        Vec3f{1.0f, 0.0f, 0.0f},
        Vec3f{1.0f, 0.0f, 0.0f},
        Vec3f{0.0f, 0.0f, 1.0f},
        Vec3f{0.0f, 0.0f, 1.0f},
    };
    for (uint32_t i = 0; i < 6; ++i) {
        rhi::ImageViewInfo arrViewInfo{
            .type = rhi::ImageViewType::IMAGE_VIEW_2D,
            .image = diffuse.get(),
            .range = {
                .firstSlice = i,
                .sliceCount = 1,
                .mipCount = 1,
            },
            .format = diffuse->info().format,
        };
        auto view = views.emplace_back(rhi::ImageViewPtr(device->createImageView(arrViewInfo)));

        rhi::FrameBufferInfo fbInfo{
            .renderPass = renderPass.get(),
            .images = {view.get()},
            .width = 32,
            .height = 32,
            .layers = 1,
        };
        frameBuffers.emplace_back(rhi::FrameBufferPtr(device->createFrameBuffer(fbInfo)));

        auto matrix = glm::rotate(Mat4(1.0f), angles[i], axis[i]);
        modelMats.emplace_back(matrix);
    }
    for (uint32_t i = 0; i < 6; ++i) {
        rhi::RenderPassBeginInfo beginInfo{
            .renderPass = renderPass.get(),
            .frameBuffer = frameBuffers[i].get(),
            .renderArea = rect,
            .clearColors = &clear,
        };
        renderEncoder->beginRenderPass(beginInfo);
        renderEncoder->setViewport({rect});
        renderEncoder->setScissor(rect);
        renderEncoder->bindPipeline(pso.get());
        renderEncoder->bindDescriptorSet(descSet.get(), 1, nullptr, 0);
        renderEncoder->bindVertexBuffer(scene::Cube::vertexBuffer.buffer.get(), 0);
        renderEncoder->pushConstants(rhi::ShaderStage::VERTEX, 0, &modelMats[i], 16 * sizeof(float));
        renderEncoder->draw(scene::Cube::vertexCount, 1, 0, 0);
        renderEncoder->endRenderPass();
    }

    cmdBuffer->onComplete([renderPass,
                           layout,
                           descSet,
                           pplLayout,
                           vertShader,
                           fragShader,
                           pso,
                           frameBuffers,
                           views,
                           descPool]() mutable {
        descSet.reset();
        descPool.reset();
        });
}

Skybox::Skybox(rhi::BufferPtr data,
               uint32_t width,
               uint32_t height,
               rhi::CommandBufferPtr cmdBuffer,
               rhi::DevicePtr device,
               graph::ShaderGraph& shaderGraph) {
    _model = std::make_shared<scene::Model>();
    auto meshRenderer = initModel(*_model, device);

    rhi::ImageInfo cubeImageInfo{
        .type = rhi::ImageType::IMAGE_2D,
        .usage = rhi::ImageUsage::COLOR_ATTACHMENT | rhi::ImageUsage::SAMPLED,
        .imageFlag = rhi::ImageFlag::CUBE_COMPATIBLE,
        .intialLayout = rhi::ImageLayout::UNDEFINED,
        .format = rhi::Format::RGBA32_SFLOAT,
        .sliceCount = 6,
        .mipCount = 1,
        .extent = {
            resolution,
            resolution,
            1,
        }};
    _cubeImage = rhi::ImagePtr(device->createImage(cubeImageInfo));

    sampleEquirectangular(data, width, height, cmdBuffer, device, shaderGraph, _cubeImage, meshRenderer);
    init(meshRenderer, cmdBuffer, device);
    bakeIrradiance(_diffuse, _diffuseView, _cubeView, cmdBuffer, device);
}

void Skybox::init(scene::MeshRendererPtr meshRenderer, rhi::CommandBufferPtr cmdBuffer, rhi::DevicePtr device) {
    rhi::ImageViewInfo cubeViewInfo{
        .type = rhi::ImageViewType::IMAGE_VIEW_CUBE,
        .image = _cubeImage.get(),
        .range = {
            .sliceCount = 6,
            .mipCount = 1,
        },
        .format = _cubeImage->info().format,
    };
    _cubeView = rhi::ImageViewPtr(device->createImageView(cubeViewInfo));

    scene::Texture cube{
        .name = "environmentMap",
        .texture = _cubeImage,
        .textureView = _cubeView,
    };
    scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/skybox");
    auto skyboxMat = matTemplate->instantiate("asset/layout/skybox", scene::MaterialType::CUSTOM);
    skyboxMat->add(cube);
    scene::Sampler sampler{
        "linearSampler",
        rhi::SamplerInfo{
            .magFilter = rhi::Filter::LINEAR,
            .minFilter = rhi::Filter::LINEAR,
        },
    };
    skyboxMat->add(sampler);

    scene::TechniquePtr skyboxTech = std::make_shared<scene::Technique>(skyboxMat, "default");
    skyboxTech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
    auto& ds = skyboxTech->depthStencilInfo();
    ds.depthTestEnable = true;
    ds.depthWriteEnable = false;
    ds.depthCompareOp = rhi::CompareOp::LESS_OR_EQUAL;
    auto& bs = skyboxTech->blendInfo();
    bs.attachmentBlends.emplace_back();

    meshRenderer->removeTechnique(0);
    meshRenderer->addTechnique(skyboxTech);
    meshRenderer->setVertexInfo(0, 36, 0);
}

rhi::ImagePtr Skybox::diffuseIrradianceImage() const {
    return _diffuse;
}

rhi::ImageViewPtr Skybox::diffuseIrradianceView() const {
    return _diffuseView;
}

scene::ModelPtr Skybox::model() const {
    return _model;
}

} // namespace raum::asset