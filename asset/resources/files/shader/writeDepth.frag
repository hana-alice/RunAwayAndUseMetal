layout (location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(gl_FragCoord.z, 0.0, 0.0, 1.0);
}