#version 450 core

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 1) uniform texture2D mainTexture;
layout(set = 1, binding = 2) uniform sampler mainSampler;


void main () {
    FragColor = texture(sampler2D(mainTexture, mainSampler), f_uv);
}