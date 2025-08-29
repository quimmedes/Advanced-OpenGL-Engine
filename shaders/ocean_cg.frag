#version 430

in vec3 varyingNormal;
in vec3 varyingLightDir;
in vec3 varyingVertPos;
in vec3 varyingHalfVector;
in vec2 tc;
in vec4 screenPos;
in vec3 worldPos;
in float waterDepth;

out vec4 fragColor;

struct PositionalLight
{	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 position;
};
struct Material
{	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;
uniform float time;

// Ocean-specific uniforms
uniform vec3 oceanDeepColor;
uniform vec3 oceanShallowColor;
uniform float fresnelPower;

// Depth and translucency uniforms
uniform float waterMaxDepth;        // Maximum depth for color calculations
uniform float edgeFoamDistance;     // Distance for edge foam detection
uniform float waterClarityDepth;    // Depth at which water becomes fully opaque

// Enhanced procedural water texture from the book (Chapter on procedural texturing)
float noise2D(vec2 p)
{
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 6; i++)  // More octaves for detail
    {
        value += amplitude * noise2D(p);
        p *= 2.07;  // Slightly irregular scaling for more natural look
        amplitude *= 0.5;
    }
    return value;
}

// Enhanced water normal perturbation for realistic surface detail
vec3 getNormalPerturbation(vec2 coords)
{
    vec2 offset = vec2(0.01, 0.0);
    float heightL = fbm(coords - offset.xy);
    float heightR = fbm(coords + offset.xy);
    float heightD = fbm(coords - offset.yx);
    float heightU = fbm(coords + offset.yx);
    
    vec3 normal = vec3(heightL - heightR, heightD - heightU, 0.2);
    return normalize(normal);
}

