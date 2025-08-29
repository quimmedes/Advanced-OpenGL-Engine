#version 420 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec3 Tangent;
in vec3 Bitangent;
out vec4 FragColor;

// Basic material uniforms
uniform vec3 material_albedo;
uniform float material_metallic;
uniform float material_roughness;
uniform float material_ao;

// Textures
uniform sampler2D material_diffuseTexture;
uniform sampler2D material_normalTexture;
uniform sampler2D material_specularTexture;
uniform sampler2D material_occlusionTexture;
uniform bool material_hasDiffuseTexture;
uniform bool material_hasNormalTexture;
uniform bool material_hasSpecularTexture;
uniform bool material_hasOcclusionTexture;

// Advanced material system uniforms
uniform int u_materialType;
uniform bool u_hasAdvancedMaterial;

// Disney BRDF uniforms
uniform float u_disney_subsurface;
uniform float u_disney_sheen;
uniform float u_disney_sheenTint;
uniform float u_disney_clearcoat;
uniform float u_disney_clearcoatGloss;
uniform float u_disney_specularTint;
uniform float u_disney_transmission;
uniform float u_disney_ior;

// Metal material uniforms
uniform vec3 u_metal_eta;
uniform vec3 u_metal_k;

// Glass material uniforms
uniform float u_glass_ior;
uniform float u_glass_transmission;

// Subsurface material uniforms
uniform vec3 u_subsurface_sigmaA;
uniform vec3 u_subsurface_sigmaS;
uniform float u_subsurface_scale;

// Emissive material uniforms
uniform vec3 u_emission_color;
uniform float u_emission_power;

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

const float PI = 3.14159265359;

// Material type constants
#define MATERIAL_PBR_BASIC 0
#define MATERIAL_PBR_ADVANCED 1
#define MATERIAL_DISNEY_BRDF 2
#define MATERIAL_METAL_MATERIAL 3
#define MATERIAL_GLASS_MATERIAL 4
#define MATERIAL_SUBSURFACE_MATERIAL 5
#define MATERIAL_EMISSIVE_MATERIAL 6

// Advanced PBR functions
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

// Fresnel for conductors (metals)
vec3 fresnelConductor(float cosTheta, vec3 eta, vec3 k) {
    vec3 etak = eta + k;
    vec3 eta2 = eta * eta;
    vec3 etak2 = etak * etak;
    
    vec3 t0 = eta2 - k*k + cosTheta*cosTheta;
    vec3 a2plusb2 = sqrt(t0*t0 + 4.0*eta2*k*k);
    vec3 t1 = a2plusb2 + cosTheta*cosTheta;
    vec3 a = sqrt(0.5 * (a2plusb2 + t0));
    vec3 t2 = 2.0*a*cosTheta;
    vec3 Rs = (t1 - t2) / (t1 + t2);
    
    vec3 t3 = cosTheta*cosTheta*a2plusb2 + 1.0;
    vec3 t4 = t2 * 1.0;
    vec3 Rp = Rs * (t3 - t4) / (t3 + t4);
    
    return 0.5 * (Rp + Rs);
}

// Disney BRDF components
vec3 DisneyDiffuse(vec3 V, vec3 L, vec3 H, vec3 albedo, float roughness, float subsurface) {
    float NdotL = max(dot(vec3(0,0,1), L), 0.0);
    float NdotV = max(dot(vec3(0,0,1), V), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    
    float FL = pow(1.0 - NdotL, 5.0);
    float FV = pow(1.0 - NdotV, 5.0);
    float Fd90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);
    
    // Fake subsurface scattering
    if (subsurface > 0.0) {
        float Fss90 = roughness * LdotH * LdotH;
        float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
        float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);
        Fd = mix(Fd, ss, subsurface);
    }
    
    return albedo * Fd / PI;
}

vec3 DisneySheen(vec3 V, vec3 L, vec3 H, vec3 baseColor, float sheen, float sheenTint) {
    float LdotH = max(dot(L, H), 0.0);
    float FH = pow(1.0 - LdotH, 5.0);
    
    vec3 Ctint = length(baseColor) > 0.0 ? baseColor / length(baseColor) : vec3(1.0);
    vec3 Csheen = mix(vec3(1.0), Ctint, sheenTint);
    
    return Csheen * sheen * FH;
}

vec3 DisneyClearcoat(vec3 V, vec3 L, vec3 H, float clearcoat, float clearcoatGloss) {
    float LdotH = max(dot(L, H), 0.0);
    float alpha = mix(0.1, 0.001, clearcoatGloss);
    float FH = pow(1.0 - LdotH, 5.0);
    float Fr = mix(0.04, 1.0, FH);
    
    // GTR1 distribution for clearcoat
    float Dr = (alpha*alpha - 1.0) / (PI * log(alpha*alpha) * (1.0 + (alpha*alpha - 1.0) * LdotH*LdotH));
    
    return vec3(clearcoat * 0.25 * Dr * Fr);
}

// Simple subsurface scattering approximation
vec3 SubsurfaceScattering(vec3 albedo, vec3 sigmaS, vec3 sigmaA, float scale) {
    // Simplified subsurface term using diffusion approximation
    vec3 sigmaT = sigmaS + sigmaA;
    vec3 alphaPrime = sigmaS / sigmaT;
    vec3 sigmaTr = sqrt(3.0 * sigmaA * sigmaT) * scale;
    
    // Approximate multiple scattering contribution
    return albedo * alphaPrime * exp(-sigmaTr);
}

