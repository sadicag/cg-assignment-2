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
uniform vec3 lightPosition; // Position
uniform vec3 lightDirection_optional; // Direction
uniform vec3 lightColor; // Colour
uniform int isSpot; // Is spotlight? :0

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 normal = normalize(fragNormal);

    // --- Set the light direction vector
    vec3 lightDirection = lightDirection_optional;
    if (isSpot == 1)
    { // don't use the direction vector
	lightDirection = normalize(lightPosition - fragPosition);
    }

    // --- Set the final output
    vec3 finalOut;
    if (hasTexCoords)
    { 
	finalOut = texture(colorMap, fragTexCoord).rgb;
    }
    else if (useMaterial)
    { 
	// Basic Lambertian Diffusion
	vec3 diffusion = kd * max(dot(fragNormal, lightDirection), 0.1);
	finalOut = lightColor * diffusion;
    }
    else               
    { 
	// Output color value, change from (1, 0, 0) to something else
	finalOut = normal;
    }
    
    fragColor = vec4(finalOut, 1.0);
}
