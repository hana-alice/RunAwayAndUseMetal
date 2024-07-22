
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec2 f_uv;

layout(set = 0, binding = 0) uniform Mat {
    mat4 modelMat;
    mat4 viewMat;
    mat4 projectMat;
};

void main () {
    f_uv = v_uv;
    gl_Position =  projectMat * viewMat * modelMat * vec4(aPos.xy, aPos.z, 1.0f);
}