#version 410

// Material properties 
layout(std140) uniform Material
{
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};
//Empty fragment shader to keep mac drivers happy
void main()
{
}
