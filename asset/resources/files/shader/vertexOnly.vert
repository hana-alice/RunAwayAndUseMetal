layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 f_fragPos;

layout(set = 0, binding = 0) uniform Mat {
    mat4 viewMat;
    mat4 projectMat;
};

layout(set = 2, binding = 0) uniform LocalMat {
    mat4 modelMat;
};

void main () {
    gl_Position =  projectMat * viewMat * modelMat * vec4(aPos.xyz, 1.0f);
    f_fragPos = gl_Position.xyz;
}