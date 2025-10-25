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

//these ones i just added:
uniform vec3 cameraPosition;
uniform float metallic;
uniform float roughness;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 N = normal;
    
    //these ones i just added
    vec3 V = normalize(cameraPosition - fragPosition);   
    vec3 L = normalize(lightPosition - fragPosition);    
    vec3 H = normalize(V + L);                         

    // --- Set the light direction vector
    vec3 lightDirection = lightDirection_optional;
    if (isSpot == 1)
    { // don't use the direction vector
	lightDirection = normalize(lightPosition - fragPosition);
    }

    /*
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
    */
  
    // all this is for Cook Torrange PBR
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);


    vec3 F0 = mix(vec3(0.04), kd, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);

    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G_V = NdotV / (NdotV * (1.0 - k) + k);
    float G_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G_V * G_L;

    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;


    vec3 kS = F;                 
    vec3 kD = vec3(1.0) - kS;   
    kD *= 1.0 - metallic;       

    vec3 irradiance = lightColor * NdotL;
    vec3 diffuse = kD * kd / 3.14159265;
    vec3 finalColor = (diffuse + specular) * irradiance;
    fragColor = vec4(finalColor, 1.0);
}
