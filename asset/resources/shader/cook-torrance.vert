#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

layout(location = 0) out vec2 f_uv;
layout(location = 1) out vec3 f_worldPos;

layout(set = 0, binding = 0) uniform Mat {
    mat4 modelMat;
    mat4 viewMat;
    mat4 projectMat;
};

void main () {
    f_uv = v_uv;
    vec4 worldPos = modelMat * vec4(aPos.xyz, 1.0f);
    f_worldPos = (worldPos / worldPos.w).xyz;
    gl_Position =  projectMat * viewMat * modelMat * vec4(aPos.xyz, 1.0f);
}