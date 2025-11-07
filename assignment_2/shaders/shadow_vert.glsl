#version 410

uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;

layout(location = 0) in vec3 position;

void main()
{
    gl_Position = lightSpaceMatrix * modelMatrix * vec4(position, 1.0);
}
