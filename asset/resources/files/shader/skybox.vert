
layout (location = 0) in vec3 aPos;

layout (location = 0) out vec3 WorldPos;

void main()
{
    WorldPos = vec3(aPos.x, -aPos.y, aPos.z);
    gl_Position =  vec4(aPos, 1.0);
}