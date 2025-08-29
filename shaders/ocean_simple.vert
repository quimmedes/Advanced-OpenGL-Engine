#version 420 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec3 ViewDir;

// Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

// Simple wave parameters
uniform float u_time;
uniform float u_waveAmplitude;

void main() {
    // Simple animated waves to prevent complex shader issues
    vec3 worldPos = (model * vec4(aPos, 1.0)).xyz;
    
    // Add simple sine wave animation
    worldPos.y += sin(worldPos.x * 0.1 + u_time) * u_waveAmplitude * 0.1;
    worldPos.y += cos(worldPos.z * 0.15 + u_time * 0.8) * u_waveAmplitude * 0.05;
    
    FragPos = worldPos;
    Normal = normalize((model * vec4(aNormal, 0.0)).xyz);
    TexCoords = aTexCoords;
    ViewDir = normalize(viewPos - worldPos);
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
}