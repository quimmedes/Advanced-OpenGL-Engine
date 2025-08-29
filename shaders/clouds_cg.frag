#version 430

in vec3 tc;
out vec4 fragColor;

uniform float time;
uniform vec3 lightDirection;
uniform vec3 skyColor;
uniform float cloudCoverage;
uniform float cloudDensity;

// 3D Procedural noise from the book (Chapter on procedural textures)
// Implementation based on Perlin noise as described in the CG book

float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

float noise3d(vec3 x) {
    // 3D noise implementation from the book
    vec3 p = floor(x);
    vec3 f = fract(x);
    
    // Smooth interpolation
    f = f * f * (3.0 - 2.0 * f);
    
    // Calculate noise at 8 corners of the cube
    float n = p.x + p.y * 57.0 + 113.0 * p.z;
    float res = mix(
        mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
            mix(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
        mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
            mix(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
    return res;
}

// Fractal Brownian Motion as described in the book
float fbm(vec3 p) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    // Octave-based noise accumulation from the book
    for (int i = 0; i < 5; i++) {
        value += amplitude * noise3d(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main(void)
{
    vec3 rayDir = normalize(tc);
    
    // Early exit for rays pointing downward (below horizon)
    if (rayDir.y < 0.0) {
        fragColor = vec4(skyColor, 1.0);
        return;
    }
    
    // Cloud layer simulation from the book's volumetric rendering chapter
    float cloudLayerHeight = 0.1;  // Lower threshold for better visibility
    float cloudThickness = 0.6;
    
    // Render clouds in more of the sky
    if (rayDir.y < cloudLayerHeight) {
        fragColor = vec4(skyColor, 1.0);
        return;
    }
    
    // Sample noise for cloud density
    vec3 cloudPos = rayDir + vec3(time * 0.02, 0.0, time * 0.01);
    float cloudNoise = fbm(cloudPos * 3.0);
    
    // Apply cloud coverage threshold with better visibility
    float density = smoothstep(0.3, 1.0, cloudNoise) * cloudDensity;
    density = mix(0.0, density, cloudCoverage);
    
    if (density < 0.05) {
        fragColor = vec4(skyColor, 1.0);
        return;
    }
    
    // Simple lighting model from the book
    vec3 lightDir = normalize(-lightDirection);
    float lighting = max(0.4, dot(vec3(0, 1, 0), lightDir));
    
    // Cloud color based on lighting - make more visible
    vec3 cloudColor = vec3(0.9, 0.9, 0.95) * lighting;
    cloudColor = mix(vec3(0.4, 0.5, 0.6), vec3(1.0), lighting);
    
    // Blend with sky color with stronger contrast
    float cloudAlpha = clamp(density * 2.0, 0.0, 0.9);
    vec3 finalColor = mix(skyColor, cloudColor, cloudAlpha);
    
    fragColor = vec4(finalColor, 1.0);
}