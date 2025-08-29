#version 420 core

in vec3 FragPos;
in vec3 ViewRay;

out vec4 FragColor;

// Cloud system parameters
uniform float u_time;
uniform vec3 viewPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 skyColor;

// Cloud properties
uniform float u_cloudCoverage;
uniform float u_cloudDensity;
uniform float u_cloudScale;
uniform float u_cloudSpeed;
uniform float u_cloudHeight;
uniform float u_cloudThickness;
uniform vec3 u_windDirection;

// Ray marching parameters
uniform int u_numSteps;
uniform int u_numLightSteps;
uniform float u_maxDistance;

// Noise parameters
uniform float u_noiseScale;
uniform float u_noiseStrength;
uniform int u_octaves;

const float PI = 3.14159265359;

// 3D Noise functions for volumetric clouds
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

float noise3D(vec3 x) {
    vec3 p = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    
    float n = p.x + p.y * 57.0 + 113.0 * p.z;
    return mix(
        mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
            mix(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
        mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
            mix(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
}

// Fractal Brownian Motion for realistic cloud shapes
float fbm(vec3 pos, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for(int i = 0; i < octaves; i++) {
        value += amplitude * noise3D(pos * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    
    return value;
}

// Worley noise for cloud structure
float worleyNoise(vec3 pos) {
    vec3 id = floor(pos);
    vec3 p = fract(pos);
    
    float minDist = 10.0;
    for(int x = -1; x <= 1; x++) {
        for(int y = -1; y <= 1; y++) {
            for(int z = -1; z <= 1; z++) {
                vec3 offset = vec3(x, y, z);
                vec3 h = hash(dot(id + offset, vec3(1.0, 57.0, 113.0))) * vec3(1.0);
                h = 0.5 + 0.5 * sin(u_time * 0.01 + 6.2831 * h);
                vec3 r = offset + h - p;
                minDist = min(minDist, dot(r, r));
            }
        }
    }
    
    return 1.0 - minDist;
}

// Cloud density calculation
float getCloudDensity(vec3 pos) {
    // Move clouds with wind
    vec3 windPos = pos + u_windDirection * u_time * u_cloudSpeed;
    
    // Base cloud shape using FBM
    float baseNoise = fbm(windPos * u_cloudScale, u_octaves);
    
    // Add detail with Worley noise
    float detailNoise = worleyNoise(windPos * u_cloudScale * 4.0) * 0.5;
    
    // Combine noises
    float density = baseNoise + detailNoise * u_noiseStrength;
    
    // Apply coverage threshold
    density = clamp(density - (1.0 - u_cloudCoverage), 0.0, 1.0) * u_cloudDensity;
    
    // Height-based attenuation (clouds get thinner at edges)
    float heightFactor = 1.0 - abs(pos.y - u_cloudHeight) / (u_cloudThickness * 0.5);
    heightFactor = clamp(heightFactor, 0.0, 1.0);
    heightFactor = smoothstep(0.0, 1.0, heightFactor);
    
    return density * heightFactor;
}

// Light scattering calculation
float getScattering(vec3 pos, vec3 lightDir) {
    float density = 0.0;
    vec3 step = lightDir * (u_cloudThickness / float(u_numLightSteps));
    
    for(int i = 0; i < u_numLightSteps; i++) {
        density += getCloudDensity(pos + step * float(i));
    }
    
    return exp(-density * 0.1); // Beer's law approximation
}

// Henyey-Greenstein phase function for realistic scattering
float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

// Main ray marching function
vec4 rayMarchClouds(vec3 rayStart, vec3 rayDir, float maxDist) {
    vec3 pos = rayStart;
    vec3 step = rayDir * (maxDist / float(u_numSteps));
    
    float totalDensity = 0.0;
    vec3 totalLighting = vec3(0.0);
    float transmittance = 1.0;
    
    for(int i = 0; i < u_numSteps && transmittance > 0.01; i++) {
        float density = getCloudDensity(pos);
        
        if(density > 0.01) {
            // Calculate lighting
            vec3 lightDir = normalize(-lightDirection);
            float scattering = getScattering(pos, lightDir);
            
            // Phase function for forward/backward scattering
            float cosTheta = dot(rayDir, lightDir);
            float phase = mix(henyeyGreenstein(cosTheta, 0.3), 
                            henyeyGreenstein(cosTheta, -0.3), 0.7);
            
            // Calculate lighting contribution
            vec3 lighting = lightColor * scattering * phase;
            
            // Add ambient sky light
            lighting += skyColor * 0.3;
            
            // Apply powder effect (darker clouds are less illuminated)
            float powder = 1.0 - exp(-density * 2.0);
            lighting *= powder;
            
            // Accumulate lighting and density
            totalLighting += lighting * density * transmittance * 0.1;
            totalDensity += density * 0.1;
            
            // Update transmittance
            transmittance *= exp(-density * 0.1);
        }
        
        pos += step;
    }
    
    // Apply energy conservation
    totalLighting /= max(totalDensity, 0.01);
    
    float alpha = 1.0 - transmittance;
    return vec4(totalLighting, alpha);
}

// Ray-box intersection for cloud bounds
bool rayBoxIntersection(vec3 rayStart, vec3 rayDir, vec3 boxMin, vec3 boxMax, out float tNear, out float tFar) {
    vec3 invDir = 1.0 / rayDir;
    vec3 t1 = (boxMin - rayStart) * invDir;
    vec3 t2 = (boxMax - rayStart) * invDir;
    
    vec3 tMin = min(t1, t2);
    vec3 tMax = max(t1, t2);
    
    tNear = max(max(tMin.x, tMin.y), tMin.z);
    tFar = min(min(tMax.x, tMax.y), tMax.z);
    
    return tNear <= tFar && tFar > 0.0;
}

void main() {
    vec3 rayDir = normalize(ViewRay);
    vec3 rayStart = viewPos;
    
    // Define cloud bounding box
    vec3 cloudBoxMin = vec3(-500.0, u_cloudHeight - u_cloudThickness * 0.5, -500.0);
    vec3 cloudBoxMax = vec3(500.0, u_cloudHeight + u_cloudThickness * 0.5, 500.0);
    
    float tNear, tFar;
    if(!rayBoxIntersection(rayStart, rayDir, cloudBoxMin, cloudBoxMax, tNear, tFar)) {
        discard; // Ray doesn't intersect cloud volume
    }
    
    // Adjust ray start if camera is inside cloud volume
    if(tNear < 0.0) tNear = 0.0;
    
    vec3 marchStart = rayStart + rayDir * tNear;
    float marchDistance = min(tFar - tNear, u_maxDistance);
    
    // Perform ray marching
    vec4 cloudResult = rayMarchClouds(marchStart, rayDir, marchDistance);
    
    // Apply atmospheric perspective
    float distance = tNear + marchDistance * 0.5;
    float atmosphere = exp(-distance * 0.0001);
    cloudResult.rgb = mix(skyColor, cloudResult.rgb, atmosphere);
    
    // Gamma correction
    cloudResult.rgb = pow(cloudResult.rgb, vec3(1.0/2.2));
    
    FragColor = cloudResult;
}