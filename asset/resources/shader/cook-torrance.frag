#define PI 3.14159265358979
// Trowbridge-Reitz GGX
// NDG_GGXTR(n, h, a) = a^2 / PI * ((n*h)^2*(a^2 - 1) + 1)^2
float D_GGX_TR(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

// Schlick-GGX
// G_SchlichGGX(n, v, k) = n*v / ((n*v)(1-k) + k)
// k_direct = (a+1)^2 / 8
// k_IBL = a^2 / 2
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float k = roughness;
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}

// Fresnel-Schlick
// FSchlick(h,v,F0)=F0+(1−F0)(1−(h⋅v))5
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// https://hacksoflife.blogspot.com/2009/11/per-pixel-tangent-space-normal-mapping.html
mat3x3 getTBN(vec3 pos, vec2 uv, vec3 normal) {
    vec3 q0 = dFdx(pos);
    vec3 q1 = dFdy(pos);
    vec2 st0 = dFdx(uv);
    vec2 st1 = dFdy(uv);

    //    vec3 S = normalize(q0 * st1.t - q1 * st0.t);
    vec3 T = normalize(-q0 * st1.s + q1 * st0.s);
    vec3 N = normal;
    vec3 B = cross(T, N);
    return mat3x3(T, B, N);
}

layout (location = 0) out vec4 FragColor;
layout (location = 0) in vec3 f_worldPos;
layout (location = 1) in vec2 f_uv;
layout (location = 2) in vec4 f_tan;
layout (location = 3) in vec3 f_normal;

layout (set = 0, binding = 1) uniform CamPos {
    vec3 camPos;
};
layout (set = 0, binding = 2) uniform Light {
    vec4 lightPos;
    vec4 lightColor;
};

layout (set = 1, binding = 0) uniform texture2D albedoMap;
layout (set = 1, binding = 1) uniform texture2D normalMap;
layout (set = 1, binding = 2) uniform texture2D metallicRoughnessMap;
layout (set = 1, binding = 3) uniform texture2D emissiveMap;
layout (set = 1, binding = 4) uniform texture2D aoMap;

layout (set = 1, binding = 5) uniform sampler pointSampler;

layout (set = 1, binding = 6) uniform PBRParams {
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    vec4 mrno; // metallic, roughness, normalscale, occlusionscale
};

void main() {
    vec3 raw = texture(sampler2D(albedoMap, pointSampler), f_uv).rgb;
    vec3 albedo = pow(raw, vec3(2.2));
    albedo = albedo * baseColorFactor.xyz;
    float metallic = texture(sampler2D(metallicRoughnessMap, pointSampler), f_uv).b * mrno.x;
    float roughness = texture(sampler2D(metallicRoughnessMap, pointSampler), f_uv).g * mrno.y;
    #ifdef VERTEX_TANGENT
    vec3 sn = texture(sampler2D(normalMap, pointSampler), f_uv).rgb;
    sn = (sn * 2.0 - vec3(1.0f)) * vec3(mrno.z, mrno.z, 1.0f);
    vec3 bi = cross(f_normal, f_tan.xyz) * f_tan.w;
    mat3x3 tbn = mat3x3(f_tan.xyz, bi, f_normal);
    vec3 N = tbn * sn;
    #else
    #ifdef NORMAL_MAP
        vec3 sn = texture(sampler2D(normalMap, pointSampler), f_uv).rgb;
        mat3x3 tbn = getTBN(f_worldPos, f_uv, f_normal);
        vec3 N = tbn * sn;
    #else
        vec3 N = f_normal;
    #endif
#endif
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 V = normalize(camPos - f_worldPos);

    vec3 Lo = vec3(0.0);

    // accumulate begin
    vec3 L = normalize(lightPos.xyz - f_worldPos);
    vec3 H = normalize(V + L);
    float distance = length(lightPos.xyz - f_worldPos);
    float attenuation = 1.0f;// 1.0 / (distance * distance);
    vec3 radiance = lightColor.rgb * attenuation;

    float NDF = D_GGX_TR(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 Ks = F;
    vec3 KD = vec3(1.0) - Ks;
    KD *= 1.0 - metallic;

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = nominator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (KD * albedo / PI + specular) * radiance * NdotL;

    #ifdef OCCLUSION_MAP
    float ao = texture(sampler2D(aoMap, pointSampler), f_uv).r;
    ao = 1.0f + mrno.w * (ao - 1.0f);
    #else
    float ao = 1.0f;
    #endif
    vec3 ambient = KD * albedo * lightColor.rgb * ao;
    #ifdef EMISSIVE_MAP
    vec3 emissive = texture(sampler2D(emissiveMap, pointSampler), f_uv).rgb * emissiveFactor.xyz;
    #else
    vec3 emissive = vec3(0.0f);
    #endif
    vec3 color = ambient + Lo + emissive;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
    // accumulate end

}