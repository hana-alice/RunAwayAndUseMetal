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
        mat4 projMat;
    } Mat;

    void main()
    {
        WorldPos = aPos;
        gl_Position = Mat.projMat * Mat.modelMat * vec4(WorldPos, 1.0);
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
        uv.y = 1.0 - uv.y;
        return uv;
    }

    void main()
    {
        vec3 pos = normalize(WorldPos);
        vec2 uv = SampleSphericalMap(vec3(pos.x, pos.y, pos.z));
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

constexpr auto specularPrefilterSource = R"(

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec3 localPos;

layout (set = 1, binding = 0) uniform textureCube environmentMap;
layout (set = 1, binding = 1) uniform sampler linearSampler;
layout( push_constant ) uniform constants
{
    layout(offset = 128) float val;
} Roughness;

const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a*a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

void main() {
    vec3 N = normalize(localPos);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);
    for(uint i = 0; i < SAMPLE_COUNT; ++i) {
        vec2 xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(xi, N, Roughness.val);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            // sample from the environment's mip level based on roughness/pdf
            float D   = DistributionGGX(N, H, Roughness.val);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 

            float resolution = 1024.0; // resolution of source cubemap (per face)
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = Roughness.val == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            vec3 envColor = textureLod(samplerCube(environmentMap, linearSampler), L, mipLevel).rgb;
            vec3 mapped = vec3(1.0) - exp(-envColor * 1.0f);
            mapped = pow(mapped, vec3(1.0/2.2));

            prefilteredColor += mapped * NdotL;

            totalWeight      += NdotL;
        }
    }
    prefilteredColor /= totalWeight;
    FragColor = vec4(prefilteredColor, 1.0);
}
)";

std::array<float, 6> angles = {
    -glm::half_pi<float>(),
    glm::half_pi<float>(),
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
    Vec3f{0.0f, 1.0f, 0.0f},
    Vec3f{0.0f, 1.0f, 0.0f},
};

auto renderCube(rhi::ImagePtr cubeImage,
                uint32_t resolution,
                uint32_t mipLevel,
                rhi::GraphicsPipelinePtr pso,
                rhi::DescriptorSetPtr desc0,
                rhi::DescriptorSetPtr desc1,
                void* fragConstants,
                uint32_t cSize,
                rhi::RenderPassPtr renderPass,
                rhi::CommandBufferPtr cmdBuffer,
                rhi::DevicePtr device) {
    std::vector<rhi::ImageViewPtr> views;
    std::vector<rhi::FrameBufferPtr> frameBuffers;
    std::vector<Mat4> modelMats = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    };
    uint32_t len = static_cast<uint32_t>(resolution * pow(0.5, mipLevel));
    for (uint32_t i = 0; i < 6; ++i) {
        rhi::ImageViewInfo arrViewInfo{
            .type = rhi::ImageViewType::IMAGE_VIEW_2D,
            .image = cubeImage.get(),
            .range = {
                .firstSlice = i,
                .sliceCount = 1,
                .firstMip = mipLevel,
                .mipCount = 1,
            },
            .format = cubeImage->info().format,
        };
        auto view = views.emplace_back(rhi::ImageViewPtr(device->createImageView(arrViewInfo)));

        rhi::FrameBufferInfo fbInfo{
            .renderPass = renderPass.get(),
            .images = {view.get()},
            .width = len,
            .height = len,
            .layers = 1,
        };
        frameBuffers.emplace_back(rhi::FrameBufferPtr(device->createFrameBuffer(fbInfo)));
    }

    auto captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    auto renderEncoder = rhi::RenderEncoderPtr(cmdBuffer->makeRenderEncoder(rhi::RenderEncoderHint::NO_FLIP_Y));
    rhi::ClearValue clear = {1.0f, 1.0f, 1.0f, 1.0f};
    rhi::Rect2D rect{0, 0, len, len};
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
        if (desc0) {
            renderEncoder->bindDescriptorSet(desc0.get(), 0, nullptr, 0);
        }
        if (desc1) {
            renderEncoder->bindDescriptorSet(desc1.get(), 1, nullptr, 0);
        }
        renderEncoder->pushConstants(rhi::ShaderStage::VERTEX, 0, &modelMats[i], 16 * sizeof(float));
        renderEncoder->pushConstants(rhi::ShaderStage::VERTEX, 16 * sizeof(float), &captureProjection[0], 16 * sizeof(float));
        if (fragConstants) {
            renderEncoder->pushConstants(rhi::ShaderStage::FRAGMENT, 32 * sizeof(float), fragConstants, cSize);
        }
        renderEncoder->bindVertexBuffer(scene::Cube::vertexBuffer.buffer.get(), 0);
        renderEncoder->draw(scene::Cube::vertexCount, 1, 0, 0);
        renderEncoder->endRenderPass();
    }
    return std::make_tuple(views, frameBuffers);
}

