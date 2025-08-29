#version 420 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec4 FragPosLightSpace; // Position in light space for shadow mapping

out vec4 FragColor;

// Material uniforms
uniform vec3 material_albedo;
uniform float material_metallic;
uniform float material_roughness;
uniform float material_ao;

// Textures
uniform sampler2D material_diffuseTexture;
uniform sampler2D material_normalTexture;
uniform sampler2D material_specularTexture;
uniform bool material_hasDiffuseTexture;
uniform bool material_hasNormalTexture;
uniform bool material_hasSpecularTexture;

// Shadow mapping
uniform sampler2D shadowMap;
uniform samplerCube pointShadowMap;
uniform bool hasShadowMap;
uniform bool hasPointShadowMap;

// Environment mapping for reflections
uniform samplerCube environmentMap;
uniform bool hasEnvironmentMap;

// Camera
uniform vec3 viewPos;

// Light structures
struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float innerCone;
    float outerCone;
    float constant;
    float linear;
    float quadratic;
};

// Light arrays and counts
uniform DirectionalLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;

uniform int numDirLights;
uniform int numPointLights;
uniform int numSpotLights;

// Point light shadow mapping
uniform vec3 pointLightPos;
uniform float pointShadowFarPlane;

const float PI = 3.14159265359;

// PBR functions
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Shadow calculation for directional lights
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) {
    if (!hasShadowMap) return 0.0;
    
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Calculate bias (to prevent shadow acne)
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (Percentage Closer Filtering)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
    
    return shadow;
}

// Point light shadow calculation
float PointShadowCalculation(vec3 fragPos, vec3 lightPos) {
    if (!hasPointShadowMap) return 0.0;
    
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    
    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / pointShadowFarPlane)) / 25.0;
    
    // Sample directions for PCF
    vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );
    
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(pointShadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= pointShadowFarPlane; // undo mapping [0,1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    
    return shadow;
}

// Calculate lighting contribution for directional light
vec3 CalcDirLight(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 lightDir = normalize(-light.direction);
    vec3 halfwayDir = normalize(viewDir + lightDir);
    
    // BRDF
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    // Calculate shadows
    float shadow = ShadowCalculation(FragPosLightSpace, normal, lightDir);
    
    return (1.0 - shadow) * (kD * albedo / PI + specular) * light.color * light.intensity * NdotL;
}

// Calculate lighting contribution for point light
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 lightDir = normalize(light.position - fragPos);
    vec3 halfwayDir = normalize(viewDir + lightDir);
    
    // Attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // BRDF
    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    // Calculate shadows
    float shadow = PointShadowCalculation(fragPos, light.position);
    
    return (1.0 - shadow) * (kD * albedo / PI + specular) * light.color * light.intensity * attenuation * NdotL;
}

void main() {
    // Sample albedo texture
    vec3 albedo = material_albedo;
    if(material_hasDiffuseTexture) {
        albedo = texture(material_diffuseTexture, TexCoords).rgb;
        albedo = pow(albedo, vec3(2.2)); // Convert from sRGB to linear
    }
    
    // Material properties
    float metallic = material_metallic;
    float roughness = material_roughness;
    float ao = material_ao;
    
    // Override with texture values if available
    if(material_hasSpecularTexture) {
        vec3 specularSample = texture(material_specularTexture, TexCoords).rgb;
        roughness = 1.0 - specularSample.g; // Convert glossiness to roughness
        metallic = specularSample.b;
    }
    
    // Ensure valid ranges
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - FragPos);
    vec3 R = reflect(-V, N); // Reflection vector for environment mapping
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Calculate lighting
    vec3 Lo = vec3(0.0);
    
    // Directional lights
    if (numDirLights > 0) {
        Lo += CalcDirLight(dirLight, N, V, albedo, metallic, roughness, F0);
    }
    
    // Point lights
    if (numPointLights > 0) {
        Lo += CalcPointLight(pointLight, N, FragPos, V, albedo, metallic, roughness, F0);
    }
    
    // Ambient lighting with image-based lighting (IBL)
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    // Add environment reflections for metallic surfaces
    if (hasEnvironmentMap && metallic > 0.1) {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 envColor = texture(environmentMap, R).rgb;
        
        // Simple approximation for environment lighting
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;
        
        vec3 irradiance = texture(environmentMap, N).rgb; // Very simplified irradiance
        vec3 diffuse = irradiance * albedo;
        
        // Combine environment reflections
        ambient = (kD * diffuse + envColor * metallic) * ao;
    }
    
    vec3 color = ambient + Lo;
    
    // HDR tonemapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}