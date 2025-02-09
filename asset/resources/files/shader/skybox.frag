layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 localPos;

layout (set = 1, binding = 0) uniform textureCube environmentMap;
layout (set = 1, binding = 1) uniform sampler linearSampler;

void main()
{
    vec3 envColor = texture(samplerCube(environmentMap, linearSampler), localPos).rgb;

    vec3 mapped = envColor / (envColor + vec3(1.0));
    mapped = pow(mapped, vec3(1.0/2.2));

    FragColor = vec4(mapped, 1.0);
}