auto generateResources(rhi::ImageViewPtr imgView,
                       const std::string& vert,
                       const std::string_view vertPath,
                       const std::string& frag,
                       const std::string_view fragPath,
                       uint32_t fSize,
                       rhi::DevicePtr device) {
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
        {.size = 32 * sizeof(float)}};
    if (fSize) {
        pplLayoutInfo.pushConstantRanges.emplace_back(rhi::ShaderStage::FRAGMENT, static_cast<uint32_t>(32 * sizeof(float)), fSize);
    }
    auto pplLayout = rhi::PipelineLayoutPtr(device->createPipelineLayout(pplLayoutInfo));

    rhi::DescriptorSetInfo setInfo;
    setInfo.layout = layout.get();
    auto& imgBd = setInfo.bindingInfos.imageBindings.emplace_back();
    imgBd.type = rhi::DescriptorType::SAMPLED_IMAGE;
    imgBd.binding = 0;
    imgBd.arrayElement = 0;
    imgBd.imageViews.emplace_back(rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL, imgView.get());
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
    rhi::DescriptorSetInfo set0Info;
    set0Info.layout = layout0.get();
    auto descSet0 = rhi::DescriptorSetPtr(descPool->makeDescriptorSet(set0Info));

    rhi::GraphicsPipelineInfo pplInfo;
    pplInfo.colorBlendInfo.attachmentBlends.emplace_back();
    pplInfo.depthStencilInfo.depthTestEnable = false;
    pplInfo.depthStencilInfo.depthWriteEnable = false;
    pplInfo.primitiveType = rhi::PrimitiveType::TRIANGLE_LIST;
    pplInfo.vertexLayout = scene::Cube::vertexLayout;

    rhi::RenderPassInfo rpInfo{};
    auto& attachment = rpInfo.attachments.emplace_back();
    attachment.format = rhi::Format::RGBA16_SFLOAT;
    attachment.initialLayout = rhi::ImageLayout::UNDEFINED;
    attachment.finalLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
    auto& subpass = rpInfo.subpasses.emplace_back();
    subpass.colors.emplace_back(0, rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
    auto renderPass = rhi::RenderPassPtr(device->createRenderPass(rpInfo));

    std::string vsrc{"#version 450 core\n"};
    vsrc.append(vert);
    rhi::ShaderSourceInfo vertSourceInfo{
        vertPath.data(),
        {rhi::ShaderStage::VERTEX, vsrc},
    };
    auto vertShader = rhi::ShaderPtr(device->createShader(vertSourceInfo));
    std::string fsrc{"#version 450 core\n"};
    fsrc.append(frag);
    rhi::ShaderSourceInfo fragSourceInfo{
        fragPath.data(),
        {rhi::ShaderStage::FRAGMENT, fsrc},
    };
    auto fragShader = rhi::ShaderPtr(device->createShader(fragSourceInfo));
    pplInfo.shaders.emplace_back(vertShader.get());
    pplInfo.shaders.emplace_back(fragShader.get());
    pplInfo.pipelineLayout = pplLayout.get();
    pplInfo.renderPass = renderPass.get();
    auto pso = rhi::GraphicsPipelinePtr(device->createGraphicsPipeline(pplInfo));

    return std::make_tuple(layout0, layout, pplLayout, descPool, descSet0, descSet, renderPass, vertShader, fragShader, pso);
}

