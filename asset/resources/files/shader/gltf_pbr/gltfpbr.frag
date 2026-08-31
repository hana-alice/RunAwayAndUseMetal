//#version 450
#extension GL_GOOGLE_include_directive : require
#define PI 3.14159265358979

//#include "srgbtolinear.glsl"
/* Copyright (c) 2018-2023, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

vec4 SRGBtoLINEAR(vec4 srgbIn)
{
    #define MANUAL_SRGB 1
	#ifdef MANUAL_SRGB
	#ifdef SRGB_FAST_APPROXIMATION
	vec3 linOut = pow(srgbIn.xyz,vec3(2.2));
    #else //SRGB_FAST_APPROXIMATION
	vec3 bLess = step(vec3(0.04045),srgbIn.xyz);
    vec3 linOut = mix( srgbIn.xyz/vec3(12.92), pow((srgbIn.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
    #endif //SRGB_FAST_APPROXIMATION
	return vec4(linOut,srgbIn.w);;
    #else //MANUAL_SRGB
	return srgbIn;
    #endif //MANUAL_SRGB
}


// gltf pbr
// material = mix(dielectric_brdf, metal_brdf, metallic);
// expands to: (1.0 - metallic) * dielectric_brdf + metallic * metal_brdf

// metal_brdf = conductor_fresnel(F0 = baseColor, bsdf = specular_brdf(α = roughness ^ 2))
// dielectric_brdf = fresnel_mix(ior = 1.5, base = diffuse_brdf(color = baseColor), layer = specular_brdf(α = roughness ^ 2))

// specular:
// MicrofacetBRDF = G * D / (4 * | N * L | * | N * V |)
// D = α^2 * χ_(N*H) / π * ((N*H)^2 * (α^2 - 1) + 1)^2
// G1 = 2 * |N * L| * χ_(H*L)/(|N * L| + sqrt(α^2 + (1 - α^2) * |N * L|^2))
// G2 = 2 * |N * V| * χ_(H*V)/(|N * V| + sqrt(α^2 + (1 - α^2) * |N * V|^2))
// χ_(N*H) = 1 if N*H > 0, 0 otherwise
// G = G1 * G2
// let V = G / (4 * |N * L| * |N * V|)
// thus, MicrofacetBRDF = V * D
// specular_brdf = MicrofacetBRDF

// diffuse:
// LamberianBRDF = 1 / π
// diffuse_brdf = LamberianBRDF * color

// fresnel:
// F = F0 + (1 - F0) * (1 - |H * V|)^5
// conductor_fresnel = bsdf * F

vec4 fresnel_mix(float ior, vec4 base, vec4 layer, float VdotH) {
    float F0 = pow(((1.0 - ior) / (1.0 + ior)), 2);
    float F = F0 + pow((1.0 - F0) * (1.0 - abs(VdotH)), 5);
    return mix(base, layer, F);
}

// metal_brdf = specular_brdf(α = roughness ^ 2) * (baseColor + (1 - baseColor) * (1 - |H * V|)^5)
// dielectric_brdf = mix(base = diffuse_brdf(color = baseColor), layer = specular_brdf(α = roughness ^ 2), 0.04 + (1 - 0.04)*(1 - |H * V|)^5)
// material = mix(dielectric_brdf, metal_brdf, metallic)

mat3x3 getTBN(vec3 pos, vec2 uv, vec3 normal) {
    // compute derivations of the world position
    vec3 p_dx = dFdx(pos);
    vec3 p_dy = dFdy(pos);
    // compute derivations of the texture coordinate
    vec2 tc_dx = dFdx(uv);
    vec2 tc_dy = dFdy(uv);
    // compute initial tangent and bi-tangent
    vec3 t = normalize( tc_dy.y * p_dx - tc_dx.y * p_dy );
    vec3 b = normalize( tc_dy.x * p_dx - tc_dx.x * p_dy ); // sign inversion
    // get new tangent from a given mesh normal
    vec3 n = normalize(normal);
    vec3 x = cross(n, t);
    t = cross(x, n);
    t = normalize(t);
    // get updated bi-tangent
    x = cross(b, n);
    b = cross(n, x);
    b = normalize(b);
    mat3 tbn = mat3(t, b, n);
    return tbn;
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
    vec4 lightDirection;
    vec4 lightColor;
};

layout(push_constant) uniform AlphaCutoff {
    float alphaCutoff;
};

layout (set = 1, binding = 0) uniform texture2D albedoMap;
layout (set = 1, binding = 1) uniform texture2D normalMap;
layout (set = 1, binding = 2) uniform texture2D metallicRoughnessMap;
layout (set = 1, binding = 3) uniform texture2D emissiveMap;
layout (set = 1, binding = 4) uniform texture2D aoMap;
layout (set = 1, binding = 5) uniform textureCube diffuseEnvMap;
layout (set = 1, binding = 6) uniform textureCube specularMap;
layout (set = 1, binding = 7) uniform texture2D brdfLUT;

layout (set = 1, binding = 8) uniform sampler linearSampler;
layout (set = 1, binding = 9) uniform sampler pointSampler;

layout (set = 1, binding = 10) uniform PBRParams {
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    vec4 mrno; // metallic, roughness, normalscale, occlusionscale
};

float chi(float NdotH) {
    return NdotH > 0.0 ? 1.0 : 0.0;
}

float D_component(float alpha_sq, float NdotH) {
    float numerator = alpha_sq * chi(NdotH);
    float v = NdotH * NdotH * (alpha_sq - 1.0) + 1.0;
    float denominator = PI * v * v; // avoid division by zero
    return numerator / denominator;
}

float G_component(float alpha_sq, float NdotL, float NdotV, float NdotH, float HdotL, float HdotV) {
    float absNdotL = abs(NdotL);
    float absNdotV = abs(NdotV);
    float G1 = 2.0 * absNdotL * chi(HdotL) / (absNdotL + sqrt(alpha_sq + (1.0 - alpha_sq) * absNdotL * absNdotL));
    float G2 = 2.0 * absNdotV * chi(HdotV) / (absNdotV + sqrt(alpha_sq + (1.0 - alpha_sq) * absNdotV * absNdotV));
    return G1 * G2;
}

vec3 getNormal(vec3 sampledNormal, float scaleFactor) {
    vec3 geometricNormal = normalize(f_normal);
#ifdef DOUBLE_SIDED
    if (!gl_FrontFacing) {
        geometricNormal = -geometricNormal;
    }
#endif

#ifdef NORMAL_MAP
    vec3 sn = sampledNormal;
    sn = (sn * 2.0 - vec3(1.0f)) * vec3(scaleFactor, scaleFactor, 1.0f);
#ifdef VERTEX_TANGENT
    vec3 tangent = normalize(f_tan.xyz - geometricNormal * dot(geometricNormal, f_tan.xyz));
    vec3 bi = normalize(cross(geometricNormal, tangent)) * f_tan.w;
    mat3x3 tbn = mat3x3(tangent, bi, geometricNormal);
    vec3 N = tbn * sn;
#else
    mat3 tbn = getTBN(f_worldPos, f_uv, geometricNormal);
    vec3 N = tbn * sn;
#endif
#else
    vec3 N = geometricNormal;
#endif
    return normalize(N);
}

vec3 getIBLContributions(vec3 R, vec3 N, vec3 diffuseIn, vec3 specularIn, float roughness, float NdotV) {
    vec2 brdf = texture(sampler2D(brdfLUT, linearSampler), vec2(NdotV, roughness)).rg;
    vec3 diffuseLight = texture(samplerCube(diffuseEnvMap, linearSampler), N).rgb;
    float maxReflectionLod = float(textureQueryLevels(samplerCube(specularMap, linearSampler)) - 1);
    vec3 specularLight = textureLod(samplerCube(specularMap, linearSampler), R, roughness * maxReflectionLod).rgb;

    vec3 diffuse = diffuseLight * diffuseIn;
    vec3 specular = specularLight * (specularIn * brdf.x + brdf.y);

    return diffuse + specular;
}

float getAOContributions(float factor) {
    #ifdef OCCLUSION_MAP
    float ao = texture(sampler2D(aoMap, linearSampler), f_uv).r;
    ao = 1.0f + factor * (ao - 1.0f);
    #else
    float ao = 1.0f;
    #endif
    return ao;
}

void main() {
#ifdef BASE_COLOR_MAP
    vec4 albedo = SRGBtoLINEAR(texture(sampler2D(albedoMap, linearSampler), f_uv));
#else
    vec4 albedo = vec4(1.0);
#endif

    vec4 baseColor = albedo * baseColorFactor;

#ifdef ALPHA_MASK
    if (baseColor.a < alphaCutoff) {
       discard;
    }
#endif

#ifdef EMISSIVE_MAP
    vec4 em = SRGBtoLINEAR(texture(sampler2D(emissiveMap, linearSampler), f_uv));
    vec3 emissive = vec3(em * emissiveFactor);
#else
    vec3 emissive = emissiveFactor.xyz;
#endif

#ifdef MATALLIC_ROUGHNESS_MAP
    vec4 mr = texture(sampler2D(metallicRoughnessMap, linearSampler), f_uv);
    float metallic = clamp(mr.b * mrno.x, 0.0, 1.0);
    float roughness = clamp(mr.g * mrno.y, 0.045, 1.0);
#else
    float metallic = clamp(mrno.x, 0.0, 1.0);
    float roughness = clamp(mrno.y, 0.045, 1.0);
#endif

#ifdef NORMAL_MAP
    vec3 sampledNormal = texture(sampler2D(normalMap, linearSampler), f_uv).rgb;
#else
    vec3 sampledNormal = vec3(0.5, 0.5, 1.0);
#endif
    vec3 N = getNormal(sampledNormal, mrno.z);

    vec3 V = normalize(camPos - f_worldPos);
    vec3 L = normalize(-lightDirection.xyz);
    vec3 halfVector = V + L;
    float halfLengthSquared = dot(halfVector, halfVector);
    vec3 H = halfLengthSquared > 0.000001
                 ? halfVector * inversesqrt(halfLengthSquared)
                 : N;

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float HdotV = clamp(dot(H, V), 0.0, 1.0);
    float HdotL = clamp(dot(H, L), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);

    // gltf pbr
    vec3 black = vec3(0.0f);

    vec3 colorDiff = mix(baseColor.rgb, black, metallic);
    vec3 F0 = mix(vec3(0.04f), baseColor.rgb, metallic);

    vec3 F = F0 + (vec3(1.0f) - F0) * pow(1.0f - HdotV, 5.0f);

    vec3 f_diffuse = (1 - F) * colorDiff / PI;

    float alpha = roughness * roughness;
    float alpha_sq = alpha * alpha;
    vec3 directLighting = vec3(0.0);
    if (NdotL > 0.0 && NdotV > 0.0) {
        float D = D_component(alpha_sq, NdotH);
        float G = G_component(alpha_sq, NdotL, NdotV, NdotH, HdotL, HdotV);
        vec3 f_specular = F * D * G / max(4.0f * NdotL * NdotV, 0.0001f);
        directLighting = lightColor.rgb * NdotL * (f_diffuse + f_specular);
    }
    vec3 outColor = directLighting;

    // ibl
    vec3 envDiffuseFactor = baseColor.rgb * (vec3(1.0) - F0);
    envDiffuseFactor *= 1.0 - metallic;
    vec3 envSpecFactor = F0;

    vec3 ibl = getIBLContributions(reflect(-V, N), N, envDiffuseFactor, envSpecFactor, roughness, NdotV);

    float ao = getAOContributions(mrno.w);
    outColor.rgb += ibl * ao;

    outColor.rgb += emissive.xyz;

    outColor = outColor / (outColor + vec3(1.0));
    outColor = pow(outColor, vec3(1.0 / 2.2));
#ifdef ALPHA_BLEND
    FragColor = vec4(outColor, baseColor.a);
#else
    FragColor = vec4(outColor, 1.0f);
#endif
}
