#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
	vec3 ks;
	float shininess;
	float transparency;
};

uniform sampler2D colorMap;
uniform bool hasTexCoords;
uniform bool useMaterial;

// Light properties
uniform vec3 lightPos; // Position
uniform vec3 lightDir; // Direction
uniform vec3 lightCol; // Colour
uniform int isSpot; // Is spotlight? :0

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 normal = normalize(fragNormal);


    if (hasTexCoords)
    { 
	fragColor = vec4(texture(colorMap, fragTexCoord).rgb, 1);
    }
    else if (useMaterial)
    { 
	// Output the normal as color
	vec3 diffusion = kd * max(dot(fragNormal, lightDir), 0.1);
	fragColor = vec4(lightCol * diffusion, 1.0);
    }
    else               
    { 
	// Output color value, change from (1, 0, 0) to something else
	fragColor = vec4(normal, 1); 
    }
}
