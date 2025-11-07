#version 410

//Uniforms for Blinn Phong
uniform int use_blinnPhong;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;

//Uniforms for Cook Torrance GGX
uniform int usePBR;
uniform float metallic;
uniform float roughness;
uniform vec3 kdPBR;


uniform int showNormals;
uniform sampler2D colorMap;
uniform sampler2D textureMap;

// Shadow mapping
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform bool isShadow;

// Light properties
uniform vec3 lightPosition;
uniform vec3 lightDirection_optional;
uniform vec3 lightColor;
uniform int isSpot;

//Cam uniform
uniform vec3 cameraPosition;


// Environment mapping
uniform samplerCube environmentMap;
uniform int useEnvironmentMapping;
uniform float reflectivity;

in vec3 fragPosition; 
in vec3 fragNormal;
in vec2 fragTexCoord;

layout(location = 0) out vec4 fragColor;

float calculateShadow(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    // Transform fragment position to light space
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if fragment is outside light's frustum
    // If outside, assume it's in shadow (or no shadow, depending on your preference)
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0 || 
       projCoords.z > 1.0)
        return 1.0; // Return 1.0 for full shadow outside bounds
    
    // Get closest depth value from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
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

const float ambientStrength = 0.2;


void main()
{
    // ---- Debug normals ----
    if (showNormals == 1) {
        vec3 n = normalize(fragNormal) * 0.5 + 0.5;
        fragColor = vec4(n, 1.0);
        return;
    }

    // ---- Basis ----
    vec3 N = normalize(fragNormal);
    if (!gl_FrontFacing)
        N = -N;

    vec3 L = (isSpot == 1)
           ? normalize(lightPosition - fragPosition)
           : normalize(-lightDirection_optional);
    vec3 V = normalize(cameraPosition - fragPosition);
    vec3 H = normalize(L + V);

    // ---- Shadow ----
    float shadow = calculateShadow(fragPosition, N, L);
    float shadowFactor = 1.0 - shadow;

    // ---- Texture and base color ----
    vec3 tex = texture(textureMap, fragTexCoord).rgb;

    // ---- Attenuation ----
    float attConstant = 1.0;
    float attLinear = 0.09;
    float attQuadratic = 0.032;
    float attenuation = 1.0;
    if (isSpot == 1) {
        float dist = length(lightPosition - fragPosition);
        //attenuation = 1.0 / (attConstant + attLinear * dist + attQuadratic * dist * dist);
    }
    vec3 radiance = lightColor * attenuation;


    if (use_blinnPhong == 1) {
        vec3 albedo = kd * tex;

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0);

        vec3 ambient  = albedo * ambientStrength;
        vec3 diffuse  = albedo * NdotL;
        float specPow = (NdotL > 0.0) ? pow(NdotH, shininess) : 0.0;
        vec3 specular = ks * specPow;

        vec3 lighting = ambient + (diffuse + specular) * radiance * shadowFactor;
        fragColor = vec4(clamp(lighting, 0.0, 1.0), 1.0);
        return;
    }


    if (usePBR == 1) {
        vec3 albedo = kdPBR * tex; 
        // Core dot products
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // Fresnel (Schlick approximation)
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

        // GGX normal distribution
        float alpha = roughness * roughness; 
        float alpha2 = alpha * alpha;
        float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
        float D = alpha2 / (3.14159265 * denom * denom);

        // Smith geometry term
        float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;  
        float G_V = NdotV / (NdotV * (1.0 - k) + k);
        float G_L = NdotL / (NdotL * (1.0 - k) + k);
        float G = G_V * G_L;

        // Specular term
        vec3 numerator = D * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.001;
        vec3 specular = numerator / denominator;

        // Diffuse + energy conservation
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic); 
        vec3 diffuse = kD * albedo / 3.14159265;

        // Combine lighting
        vec3 color = (diffuse + specular) * radiance * NdotL * shadowFactor;

        // Ambient (simple fallback, no IBL yet)
        color += albedo * ambientStrength;

        // Gamma correction
        color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
        fragColor = vec4(color, 1.0);
        return;
    }

    // if both PBR and Blinn Phong are off 
    fragColor = vec4(texture(textureMap, fragTexCoord).rgb, 1.0);
}