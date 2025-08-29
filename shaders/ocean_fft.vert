#version 420 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 ViewDir;
out vec4 ClipSpace;
out float FoamFactor;

// Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Ocean parameters
uniform float time;
uniform float oceanSize;
uniform float choppiness;
uniform bool enableChoppiness;

// Textures from FFT computation
uniform sampler2D heightTexture;
uniform sampler2D displacementTexture;
uniform sampler2D normalTexture;
uniform sampler2D foamTexture;

// Camera
uniform vec3 cameraPos;

void main() {
    TexCoords = aTexCoord;
    
    // Sample height from FFT-generated heightmap
    float height = texture(heightTexture, TexCoords).r;
    
    // Sample displacement for choppiness
    vec3 displacement = vec3(0.0);
    if (enableChoppiness) {
        vec4 displace = texture(displacementTexture, TexCoords);
        displacement = vec3(displace.x, 0.0, displace.z) * choppiness;
    }
    
    // Apply displacement to vertex position
    vec3 worldPos = aPosition + vec3(0.0, height, 0.0) + displacement;
    
    // Calculate matrices
    vec4 worldPosition = model * vec4(worldPos, 1.0);
    vec4 viewPosition = view * worldPosition;
    
    FragPos = worldPosition.xyz;
    ClipSpace = projection * viewPosition;
    gl_Position = ClipSpace;
    
    // Sample normal from FFT-generated normal map
    vec3 sampledNormal = texture(normalTexture, TexCoords).xyz;
    // Convert from [0,1] range back to [-1,1]
    sampledNormal = sampledNormal * 2.0 - 1.0;
    
    // Transform normal to world space
    Normal = mat3(transpose(inverse(model))) * sampledNormal;
    
    // Calculate view direction
    ViewDir = cameraPos - FragPos;
    
    // Sample foam factor
    FoamFactor = texture(foamTexture, TexCoords).r;
}