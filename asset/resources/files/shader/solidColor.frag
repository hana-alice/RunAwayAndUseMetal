layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec3 f_worldPos;

layout(set = 0, binding = 1) uniform ShadowView {
    mat4 shadowView;
    mat4 shadowProjection;
};

layout(set = 0, binding = 2) uniform texture2D shadowMap;
layout(set = 0, binding = 3) uniform sampler shadowSampler;

void main () {
    vec4 shadowCoord = shadowProjection * shadowView * vec4(f_worldPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xyz = shadowCoord.xyz * 0.5 + 0.5;
    float shadow = texture(sampler2D(shadowMap, shadowSampler), shadowCoord.xy).r;

    float solidColor = 0.8;
    if (shadowCoord.z > shadow) {
        solidColor = 0.2;
    }

    FragColor = vec4(solidColor, solidColor, solidColor, 1.0);
}