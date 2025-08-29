#version 420 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 ClipSpace;
out vec3 ToCameraVector;
out vec3 FromLightVector;
out float Visibility;

// Matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Ocean parameters
uniform float u_time;
uniform vec2 u_windDirection;
uniform float u_windSpeed;
uniform float u_waveAmplitude;
uniform float u_waveFrequency;
uniform int u_numWaves;

// Lighting
uniform vec3 viewPos;
uniform vec3 lightDirection;

// Wave structures for complex wave simulation
struct Wave {
    vec2 direction;
    float amplitude;
    float frequency;
    float phase;
    float steepness;
};

// Gerstner wave implementation for realistic ocean waves
vec3 GerstnerWave(vec2 pos, Wave wave, float time, inout vec3 tangent, inout vec3 binormal) {
    float c = sqrt(9.8 / wave.frequency); // Wave speed from physics
    vec2 d = normalize(wave.direction);
    float f = wave.frequency;
    float a = wave.amplitude;
    float phi = wave.phase;
    float q = wave.steepness / (wave.frequency * a * float(u_numWaves));
    
    float theta = dot(d, pos) * f + time * c + phi;
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);
    
    // Displacement
    vec3 displacement;
    displacement.x = q * a * d.x * cosTheta;
    displacement.z = q * a * d.y * cosTheta;
    displacement.y = a * sinTheta;
    
    // Tangent and binormal for normal calculation
    tangent.x += -q * d.x * d.x * f * a * sinTheta;
    tangent.y += d.x * f * a * cosTheta;
    tangent.z += -q * d.x * d.y * f * a * sinTheta;
    
    binormal.x += -q * d.x * d.y * f * a * sinTheta;
    binormal.y += d.y * f * a * cosTheta;
    binormal.z += -q * d.y * d.y * f * a * sinTheta;
    
    return displacement;
}

void main() {
    vec3 worldPos = (model * vec4(aPos, 1.0)).xyz;
    vec2 pos2D = worldPos.xz;
    
    // Initialize wave system
    Wave waves[8];
    
    // Setup wave parameters based on wind
    vec2 windDir = normalize(u_windDirection);
    float baseFreq = u_waveFrequency;
    float baseAmp = u_waveAmplitude;
    
    // Generate wave set with physically based parameters
    for(int i = 0; i < min(u_numWaves, 8); i++) {
        float angle = float(i) * 0.785398; // 45 degrees in radians
        vec2 dir = vec2(cos(angle), sin(angle));
        dir = mix(dir, windDir, 0.7); // Bias towards wind direction
        
        waves[i].direction = dir;
        waves[i].frequency = baseFreq * (1.0 + float(i) * 0.3);
        waves[i].amplitude = baseAmp / (1.0 + float(i) * 0.5);
        waves[i].phase = float(i) * 1.57079; // 90 degrees
        waves[i].steepness = 0.8 / (waves[i].frequency * waves[i].amplitude * float(u_numWaves));
    }
    
    // Calculate wave displacement
    vec3 totalDisplacement = vec3(0.0);
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    
    for(int i = 0; i < min(u_numWaves, 8); i++) {
        totalDisplacement += GerstnerWave(pos2D, waves[i], u_time, tangent, binormal);
    }
    
    // Apply displacement
    worldPos += totalDisplacement;
    FragPos = worldPos;
    
    // Calculate normal from tangent and binormal
    vec3 normal = normalize(cross(binormal, tangent));
    Normal = normal;
    
    TexCoords = aTexCoords + totalDisplacement.xz * 0.01; // Slight texture distortion
    
    // Calculate clip space position
    gl_Position = projection * view * vec4(worldPos, 1.0);
    ClipSpace = gl_Position;
    
    // Lighting vectors
    ToCameraVector = viewPos - worldPos;
    FromLightVector = -lightDirection;
    
    // Fog/distance visibility
    float distance = length(viewPos - worldPos);
    Visibility = exp(-pow(distance * 0.0015, 1.5));
    Visibility = clamp(Visibility, 0.0, 1.0);
}