// Calculate lighting for different material types
vec3 CalculateLighting(vec3 lightDir, vec3 lightColor, float lightIntensity, float attenuation,
                      vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0) {
    
    vec3 H = normalize(V + lightDir);
    float NdotL = max(dot(N, lightDir), 0.0);
    
    if (NdotL <= 0.0) return vec3(0.0);
    
    if (u_hasAdvancedMaterial && u_materialType == MATERIAL_DISNEY_BRDF) {
        // Disney BRDF
        vec3 diffuse = DisneyDiffuse(V, lightDir, H, albedo, roughness, u_disney_subsurface);
        
        // Standard specular
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, lightDir, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        
        vec3 result = (kD * diffuse + specular) * lightColor * lightIntensity * attenuation * NdotL;
        
        // Add Disney components
        if (u_disney_sheen > 0.0) {
            result += DisneySheen(V, lightDir, H, albedo, u_disney_sheen, u_disney_sheenTint) * lightColor * lightIntensity * attenuation * NdotL;
        }
        
        if (u_disney_clearcoat > 0.0) {
            result += DisneyClearcoat(V, lightDir, H, u_disney_clearcoat, u_disney_clearcoatGloss) * lightColor * lightIntensity * attenuation * NdotL;
        }
        
        return result;
        
    } else if (u_hasAdvancedMaterial && u_materialType == MATERIAL_METAL_MATERIAL) {
        // Metal BRDF with complex IOR
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, lightDir, roughness);
        vec3 F = fresnelConductor(max(dot(H, V), 0.0), u_metal_eta, u_metal_k);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        
        return specular * lightColor * lightIntensity * attenuation * NdotL;
        
    } else if (u_hasAdvancedMaterial && u_materialType == MATERIAL_SUBSURFACE_MATERIAL) {
        // Subsurface scattering material
        vec3 subsurface = SubsurfaceScattering(albedo, u_subsurface_sigmaS, u_subsurface_sigmaA, u_subsurface_scale);
        
        // Standard PBR for surface reflection
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, lightDir, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = (1.0 - kS) * (1.0 - metallic);
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        
        return (kD * (albedo / PI + subsurface) + specular) * lightColor * lightIntensity * attenuation * NdotL;
        
    } else {
        // Standard PBR
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, lightDir, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
        vec3 specular = numerator / denominator;
        
        return (kD * albedo / PI + specular) * lightColor * lightIntensity * attenuation * NdotL;
    }
}

vec3 GetNormal() {
    vec3 normal = normalize(Normal);
    
    if (material_hasNormalTexture) {
        // Sample normal map and convert from [0,1] to [-1,1]
        vec3 normalMap = texture(material_normalTexture, TexCoords).rgb * 2.0 - 1.0;
        
        // Create TBN matrix
        vec3 N = normal;
        vec3 T = normalize(Tangent);
        vec3 B = normalize(Bitangent);
        mat3 TBN = mat3(T, B, N);
        
        return normalize(TBN * normalMap);
    }
    
    return normal;
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
    
    // Apply occlusion texture if available
    if(material_hasOcclusionTexture) {
        float occlusionSample = texture(material_occlusionTexture, TexCoords).r;
        ao *= occlusionSample; // Multiply with base AO value
    }
    
    // Ensure valid ranges
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    ao = clamp(ao, 0.0, 1.0);
    
    vec3 N = GetNormal();
    vec3 V = normalize(viewPos - FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Handle emissive materials
    vec3 emission = vec3(0.0);
    if (u_hasAdvancedMaterial && u_materialType == MATERIAL_EMISSIVE_MATERIAL) {
        emission = u_emission_color * u_emission_power;
    }
    
    // Calculate lighting
    vec3 Lo = vec3(0.0);
    
    // Directional lights (SUN)
    if (numDirLights > 0) {
        vec3 lightDir = normalize(-dirLight.direction);
        // Boost sun contribution for bright daytime look
        vec3 sunContribution = CalculateLighting(lightDir, dirLight.color, dirLight.intensity, 1.0, N, V, albedo, metallic, roughness, F0);
        Lo += sunContribution * 1.2; // Extra boost for sun
    }
    
    // Point lights
    if (numPointLights > 0) {
        vec3 lightDir = normalize(pointLight.position - FragPos);
        float distance = length(pointLight.position - FragPos);
        float attenuation = 1.0 / (pointLight.constant + pointLight.linear * distance + pointLight.quadratic * (distance * distance));
        
        Lo += CalculateLighting(lightDir, pointLight.color, pointLight.intensity, attenuation, N, V, albedo, metallic, roughness, F0);
    }
    
    // Spot lights
    if (numSpotLights > 0) {
        vec3 lightDir = normalize(spotLight.position - FragPos);
        float distance = length(spotLight.position - FragPos);
        float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));
        
        // Spotlight intensity
        float theta = dot(lightDir, normalize(-spotLight.direction));
        float epsilon = spotLight.innerCone - spotLight.outerCone;
        float intensity = clamp((theta - spotLight.outerCone) / epsilon, 0.0, 1.0);
        
        Lo += CalculateLighting(lightDir, spotLight.color, spotLight.intensity, attenuation * intensity, N, V, albedo, metallic, roughness, F0);
    }
    
    // BRIGHT DAYTIME AMBIENT - simulating scattered skylight
    vec3 ambient = vec3(0.4) * albedo * ao; // Very bright ambient for daytime
    if (u_hasAdvancedMaterial && u_materialType == MATERIAL_SUBSURFACE_MATERIAL) {
        // Boost ambient for subsurface materials
        ambient *= 2.0;
    }
    
    vec3 color = ambient + Lo + emission;
    
    // OUTDOOR DAYTIME EXPOSURE - much brighter
    color *= 2.5;
    
    // Softer tonemapping for bright outdoor scenes
    color = color / (color + vec3(0.5));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    FragColor = vec4(color, 1.0);
}