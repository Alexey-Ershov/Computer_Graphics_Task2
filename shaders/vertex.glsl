#version 330

layout(location = 0) in vec3 vertex;

void main(void)
{
    gl_Position  = vec4(vertex.x, vertex.y, vertex.z, 1.0);
}