void main(void)
{
    // Normalize the varying vectors
    vec3 L = normalize(varyingLightDir);
    vec3 N = normalize(varyingNormal);
    vec3 V = normalize(-varyingVertPos);
    
    // Calculate screen coordinates for depth sampling
    vec2 screenCoord = (screenPos.xy / screenPos.w) * 0.5 + 0.5;
    
    // Calculate depth-based parameters
    float depthFactor = clamp(waterDepth / waterClarityDepth, 0.0, 1.0);
    float shallowFactor = 1.0 - depthFactor;
    
    // Enhanced surface detail with properly scaled UV coordinates
    vec2 baseUV = tc * 2.0;  // Reduce stretching by scaling down
    vec2 waveCoords1 = baseUV + vec2(time * 0.15, time * 0.1);  // Slower animation
    vec2 waveCoords2 = baseUV * 1.5 + vec2(time * -0.05, time * 0.2);  // Different scale
    vec3 normalPerturbation = getNormalPerturbation(waveCoords1) + getNormalPerturbation(waveCoords2) * 0.5;
    N = normalize(N + normalPerturbation * 0.3);
    
    // Recalculate half vector with perturbed normal
    vec3 H = normalize(L + V);
    
    // Enhanced Fresnel effect with more realistic curve
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = pow(1.0 - NdotV, 1.5) * 0.8 + 0.2;
    
    // Depth-based water color system
    vec3 shallowTint = vec3(0.6, 0.9, 0.95);   // Very light turquoise for shallow areas
    vec3 mediumTint = vec3(0.3, 0.7, 0.9);     // Medium blue-green
    vec3 deepTint = vec3(0.05, 0.25, 0.5);     // Deep blue for depth
    vec3 reflectionTint = vec3(0.8, 0.9, 1.0); // Slightly blue-white reflection
    
    // Blend colors based on depth and viewing angle
    vec3 baseWaterColor = mix(shallowTint, mediumTint, clamp(depthFactor * 0.8, 0.0, 1.0));
    baseWaterColor = mix(baseWaterColor, deepTint, clamp(depthFactor * 1.5 - 0.5, 0.0, 1.0));
    vec3 waterColor = mix(baseWaterColor, deepTint, fresnel * 0.4);
    
    // Enhanced Phong reflection model
    vec3 ambient = (globalAmbient * material.ambient).xyz * 0.6;
    vec3 diffuse = light.diffuse.xyz * material.diffuse.xyz * max(dot(N, L), 0.0) * 0.8;
    
    // Enhanced specular with multiple highlight layers
    float specularPower1 = pow(max(dot(N, H), 0.0), material.shininess);
    float specularPower2 = pow(max(dot(N, H), 0.0), material.shininess * 0.3);
    vec3 specular = light.specular.xyz * material.specular.xyz * (specularPower1 * 0.8 + specularPower2 * 0.4);
    
    // Enhanced foam with properly scaled coordinates + edge foam
    vec2 foamCoords1 = baseUV * 3.0 + vec2(time * 0.2, time * 0.15);
    vec2 foamCoords2 = baseUV * 1.5 + vec2(time * -0.1, time * 0.25);
    float foam1 = fbm(foamCoords1);
    float foam2 = fbm(foamCoords2);
    float baseFoam = foam1 * 0.7 + foam2 * 0.3;
    baseFoam = smoothstep(0.55, 0.95, baseFoam);
    
    // Edge foam detection (simulate intersection with other geometry)
    // Create foam in shallow areas and at "edges" using depth gradient
    float edgeFoam = 0.0;
    if (waterDepth < edgeFoamDistance) {
        float edgeIntensity = 1.0 - (waterDepth / edgeFoamDistance);
        vec2 edgeCoords = baseUV * 8.0 + vec2(time * 0.3, time * 0.2);
        float edgeNoise = fbm(edgeCoords);
        edgeFoam = smoothstep(0.3, 0.8, edgeNoise) * edgeIntensity;
    }
    
    // Combine foams
    float foam = max(baseFoam, edgeFoam);
    
    // Caustics effect with reduced stretching
    vec2 causticsCoords = baseUV * 4.0 + vec2(time * 0.05, time * 0.025);
    float caustics = fbm(causticsCoords) * 0.3;
    caustics = smoothstep(0.3, 0.8, caustics);
    
    // Surface ripples with better scaling
    vec2 rippleCoords = baseUV * 5.0 + vec2(time * 0.4, time * 0.3);
    float ripples = fbm(rippleCoords) * 0.2;
    ripples = smoothstep(0.4, 0.7, ripples);
    
    // Combine all lighting components
    vec3 baseColor = (ambient + diffuse) * waterColor;
    vec3 finalColor = baseColor + specular * (1.0 + fresnel);
    
    // Add foam (white caps)
    finalColor = mix(finalColor, vec3(0.95, 0.98, 1.0), foam * 0.4);
    
    // Add caustics for underwater light effect
    finalColor += caustics * vec3(0.3, 0.5, 0.7) * (1.0 - foam);
    
    // Add surface ripples
    finalColor += ripples * vec3(0.2, 0.3, 0.4) * (1.0 - foam);
    
    // Add reflection tint based on Fresnel
    finalColor = mix(finalColor, reflectionTint, fresnel * 0.3);
    
    // Enhanced depth-based translucency system
    // Much more transparent shallow water, gradually becoming opaque
    float baseAlpha = 0.15 + depthFactor * 0.5;  // 0.15 to 0.65 based on depth
    float fresnelAlpha = fresnel * 0.25;         // Reduced fresnel opacity contribution
    float foamAlpha = foam * 0.3;                // Foam adds more opacity
    
    float alpha = baseAlpha + fresnelAlpha + foamAlpha;
    alpha = clamp(alpha, 0.05, 0.85);  // Allow much more transparency
    
    // Enhanced translucency in shallow areas - more dramatic effect
    if (waterDepth < 2.0) {
        float shallowFactor = waterDepth / 2.0;
        alpha *= 0.3 + 0.7 * shallowFactor;  // Make shallow areas much more transparent
    }
    
    // Add distance-based transparency for better depth perception
    float viewDistance = length(varyingVertPos);
    float distanceAlpha = clamp(viewDistance * 0.005, 0.0, 0.3);
    alpha += distanceAlpha;
    
    fragColor = vec4(finalColor, alpha);
}