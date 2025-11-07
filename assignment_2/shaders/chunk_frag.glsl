#version 410

// Material uniforms

layout(std140) uniform Material
{
    vec3 kd;
    vec3 ks;
    float shininess;
    float transparency;
};


uniform int showNormals;
uniform sampler2D colorMap;
uniform sampler2D textureMap;
uniform sampler2D normalMap;
uniform int useNormalMapping;

// Shadow mapping
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform bool isShadow;

// Light uniforms
uniform vec3 lightPosition;
uniform vec3 lightDirection_optional;
uniform vec3 lightColor;
uniform int isSpot;

// PBR uniforms
uniform vec3 cameraPosition;
uniform float metallic;
uniform float roughness;

// Environment mapping uniforms
uniform samplerCube environmentMap;
uniform int useEnvironmentMapping;
uniform float reflectivity;

in vec3 fragPosition; 
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec3 fragTangent;
in vec3 fragBitangent;

layout(location = 0) out vec4 fragColor;

float calculateShadow(vec3 fragPos, vec3 normal, vec3 lightDir)
{
    //  fragment position to light space
    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(fragPos, 1.0);
    
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to correct range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if fragment is outside light frustrum
    // If outside, shadow
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0 || 
       projCoords.z > 1.0)
        return 1.0; 
    
    // closest depth value from shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    //  bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // PCF for softer shadows
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
    if(showNormals == 1){
        fragColor = vec4( 0.5 * 0.5 + normalize(fragNormal),1.0f);
        return;
    }
    vec3 ambientColor = vec3(0.3, 0.3, 0.3);
    
    
    // sample normal from normal map and apply normal mapping
    vec3 N;
    if (useNormalMapping == 1) {
        // repeat the normal map texture coordinates
        vec2 repeatedTexCoord = fragTexCoord * 10.0; // adjust repetition factor as needed
        
        // Sample normal map
        vec3 normalMapSample = texture(normalMap, repeatedTexCoord).rgb;
        // Transform from [0,1] to [-1,1]
        normalMapSample = normalMapSample * 2.0 - 1.0;
        
        // build TBN matrix
        vec3 T = normalize(fragTangent);
        vec3 B = normalize(fragBitangent);
        vec3 Nbase = normalize(fragNormal);
        mat3 TBN = mat3(T, B, Nbase);
        
        // transoform normal from tangent space to world space
        N = normalize(TBN * normalMapSample);
    } else {
        N = normalize(fragNormal);
    }
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

    // Normal Distributoin for GGX
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);

    // Smith
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

    //  light attenuation
    float distance = length(lightPosition - fragPosition);
    float attenuation = 1.0 / (1.0 + 0.07 * distance + 0.017 * distance * distance);

    // Calculate shadow
    //float shadow = calculateShadow(fragPosition, N, L);
    float shadow = isShadow ? calculateShadow(fragPosition, N, L) : 0.0; 

    // Light radiance
    vec3 lightRadiance = lightColor * attenuation;

    // Shadow factor
    float shadowFactor = 1.0 - shadow * 0.7;

    // Ambient lighting
    vec3 ambient = lightColor * 0.3 * textureColor * shadowFactor;
    vec3 baseAmbient = vec3(0.2, 0.2, 0.2) * textureColor; // Minimum visibility

    // direct lighting (affected by light color and shadows)
    vec3 directLighting = (diffuse + specular) * textureColor * lightRadiance * NdotL * shadowFactor;

    vec3 finalColor = baseAmbient + ambient + directLighting;

    // clamp to prevent over saturation
    finalColor = clamp(finalColor, 0.0, 1.0);

    // if environment mapping
    if (useEnvironmentMapping == 1) {
	// we compute reflection vector
	vec3 I = normalize(fragPosition - cameraPosition);
	vec3 R = reflect(I, N);
	
	// sample the environment map
	vec3 reflection = texture(environmentMap, R).rgb;
	
	// blend reflection with existing color based on reflectivity
	// fresnel effect: more reflective at grazing angles
	float fresnelFactor = pow(1.0 - max(dot(N, V), 0.0), 5.0);
	float finalReflectivity = mix(reflectivity * 0.3, reflectivity, fresnelFactor);
	
	finalColor = mix(finalColor, reflection, finalReflectivity);
    } 

    fragColor = vec4(finalColor, 1.0); 
}
