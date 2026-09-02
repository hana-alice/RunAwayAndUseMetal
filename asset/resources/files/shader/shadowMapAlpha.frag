layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec2 f_uv;

layout(set = 1, binding = 0) uniform texture2D albedoMap;
layout(set = 1, binding = 1) uniform sampler linearSampler;
layout(set = 1, binding = 2) uniform AlphaParams {
    vec4 alphaParams; // cutoff, baseColorFactor alpha, unused, unused
};

void main() {
    float alpha = texture(sampler2D(albedoMap, linearSampler), f_uv).a * alphaParams.y;
    if (alpha < alphaParams.x) {
        discard;
    }
    FragColor = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}
