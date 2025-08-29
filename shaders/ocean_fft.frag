#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 ViewDir;
in vec4 ClipSpace;
in float FoamFactor;

out vec4 FragColor;

// Lighting
uniform vec3 cameraPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 skyColor;

// Ocean colors
uniform vec3 deepWaterColor = vec3(0.0, 0.1, 0.3);
uniform vec3 shallowWaterColor = vec3(0.1, 0.4, 0.8);
uniform vec3 foamColor = vec3(0.95, 0.98, 1.0);

// Water properties
uniform float fresnelPower = 2.0;
uniform float specularStrength = 1.0;
uniform float roughness = 0.02;
uniform float transparency = 0.8;

const float PI = 3.14159265359;

// Fresnel calculation using Schlick's approximation
float calculateFresnel(vec3 viewDir, vec3 normal, float f0) {
    float cosTheta = max(dot(normalize(viewDir), normal), 0.0);
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, fresnelPower);
}

// PBR specular calculation (Cook-Torrance)
float calculateSpecular(vec3 viewDir, vec3 lightDirection, vec3 normal, float roughness) {
    vec3 halfwayDir = normalize(lightDirection + viewDir);
    float NdotH = max(dot(normal, halfwayDir), 0.0);
    float roughness2 = roughness * roughness;
    float roughness4 = roughness2 * roughness2;
    
    // GGX/Trowbridge-Reitz normal distribution
    float denominator = NdotH * NdotH * (roughness4 - 1.0) + 1.0;
    denominator = PI * denominator * denominator;
    
    float D = roughness4 / max(denominator, 0.0001);
    
    // Geometry function (simplified)
    float NdotV = max(dot(normal, normalize(viewDir)), 0.0);
    float NdotL = max(dot(normal, lightDirection), 0.0);
    float G = min(1.0, min(2.0 * NdotH * NdotV / dot(viewDir, halfwayDir),
                          2.0 * NdotH * NdotL / dot(viewDir, halfwayDir)));
    
    return D * G / (4.0 * NdotV * NdotL + 0.0001);
}

// Simple sky reflection approximation
vec3 getSkyReflection(vec3 reflectionDir) {
    // Simple gradient based on reflection direction
    float t = max(0.0, reflectionDir.y);
    return mix(skyColor * 0.8, skyColor, t);
}

// Calculate water depth color based on viewing angle and distance
vec3 getWaterColor(vec3 viewDirection, vec3 normal, float depth) {
    float fresnel = calculateFresnel(viewDirection, normal, 0.02);
    
    // Depth-based color transition
    float depthFactor = clamp(depth * 0.05, 0.0, 1.0);
    vec3 waterColor = mix(shallowWaterColor, deepWaterColor, depthFactor);
    
    // Apply fresnel for surface reflection vs transmission
    return waterColor * (1.0 - fresnel * 0.7);
}

// Subsurface scattering approximation
vec3 calculateSubsurface(vec3 lightDirection, vec3 viewDirection, vec3 normal) {
    vec3 scatterDir = lightDirection + normal * 0.2;
    float scatter = pow(max(0.0, dot(viewDirection, -scatterDir)), 4.0);
    return shallowWaterColor * scatter * 0.3;
}

void main() {
    // Normalize vectors
    vec3 normal = normalize(Normal);
    vec3 viewDirection = normalize(ViewDir);
    vec3 lightDirection = normalize(-lightDir);
    
    // Calculate reflection vector
    vec3 reflectionVector = reflect(-viewDirection, normal);
    
    // Get sky reflection
    vec3 skyReflection = getSkyReflection(reflectionVector);
    
    // Calculate Fresnel effect
    float fresnel = calculateFresnel(viewDirection, normal, 0.02);
    
    // Calculate specular highlights
    float specular = calculateSpecular(viewDirection, lightDirection, normal, roughness);
    specular *= specularStrength;
    
    // Calculate water color based on depth (use fragment distance as approximation)
    float depth = length(FragPos);
    vec3 waterColor = getWaterColor(viewDirection, normal, depth);
    
    // Calculate lighting
    float NdotL = max(dot(normal, lightDirection), 0.0);
    vec3 diffuse = waterColor * lightColor * NdotL;
    
    // Subsurface scattering
    vec3 subsurface = calculateSubsurface(lightDirection, viewDirection, normal);
    
    // Combine lighting components
    vec3 finalColor = diffuse + subsurface;
    
    // Apply reflections based on fresnel
    finalColor = mix(finalColor, skyReflection, fresnel * 0.8);
    
    // Add specular highlights
    finalColor += lightColor * specular * (1.0 + fresnel);
    
    // Apply foam
    finalColor = mix(finalColor, foamColor, FoamFactor * 0.8);
    
    // HDR tone mapping (Reinhard)
    finalColor = finalColor / (finalColor + vec3(1.0));
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    // Calculate transparency
    float alpha = transparency;
    
    // Increase opacity with fresnel effect
    alpha = mix(alpha * 0.7, alpha, fresnel);
    
    // Foam increases opacity
    alpha = mix(alpha, 1.0, FoamFactor * 0.6);
    
    // Distance-based transparency for depth perception
    float viewDistance = length(ViewDir);
    float distanceAlpha = clamp(viewDistance * 0.001, 0.0, 0.3);
    alpha = clamp(alpha + distanceAlpha, 0.1, 0.95);
    
    FragColor = vec4(finalColor, alpha);
}