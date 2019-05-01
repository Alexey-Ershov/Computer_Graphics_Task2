#version 330

layout(location = 0) in vec3 vertex;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

void main(void)
{
    gl_Position  = vec4(vertex, 1.0f);
    TexCoord = texCoord;
}