constexpr uint32_t diffuseResolution = 1024;
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

    auto view = rhi::ImageViewPtr(device->createImageView(imgViewInfo));
    auto [layout0,
          layout,
          pplLayout,
          descPool,
          descSet0,
          descSet,
          renderPass,
          vertShader,
          fragShader,
          pso] = generateResources(view, vertSource, "equirectangularSource", fragSource, "equirectangularFrag", 0, device);

    auto [views, frameBuffers] = renderCube(cubeImage, diffuseResolution, 0, pso, descSet0, descSet, nullptr, 0, renderPass, cmdBuffer, device);

    cmdBuffer->onComplete([=]() mutable {
        layout0.reset();
        layout.reset();
        pplLayout.reset();
        descSet0.reset();
        descSet.reset();
        descPool.reset();
        renderPass.reset();
        vertShader.reset();
        fragShader.reset();
        pso.reset();
        view.reset();
        image.reset();

        views.clear();
        frameBuffers.clear();
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

    scene::Cube::vertexBuffer.buffer = meshData.vertexBuffer.buffer;

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
        .format = rhi::Format::RGBA16_SFLOAT,
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

    auto [layout0,
          layout,
          pplLayout,
          descPool,
          descSet0,
          descSet,
          renderPass,
          vertShader,
          fragShader,
          pso] = generateResources(cubeView, vertSource, "bakeDiffuseVert", diffuseIrradianceSource, "bakeDiffuseFrag", 0, device);

    auto [views, frameBuffers] = renderCube(diffuse, 32, 0, pso, descSet0, descSet, nullptr, 0, renderPass, cmdBuffer, device);

    cmdBuffer->onComplete([=]() mutable {
        layout0.reset();
        layout.reset();
        pplLayout.reset();
        descSet0.reset();
        descSet.reset();
        descPool.reset();
        renderPass.reset();
        vertShader.reset();
        fragShader.reset();
        pso.reset();
        views.clear();
        frameBuffers.clear();
    });
}

void prefilterSpecular(rhi::ImagePtr& specular,
                       rhi::ImageViewPtr& specularView,
                       rhi::ImageViewPtr cubeView,
                       rhi::CommandBufferPtr cmdBuffer,
                       rhi::DevicePtr device) {
    rhi::ImageInfo specularInfo{
        .usage = rhi::ImageUsage::SAMPLED | rhi::ImageUsage::COLOR_ATTACHMENT,
        .imageFlag = rhi::ImageFlag::CUBE_COMPATIBLE,
        .format = rhi::Format::RGBA16_SFLOAT,
        .sliceCount = 6,
        .mipCount = 8,
        .extent = {128, 128, 1},
    };
    specular = rhi::ImagePtr(device->createImage(specularInfo));

    rhi::ImageViewInfo dvInfo{
        .type = rhi::ImageViewType::IMAGE_VIEW_CUBE,
        .image = specular.get(),
        .range = {
            .sliceCount = 6,
            .mipCount = 8,
        },
        .format = specularInfo.format,
    };
    specularView = rhi::ImageViewPtr(device->createImageView(dvInfo));

    auto [layout0,
          layout,
          pplLayout,
          descPool,
          descSet0,
          descSet,
          renderPass,
          vertShader,
          fragShader,
          pso] = generateResources(cubeView, vertSource, "prefilterVert", specularPrefilterSource, "prefilterFrag", 4, device);

    std::vector<std::vector<rhi::ImageViewPtr>> views;
    std::vector<std::vector<rhi::FrameBufferPtr>> frameBuffers;
    for (size_t i = 0; i < 8; ++i) {
        float roughness = 1.0f / 7 * i;
        auto [vs, fbs] = renderCube(specular, 128, i, pso, descSet0, descSet, &roughness, 4, renderPass, cmdBuffer, device);
        views.emplace_back(vs);
        frameBuffers.emplace_back(fbs);
    }

    cmdBuffer->onComplete([=]() mutable {
        layout0.reset();
        layout.reset();
        pplLayout.reset();
        descSet0.reset();
        descSet.reset();
        descPool.reset();
        renderPass.reset();
        vertShader.reset();
        fragShader.reset();
        pso.reset();
        views.clear();
        frameBuffers.clear();
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
        .initialLayout = rhi::ImageLayout::UNDEFINED,
        .format = rhi::Format::RGBA16_SFLOAT,
        .sliceCount = 6,
        .mipCount = 1,
        .extent = {
            diffuseResolution,
            diffuseResolution,
            1,
        }};
    _cubeImage = rhi::ImagePtr(device->createImage(cubeImageInfo));

    sampleEquirectangular(data, width, height, cmdBuffer, device, shaderGraph, _cubeImage, meshRenderer);
    init(meshRenderer, cmdBuffer, device);
    bakeIrradiance(_diffuse, _diffuseView, _cubeView, cmdBuffer, device);
    prefilterSpecular(_specular, _specularView, _cubeView, cmdBuffer, device);
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
        .texture = _cubeImage,
        .textureView = _cubeView,
    };
    scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/skybox");
    auto skyboxMat = matTemplate->instantiate("asset/layout/skybox", scene::MaterialType::CUSTOM);
    skyboxMat->set("environmentMap", cube);
    scene::Sampler sampler{
        rhi::SamplerInfo{
            .magFilter = rhi::Filter::LINEAR,
            .minFilter = rhi::Filter::LINEAR,
        },
    };
    skyboxMat->set("linearSampler", sampler);

    scene::TechniquePtr skyboxTech = std::make_shared<scene::Technique>(skyboxMat, "default");
    skyboxTech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
    auto& ds = skyboxTech->depthStencilInfo();
    ds.depthTestEnable = true;
    ds.depthWriteEnable = false;
    ds.depthCompareOp = rhi::CompareOp::LESS_OR_EQUAL;
    auto& bs = skyboxTech->blendInfo();
    bs.attachmentBlends.emplace_back();
    // auto& rs = skyboxTech->rasterizationInfo();
    // rs.frontFace = rhi::FrontFace::COUNTER_CLOCKWISE;
    // rs.cullMode = rhi::FaceMode::BACK;

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

rhi::ImagePtr Skybox::prefilteredSpecularImage() const {
    return _specular;
}

rhi::ImageViewPtr Skybox::prefilteredSpecularView() const {
    return _specularView;
}

} // namespace raum::asset