layout(location = 0) in vec3 aPos;
#ifdef VERTEX_NORMAL
layout(location = 2) in vec2 aTexCoords;
#else
layout(location = 1) in vec2 aTexCoords;
#endif

layout(location = 0) out vec2 f_uv;

layout(set = 0, binding = 0) uniform Mat {
    mat4 viewMat;
    mat4 projectMat;
};

layout(set = 2, binding = 0) uniform LocalMat {
    mat4 modelMat;
};

void main() {
    f_uv = aTexCoords;
    gl_Position = projectMat * viewMat * modelMat * vec4(aPos, 1.0);
}
