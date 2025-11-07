#version 410

// Material properties - this should match GPUMaterial in mesh.h
layout(std140) uniform Material
{
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};

uniform int useMaterial;
uniform sampler2D colorMap;
uniform sampler2D textureMap;

// Shadow mapping
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Light properties
uniform vec3 lightPosition;
uniform vec3 lightDirection_optional;
uniform vec3 lightColor;
uniform int isSpot;

// PBR properties
uniform vec3 cameraPosition;
uniform float metallic;
uniform float roughness;

in vec3 fragPosition; 
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

// Calculate shadow with PCF
float calculateShadow(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // Transform fragment position to light space
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if fragment is outside light's frustum
    if(projCoords.z > 1.0)
        return 0.0;
    
    // Get closest depth value from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias to prevent shadow acne
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (Percentage Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

void main()
{
    vec3 ambientColor = vec3(0.3, 0.3, 0.3);

    if (useMaterial == 0)
    {
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
    
    // Compute light attenuation
    float distance = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (1.0 + 0.07 * distance + 0.017 * distance * distance);
    
    // Calculate shadow (ALWAYS calculate, even if not fully visible)
    float shadow = calculateShadow(fragPosition, N, L);

    // Light radiance
    vec3 lightRadiance = lightColor * attenuation;

    // Combine components with shadow
    vec3 ambient = ambientColor * textureColor;
    vec3 diffuseSpecular = (diffuse + specular) * lightRadiance * NdotL * (1.0 - shadow * 0.7);

    vec3 finalColor = ambient + diffuseSpecular;

    // Clamp to prevent over saturation
    finalColor = clamp(finalColor, 0.0, 1.0);

    fragColor = vec4(finalColor, 1.0);
}
