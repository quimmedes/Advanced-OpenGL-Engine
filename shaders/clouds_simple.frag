#version 420 core

in vec3 FragPos;
in vec3 ViewRay;

out vec4 FragColor;

// Simple cloud parameters
uniform float u_time;
uniform vec3 viewPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 skyColor;
uniform float u_cloudCoverage;
uniform float u_cloudHeight;
uniform float u_cloudThickness;

// Simple noise function
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

float noise3D(vec3 pos) {
    vec3 p = floor(pos);
    vec3 f = fract(pos);
    f = f * f * (3.0 - 2.0 * f);
    
    float n = p.x + p.y * 57.0 + 113.0 * p.z;
    return mix(
        mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
            mix(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
        mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
            mix(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

void main() {
    vec3 rayDir = normalize(ViewRay);
    vec3 rayStart = viewPos;
    
    // Simple cloud layer check
    float cloudLayerStart = u_cloudHeight - u_cloudThickness * 0.5;
    float cloudLayerEnd = u_cloudHeight + u_cloudThickness * 0.5;
    
    // Check if ray intersects cloud layer
    if (rayDir.y <= 0.0 && rayStart.y < cloudLayerStart) {
        discard; // Ray going down and below clouds
    }
    
    // Simple ray marching
    float t = 0.0;
    float density = 0.0;
    vec3 lightAccum = vec3(0.0);
    
    // Find intersection with cloud layer
    if (rayStart.y < cloudLayerStart && rayDir.y > 0.0) {
        t = (cloudLayerStart - rayStart.y) / rayDir.y;
    }
    
    // Sample clouds
    for (int i = 0; i < 16; i++) {
        vec3 samplePos = rayStart + rayDir * t;
        
        if (samplePos.y > cloudLayerEnd) break;
        if (samplePos.y < cloudLayerStart) {
            t += 50.0;
            continue;
        }
        
        // Sample noise for cloud density
        vec3 noisePos = samplePos * 0.001 + vec3(u_time * 0.01, 0.0, u_time * 0.005);
        float noise = noise3D(noisePos);
        noise += noise3D(noisePos * 2.0) * 0.5;
        noise += noise3D(noisePos * 4.0) * 0.25;
        
        float cloudDensity = smoothstep(1.0 - u_cloudCoverage, 1.0, noise) * 0.1;
        
        if (cloudDensity > 0.01) {
            // Simple lighting
            vec3 lightDir = normalize(-lightDirection);
            float lighting = max(0.3, dot(vec3(0, 1, 0), lightDir));
            
            lightAccum += lightColor * cloudDensity * lighting;
            density += cloudDensity;
        }
        
        t += 50.0;
    }
    
    // Blend with sky
    vec3 finalColor = mix(skyColor, lightAccum / max(density, 0.01), min(density * 3.0, 1.0));
    float alpha = min(density * 2.0, 0.8);
    
    if (alpha < 0.01) discard;
    
    FragColor = vec4(finalColor, alpha);
}