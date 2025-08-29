#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 ClipSpace;
in vec3 ToCameraVector;
in vec3 FromLightVector;
in float Visibility;

out vec4 FragColor;

// Ocean properties
uniform vec3 u_oceanDeepColor;
uniform vec3 u_oceanShallowColor;
uniform float u_fresnelStrength;
uniform float u_specularStrength;
uniform float u_roughness;
uniform float u_transparency;
uniform float u_refractionStrength;

// Environment
uniform sampler2D u_dudvMap;
uniform sampler2D u_normalMap;
uniform float u_time;
uniform vec3 viewPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 skyColor;

const float PI = 3.14159265359;

// Fresnel calculation
float calculateFresnel(vec3 viewDirection, vec3 normal, float fresnelFactor) {
    float angle = dot(normalize(viewDirection), normal);
    angle = max(angle, 0.0);
    float fresnel = pow(1.0 - angle, fresnelFactor);
    return clamp(fresnel, 0.0, 1.0);
}

// PBR specular calculation
float calculateSpecular(vec3 viewDirection, vec3 lightDirection, vec3 normal, float roughness) {
    vec3 halfwayDir = normalize(lightDirection + viewDirection);
    float NdotH = max(dot(normal, halfwayDir), 0.0);
    float roughness2 = roughness * roughness;
    float roughness4 = roughness2 * roughness2;
    
    // GGX/Trowbridge-Reitz distribution
    float denominator = NdotH * NdotH * (roughness4 - 1.0) + 1.0;
    denominator = PI * denominator * denominator;
    
    return roughness4 / max(denominator, 0.0001);
}

// Ocean depth-based color mixing
vec3 calculateWaterColor(vec3 viewDirection, vec3 normal, float depth) {
    float fresnel = calculateFresnel(viewDirection, normal, u_fresnelStrength);
    
    // Depth-based color transition
    float depthFactor = clamp(depth * 0.1, 0.0, 1.0);
    vec3 waterColor = mix(u_oceanShallowColor, u_oceanDeepColor, depthFactor);
    
    // Apply fresnel for surface reflection vs transmission
    return waterColor * (1.0 - fresnel * 0.8);
}

// Noise function for procedural water effects
float noise(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Foam calculation based on wave steepness and shore proximity
float calculateFoam(vec3 worldPos, vec3 normal, float time) {
    // Wave-based foam
    float waveHeight = worldPos.y;
    float foamFromWaves = clamp(waveHeight * 2.0 - 0.5, 0.0, 1.0);
    
    // Animated foam texture
    vec2 foamCoords = worldPos.xz * 0.05 + normal.xz * 0.1;
    float foamNoise = noise(foamCoords + time * 0.1);
    foamNoise += noise(foamCoords * 2.0 + time * 0.15) * 0.5;
    foamNoise += noise(foamCoords * 4.0 + time * 0.2) * 0.25;
    
    return foamFromWaves * foamNoise;
}

void main() {
    // Normalize vectors
    vec3 viewDirection = normalize(ToCameraVector);
    vec3 lightDir = normalize(FromLightVector);
    vec3 normal = normalize(Normal);
    
    // Add simple procedural normal variation instead of texture sampling
    vec2 waveCoords = TexCoords * 8.0 + u_time * 0.1;
    vec3 proceduralNormal = vec3(
        sin(waveCoords.x * 2.0) * 0.1,
        sin(waveCoords.y * 2.0) * 0.1,
        1.0
    );
    normal = normalize(normal + proceduralNormal * 0.2);
    
    // Calculate reflection vector
    vec3 reflectionVector = reflect(-viewDirection, normal);
    
    // Use procedural sky color for reflections instead of skybox
    vec3 reflection = skyColor;
    // Add simple directional variation based on reflection vector
    reflection += skyColor * 0.3 * max(0.0, reflectionVector.y);
    
    // Calculate Fresnel effect
    float fresnel = calculateFresnel(viewDirection, normal, u_fresnelStrength);
    
    // Calculate specular highlights
    float specular = calculateSpecular(viewDirection, lightDir, normal, u_roughness);
    specular *= u_specularStrength;
    
    // Depth-based water color (simplified - would need actual depth buffer)
    float fakeDepth = clamp(FragPos.y + 10.0, 0.0, 20.0);
    vec3 waterColor = calculateWaterColor(viewDirection, normal, fakeDepth);
    
    // Calculate lighting
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = waterColor * lightColor * NdotL;
    
    // Subsurface scattering approximation
    float subsurface = pow(max(0.0, dot(-lightDir, viewDirection)), 2.0);
    vec3 subsurfaceColor = u_oceanShallowColor * subsurface * 0.3;
    
    // Foam calculation
    float foam = calculateFoam(FragPos, normal, u_time);
    
    // Combine all lighting components
    vec3 finalColor = diffuse + subsurfaceColor;
    
    // Apply reflections based on fresnel
    finalColor = mix(finalColor, reflection, fresnel * 0.8);
    
    // Add specular highlights
    finalColor += lightColor * specular;
    
    // Add foam
    finalColor = mix(finalColor, vec3(1.0), foam * 0.7);
    
    // Apply fog/atmospheric perspective
    finalColor = mix(skyColor, finalColor, Visibility);
    
    // HDR tone mapping (Reinhard)
    finalColor = finalColor / (finalColor + vec3(1.0));
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(1.0/2.2));
    
    // Enhanced ocean transparency with better foam integration
    float baseTransparency = u_transparency * 0.7; // More transparent base
    float alpha = mix(baseTransparency, 0.8, fresnel * 0.6);
    
    // Foam increases opacity more gradually
    alpha = mix(alpha, min(alpha + 0.4, 0.95), foam * 0.8);
    
    // Distance-based transparency for depth perception
    float viewDistance = length(FragPos);
    float distanceFade = clamp(viewDistance * 0.008, 0.0, 0.3);
    alpha = clamp(alpha + distanceFade, 0.05, 0.95);
    
    FragColor = vec4(finalColor, alpha);
}