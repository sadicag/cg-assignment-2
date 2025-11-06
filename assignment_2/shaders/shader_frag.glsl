#version 410

layout(std140) uniform Material // Must match the GPUMaterial defined in src/mesh.h
{
    vec3 kd;
	vec3 ks;
	float shininess;
	float transparency;
};

uniform int useMaterial;
uniform sampler2D colorMap;

// Butterfly texture properties
uniform sampler2D textureMap;

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

    vec3 ambientColor = vec3(0.2, 0.2, 0.2);

    if (useMaterial == 0)
    { // Don't use material
	fragColor = vec4(fragNormal, 1.0f);
	return;
    }

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(
	    isSpot == 1 ? 
	    lightPosition - fragPosition : 
	    -lightDirection_optional
    );
    vec3 V = normalize(cameraPosition - fragPosition);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // Fresnel
    vec3 F0 = mix(vec3(0.04), kd, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    // Normal Distribution Function (GGX)
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);

    // Geometry (Smith GGX)
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G_V = NdotV / (NdotV * (1.0 - k) + k);
    float G_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G_V * G_L;

    // Cook-Torrance specular
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    // Diffuse and energy conservation
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 diffuse = kD * kd / 3.14159265;

    // Combine lighting
    vec3 textureColor = texture(textureMap, fragTexCoord).rgb;
    
    // Compute light attenuation (optional but helps realism)
    float distance = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (distance * distance); // simple inverse-square falloff

    // Light radiance
    vec3 lightRadiance = lightColor * attenuation;

    // Combine components
    vec3 ambient = ambientColor * textureColor;
    vec3 diffuseSpecular = (diffuse + specular) * lightRadiance;

    vec3 finalColor = ambient + diffuseSpecular;

    // Clamp to prevent over saturation
    finalColor = clamp(finalColor, 0.0, 1.0);

    fragColor = vec4(finalColor, 1.0);
	

}
