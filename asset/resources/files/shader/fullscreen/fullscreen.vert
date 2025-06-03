// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
layout(location = 0) out vec2 f_uv;

void main() {
    f_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vec2 uv = vec2(f_uv.x, 1.0f - f_uv.y);
    gl_Position = vec4(uv * 2.0f + -1.0f, 0.0f, 1.0f);
}