#version 410

uniform mat4 lightSpaceMatrix;

layout(location = 0) in vec3 position;

void main()
{
    gl_Position = lightSpaceMatrix * vec4(position, 1);
}
