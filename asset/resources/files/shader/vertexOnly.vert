layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

layout(location = 0) out vec3 f_worldPos;
layout(location = 1) out vec2 f_uv;

layout(set = 0, binding = 0) uniform Mat {
    mat4 viewMat;
    mat4 projectMat;
};

layout(set = 2, binding = 0) uniform LocalMat {
    mat4 modelMat;
};

void main () {
    vec4 worldPos = modelMat * vec4(aPos, 1.0);
    f_worldPos = worldPos.xyz / worldPos.w;
    f_uv = aTexCoords;
    gl_Position =  projectMat * viewMat * worldPos;
}