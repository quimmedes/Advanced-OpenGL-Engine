#version 420 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec3 ViewDir;

out vec4 FragColor;

// Simple ocean parameters
uniform vec3 u_oceanDeepColor;
uniform vec3 u_oceanShallowColor;
uniform float u_time;
uniform vec3 viewPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 skyColor;

void main() {
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(ViewDir);
    vec3 lightDir = normalize(-lightDirection);
    
    // Simple fresnel effect
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.0);
    
    // Mix shallow and deep water colors
    vec3 waterColor = mix(u_oceanShallowColor, u_oceanDeepColor, fresnel);
    
    // Simple lighting
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = waterColor * lightColor * NdotL;
    
    // Simple specular highlight
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = lightColor * spec * 0.5;
    
    // Add sky reflection
    vec3 reflection = skyColor * fresnel;
    
    vec3 finalColor = diffuse + specular + reflection + waterColor * 0.1;
    
    // Improved transparency with depth and viewing angle
    float baseAlpha = 0.4; // More transparent base
    float fresnelAlpha = fresnel * 0.5;
    float depthAlpha = clamp(length(FragPos) * 0.01, 0.0, 0.4); // Distance-based opacity
    float alpha = baseAlpha + fresnelAlpha + depthAlpha;
    alpha = clamp(alpha, 0.1, 0.9); // Allow more transparency range
    
    FragColor = vec4(finalColor, alpha);
}