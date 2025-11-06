#version 410

layout(std140) uniform Material {
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};

uniform sampler2D textureMap;

// Light properties
uniform vec3 lightPosition;
uniform vec3 lightDirection_optional;
uniform vec3 lightColor;
uniform int isSpot;

uniform vec3 cameraPosition;
uniform float metallic;
uniform float roughness;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

void main()
{
    
    // === Base setup ===
    vec3 N = normalize(fragNormal);
    if (!gl_FrontFacing)
    N = -N;
    vec3 L = normalize(lightPosition - fragPosition);
    vec3 V = normalize(cameraPosition - fragPosition);
    vec3 H = normalize(V + L);
    
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    // === Fresnel term ===
    vec3 F0 = mix(vec3(0.04), kd, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

    // === GGX Normal distribution ===
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);

    // === Geometry (Smith GGX) ===
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G_V = NdotV / (NdotV * (1.0 - k) + k);
    float G_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G_V * G_L;

    // === Specular reflection ===
    vec3 numerator = D * G * F;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    vec3 specular = numerator / denominator;

    // === Diffuse term ===
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    vec3 baseColor = texture(textureMap, fragTexCoord).rgb;
    vec3 diffuse = kD * baseColor / 3.14159265;

    // === Light attenuation ===
    float distance = length(lightPosition - fragPosition);
    float attenuation = 0.6;
    vec3 radiance = lightColor * attenuation;

    // === Combine all ===
    vec3 ambient = 0.4 * baseColor;
    vec3 color = ambient + (diffuse + specular) * radiance * NdotL;

    fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
    
    //fragColor = vec4(N * 0.5 + 0.5, 1.0);
 
}