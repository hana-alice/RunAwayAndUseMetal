
#extension GL_ARB_sparse_texture2 : enable
layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 FragColor;

layout(set = 1, binding = 0) uniform texture2D mainTexture;
layout(set = 1, binding = 1) uniform sampler mainSampler;
layout(set = 1, binding = 2) uniform PageExtent {
    int blockWidth;
    int blockHeight;
    int width;
    int height;
};

layout(set = 1, binding = 3) buffer accessCounter {
    uint mips[];
};

vec3 color_blend_table[5] = 
{
	{1.00, 1.00, 1.00},
	{0.80, 0.60, 0.40},
	{0.60, 0.80, 0.60},
	{0.40, 0.60, 0.80},
	{0.20, 0.20, 0.20},
};

void main () {
    int minLod = 0;
    int maxLod = 9;
    int lod = minLod;
    vec4 color = vec4(0.0);
    int residencyCode = sparseTextureLodARB(sampler2D(mainTexture, mainSampler), f_uv, minLod, color);

    for(++lod; lod <= maxLod && !sparseTexelsResidentARB(residencyCode); ++lod) {
        residencyCode = sparseTextureLodARB(sampler2D(mainTexture, mainSampler), f_uv, lod, color);
    }

    uint y = int(f_uv.y * blockHeight);
    uint x = int(f_uv.x * blockWidth);
    uint pageIndex = y * blockWidth + x;
    
    vec2 extent = vec2(float(width), float(height));
    vec2 dx = dFdx(f_uv * extent);
    vec2 dy = dFdy(f_uv * extent);
    float dMaxSqr = max(dot(dx, dx), dot(dy, dy));
    float mip = 0.5 * log2(dMaxSqr);
    uint floorMip = int(mip);

    atomicMin(mips[pageIndex], floorMip);

    //lod -= 6;
    //color.xyz = (color.xyz * color_blend_table[lod]);
	
    FragColor = color;
}