#version 330

layout(location = 0) in vec3 vertex;

out vec3 TexCoords;

void main(void)
{
    TexCoords = vertex;
    gl_Position  = vec4(vertex, 1.0f);
}
