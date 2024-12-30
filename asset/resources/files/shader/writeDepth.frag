layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 f_fragPos;

void main()
{
    FragColor = vec4(f_fragPos.z, 0.0, 0.0, 1.0);
}