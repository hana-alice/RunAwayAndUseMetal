layout(location = 0) out vec4 FragColor;

layout(location = 0) in vec3 f_worldPos;

layout(set = 0, binding = 1) uniform ShadowView {
    mat4 shadowView;
    mat4 shadowProjection;
    vec2 shadowMapSizeInv;
};

layout(set = 0, binding = 2) uniform texture2D shadowMap;
layout(set = 0, binding = 3) uniform sampler shadowSampler;
layout(set = 0, binding = 4) uniform texture2D ao; // may be contact shadow

layout(set = 0, binding = 5) uniform ViewportSize {
    vec2 viewportSize;
};

void main () {
    vec4 shadowCoord = shadowProjection * shadowView * vec4(f_worldPos, 1.0);
    shadowCoord.xyz /= shadowCoord.w;
    shadowCoord.xy = shadowCoord.xy * 0.5 + 0.5;

    // vulkan flip viewport
    shadowCoord.y = 1.0 - shadowCoord.y;

    float xStep = shadowMapSizeInv.x;
    float yStep = shadowMapSizeInv.y;
    float coef = 0.0;

    float bias = 0.003;
    for (float x = -1.0; x <= 1.0; x += 1.0) {
        for (float y = -1.0; y <= 1.0; y += 1.0) {
            vec2 offset = vec2(x, y);
            float d = texture(sampler2D(shadowMap, shadowSampler), shadowCoord.xy + offset * vec2(xStep, yStep)).r;
            float w = shadowCoord.z + bias < d ? 0.0 : 1.0;
            coef += w;
        }
    }
    coef /= 9.0;

    float solidColor = 0.2;
    solidColor = mix(solidColor, 0.8, coef);

    vec2 uv = vec2(gl_FragCoord.x / viewportSize.x, gl_FragCoord.y / viewportSize.y);
    float aoValue = texture(sampler2D(ao, shadowSampler), uv).r;
    float tmp = 0.2;
    tmp = mix(tmp, 0.8, aoValue);
    solidColor = min(solidColor, tmp);

    FragColor = vec4(solidColor, solidColor, solidColor, 1.0);
}