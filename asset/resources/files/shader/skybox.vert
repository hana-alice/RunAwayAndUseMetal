
layout (location = 0) in vec3 aPos;

layout (location = 0) out vec3 WorldPos;

layout(set = 0, binding = 0) uniform Mat {
    mat4 viewMat;
    mat4 projectMat;
};

void main()
{
    WorldPos = vec3(aPos.x, -aPos.y, aPos.z);
    mat4 rot = mat4(mat3(viewMat));
    vec4 pos = projectMat * rot * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}