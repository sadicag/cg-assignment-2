#version 410
uniform mat4 lightMVP;
layout(location=0) in vec3 position;
uniform mat4 modelMatrix;
void main() {
    gl_Position = lightMVP * vec4(position, 1.0);
}

