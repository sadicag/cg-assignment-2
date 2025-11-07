#version 410

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
// Normals should be transformed differently than positions:
// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
uniform mat3 normalModelMatrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec3 fragTangent;
out vec3 fragBitangent;

void main()
{
    gl_Position = mvpMatrix * vec4(position, 1);
    
    fragPosition    = (modelMatrix * vec4(position, 1)).xyz;
    fragNormal      = normalModelMatrix * normal;
    fragTexCoord    = texCoord;
    
    // Calculate tangent and bitangent for normal mapping
    // Using simplified tangent calculation based on texture coordinates
    vec3 worldNormal = normalize(normalModelMatrix * normal);
    
    // Create tangent vector perpendicular to normal
    vec3 c1 = cross(worldNormal, vec3(0.0, 0.0, 1.0));
    vec3 c2 = cross(worldNormal, vec3(0.0, 1.0, 0.0));
    
    vec3 tangent = length(c1) > length(c2) ? c1 : c2;
    tangent = normalize(tangent);
    
    vec3 bitangent = normalize(cross(worldNormal, tangent));
    
    fragTangent = tangent;
    fragBitangent = bitangent;
}
