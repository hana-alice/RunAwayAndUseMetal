layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
#ifdef VERTEX_TANGENT
layout(location = 3) in vec4 v_tangent;
#endif

layout(location = 0) out vec3 f_worldPos;
layout(location = 1) out vec2 f_uv;
layout(location = 2) out vec4 f_tan;
layout(location = 3) out vec3 f_normal;

layout(set = 0, binding = 0) uniform Mat {
    mat4 viewMat;
    mat4 projectMat;
};

layout(set = 2, binding = 0) uniform LocalMat {
    mat4 modelMat;
};

void main () {
    f_uv = v_uv;
#ifdef VERTEX_TANGENT
    f_tan = vec4((modelMat * v_tangent).xyz, v_tangent.w);
#endif
    f_normal = (transpose(inverse(modelMat)) * vec4(v_normal, 1.0f)).xyz;
    f_normal = normalize(f_normal);
    vec4 worldPos = modelMat * vec4(aPos.xyz, 1.0f);
    f_worldPos = (worldPos / worldPos.w).xyz;
    gl_Position =  projectMat * viewMat * modelMat * vec4(aPos.xyz, 1.0f);
}