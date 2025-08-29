#include <corecrt_math_defines.h>
#include "AdvancedMaterial.hpp"
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>

// AdvancedMaterial base implementation
Spectrum AdvancedMaterial::SampleTexture(const std::shared_ptr<Texture>& texture, const glm::vec2& uv) const {
    if (!texture) return Spectrum(1.0f);
    
    // Simple texture sampling - in a real implementation this would use proper filtering
    // For now, return a placeholder spectrum
    return Spectrum::FromRGB(glm::vec3(0.5f, 0.5f, 0.5f));
}

glm::vec3 AdvancedMaterial::SampleNormalMap(const std::shared_ptr<Texture>& normalMap, const glm::vec2& uv,
                                           const glm::vec3& normal, const glm::vec3& tangent) const {
    if (!normalMap) return normal;
    
    // Sample normal map and convert from [0,1] to [-1,1]
    glm::vec3 normalSample = glm::vec3(0.5f, 0.5f, 1.0f); // Placeholder
    normalSample = normalSample * 2.0f - 1.0f;
    
    // Construct tangent space basis
    glm::vec3 N = glm::normalize(normal);
    glm::vec3 T = glm::normalize(tangent - glm::dot(tangent, N) * N);
    glm::vec3 B = glm::cross(N, T);
    
    // Transform normal from tangent space to world space
    return glm::normalize(T * normalSample.x + B * normalSample.y + N * normalSample.z);
}

// DisneyMaterial implementation
DisneyMaterial::DisneyMaterial(const Spectrum& baseColor, float metallic, float roughness,
                              float specular, float specularTint, float sheen, float sheenTint,
                              float clearcoat, float clearcoatGloss, float subsurface,
                              float transmission, float ior)
    : baseColor(baseColor), metallic(metallic), roughness(roughness), specular(specular),
      specularTint(specularTint), sheen(sheen), sheenTint(sheenTint), clearcoat(clearcoat),
      clearcoatGloss(clearcoatGloss), subsurface(subsurface), transmission(transmission), ior(ior) {
}

void DisneyMaterial::ComputeScatteringFunctions(SurfaceInteraction* si) const {
    // Sample textures if available
    Spectrum albedo = baseColor;
    if (baseColorTexture) {
        albedo = albedo * SampleTexture(baseColorTexture, si->uv);
    }
    
    float metal = metallic;
    if (metallicTexture) {
        metal = SampleTexture(metallicTexture, si->uv).luminance();
    }
    
    float rough = roughness;
    if (roughnessTexture) {
        rough = SampleTexture(roughnessTexture, si->uv).luminance();
    }
    
    // Update surface normal if normal map exists
    if (normalTexture) {
        si->n = SampleNormalMap(normalTexture, si->uv, si->n, si->dpdu);
    }
    
    // Create Disney BRDF
    // This is a simplified version - full Disney BRDF is quite complex
    if (transmission > 0.0f) {
        // Glass-like material
        auto fresnel = std::make_unique<FresnelDielectric>(1.0f, ior);
        auto distribution = std::make_unique<TrowbridgeReitzDistribution>(rough, rough);
        si->bsdf = std::make_unique<MicrofacetTransmission>(albedo, std::move(distribution), 1.0f, ior);
    } else if (metal > 0.5f) {
        // Metallic material
        Spectrum eta = Spectrum::FromRGB(glm::vec3(0.2f, 0.9f, 1.5f)); // Simplified metal IOR
        Spectrum k = Spectrum::FromRGB(glm::vec3(3.1f, 2.3f, 1.9f));   // Simplified absorption
        auto fresnel = std::make_unique<FresnelConductor>(Spectrum(1.0f), eta, k);
        auto distribution = std::make_unique<TrowbridgeReitzDistribution>(rough, rough);
        si->bsdf = std::make_unique<MicrofacetReflection>(albedo, std::move(distribution), std::move(fresnel));
    } else {
        // Dielectric material
        si->bsdf = std::make_unique<LambertianReflection>(albedo);
    }
}

Spectrum DisneyMaterial::EvaluateDisneyBRDF(const glm::vec3& wo, const glm::vec3& wi, 
                                           const SurfaceInteraction& si) const {
    // Simplified Disney BRDF evaluation
    // Full implementation would include all Disney components
    
    Spectrum result(0.0f);
    
    // Diffuse component
    result += DisneyDiffuse(wo, wi, baseColor, roughness, subsurface);
    
    // Sheen component
    if (sheen > 0.0f) {
        result += DisneySheen(wo, wi, baseColor, sheen, sheenTint);
    }
    
    // Clearcoat component
    if (clearcoat > 0.0f) {
        result += DisneyClearcoat(wo, wi, clearcoat, clearcoatGloss);
    }
    
    return result;
}

Spectrum DisneyMaterial::DisneyDiffuse(const glm::vec3& wo, const glm::vec3& wi, 
                                      const Spectrum& baseColor, float roughness, float subsurface) const {
    float NdotL = std::abs(SpectralUtils::CosTheta(wi));
    float NdotV = std::abs(SpectralUtils::CosTheta(wo));
    
    // Simplified Disney diffuse
    float FL = std::pow(1.0f - NdotL, 5.0f);
    float FV = std::pow(1.0f - NdotV, 5.0f);
    float Fd90 = 0.5f + 2.0f * roughness * SpectralUtils::Cos2Theta(glm::normalize(wo + wi));
    float Fd = (1.0f + (Fd90 - 1.0f) * FL) * (1.0f + (Fd90 - 1.0f) * FV);
    
    // Subsurface component
    if (subsurface > 0.0f) {
        float Fss90 = roughness * SpectralUtils::Cos2Theta(glm::normalize(wo + wi));
        float Fss = (1.0f + (Fss90 - 1.0f) * FL) * (1.0f + (Fss90 - 1.0f) * FV);
        float ss = 1.25f * (Fss * (1.0f / (NdotL + NdotV) - 0.5f) + 0.5f);
        Fd = Fd + (ss - Fd) * subsurface;
    }
    
    return baseColor * (Fd / M_PI);
}

Spectrum DisneyMaterial::DisneySheen(const glm::vec3& wo, const glm::vec3& wi, 
                                    const Spectrum& baseColor, float sheen, float sheenTint) const {
    glm::vec3 H = glm::normalize(wo + wi);
    float cosTheta = glm::dot(wi, H);
    
    Spectrum Ctint = baseColor.luminance() > 0 ? baseColor / baseColor.luminance() : Spectrum(1.0f);
    Spectrum Csheen = Spectrum(1.0f) + (Ctint - Spectrum(1.0f)) * sheenTint;
    
    return Csheen * sheen * std::pow(1.0f - cosTheta, 5.0f);
}

Spectrum DisneyMaterial::DisneyClearcoat(const glm::vec3& wo, const glm::vec3& wi, 
                                        float clearcoat, float clearcoatGloss) const {
    glm::vec3 H = glm::normalize(wo + wi);
    float cosTheta = std::abs(glm::dot(wi, H));
    
    float alpha = 0.1f + (0.001f - 0.1f) * clearcoatGloss;
    float Dr = (alpha * alpha - 1.0f) / (M_PI * std::log(alpha * alpha) * 
               (1.0f + (alpha * alpha - 1.0f) * cosTheta * cosTheta));
    
    float FH = std::pow(1.0f - cosTheta, 5.0f);
    float Fr = 0.04f + (1.0f - 0.04f) * FH;
    
    return Spectrum(clearcoat * 0.25f * Dr * Fr);
}

// GlassMaterial implementation
GlassMaterial::GlassMaterial(const Spectrum& kr, const Spectrum& kt, float eta, bool dispersive)
    : Kr(kr), Kt(kt), eta(eta), dispersive(dispersive) {
}

void GlassMaterial::ComputeScatteringFunctions(SurfaceInteraction* si) const {
    float currentEta = eta;
    if (dispersive) {
        // Use wavelength-dependent IOR for dispersion
        currentEta = GetIORForWavelength(550.0f); // Use green wavelength as base
    }
    
    auto fresnel = std::make_unique<FresnelDielectric>(1.0f, currentEta);
    auto distribution = std::make_unique<TrowbridgeReitzDistribution>(0.001f, 0.001f); // Very smooth
    
    // Create both reflection and transmission
    si->bsdf = std::make_unique<MicrofacetReflection>(Kr, std::move(distribution), std::move(fresnel));
}

float GlassMaterial::GetIORForWavelength(float lambda) const {
    // Simplified Sellmeier equation for glass dispersion
    // Crown glass parameters
    float B1 = 1.03961212f, B2 = 0.231792344f, B3 = 1.01046945f;
    float C1 = 6.00069867e-3f, C2 = 2.00179144e-2f, C3 = 1.03560653e2f;
    
    lambda *= 1e-3f; // Convert nm to micrometers
    float lambda2 = lambda * lambda;
    
    float n2 = 1.0f + (B1 * lambda2) / (lambda2 - C1) +
                     (B2 * lambda2) / (lambda2 - C2) +
                     (B3 * lambda2) / (lambda2 - C3);
    
    return std::sqrt(n2);
}

// MetalMaterial implementation
MetalMaterial::MetalMaterial(const Spectrum& eta, const Spectrum& k, float roughness)
    : eta(eta), k(k), roughness(roughness) {
}

void MetalMaterial::ComputeScatteringFunctions(SurfaceInteraction* si) const {
    auto fresnel = std::make_unique<FresnelConductor>(Spectrum(1.0f), eta, k);
    auto distribution = std::make_unique<TrowbridgeReitzDistribution>(roughness, roughness);
    
    si->bsdf = std::make_unique<MicrofacetReflection>(Spectrum(1.0f), std::move(distribution), std::move(fresnel));
}

MetalMaterial* MetalMaterial::CreateGold(float roughness) {
    // Gold optical constants (simplified)
    Spectrum eta = Spectrum::FromRGB(glm::vec3(0.1431f, 0.3749f, 1.4424f));
    Spectrum k = Spectrum::FromRGB(glm::vec3(3.9831f, 2.3856f, 1.6038f));
    return new MetalMaterial(eta, k, roughness);
}

MetalMaterial* MetalMaterial::CreateSilver(float roughness) {
    // Silver optical constants (simplified)
    Spectrum eta = Spectrum::FromRGB(glm::vec3(0.1552f, 0.1167f, 0.1383f));
    Spectrum k = Spectrum::FromRGB(glm::vec3(4.8250f, 3.1221f, 2.1456f));
    return new MetalMaterial(eta, k, roughness);
}

MetalMaterial* MetalMaterial::CreateCopper(float roughness) {
    // Copper optical constants (simplified)
    Spectrum eta = Spectrum::FromRGB(glm::vec3(0.2004f, 0.9240f, 1.1022f));
    Spectrum k = Spectrum::FromRGB(glm::vec3(3.9129f, 2.4528f, 2.1421f));
    return new MetalMaterial(eta, k, roughness);
}

MetalMaterial* MetalMaterial::CreateAluminum(float roughness) {
    // Aluminum optical constants (simplified)
    Spectrum eta = Spectrum::FromRGB(glm::vec3(1.3456f, 0.9648f, 0.6177f));
    Spectrum k = Spectrum::FromRGB(glm::vec3(7.4746f, 6.3995f, 5.3031f));
    return new MetalMaterial(eta, k, roughness);
}

// SubsurfaceMaterial implementation
SubsurfaceMaterial::SubsurfaceMaterial(const Spectrum& kr, const Spectrum& kt, 
                                      const Spectrum& sigma_a, const Spectrum& sigma_s,
                                      float g, float eta, float scale)
    : Kr(kr), Kt(kt), sigma_a(sigma_a), sigma_s(sigma_s), g(g), eta(eta), scale(scale) {
    
    bssrdf = std::make_unique<SubsurfaceScattering>(sigma_a * scale, sigma_s * scale, g);
}

void SubsurfaceMaterial::ComputeScatteringFunctions(SurfaceInteraction* si) const {
    // Create surface BSDF
    auto fresnel = std::make_unique<FresnelDielectric>(1.0f, eta);
    auto distribution = std::make_unique<TrowbridgeReitzDistribution>(0.001f, 0.001f);
    
    si->bsdf = std::make_unique<MicrofacetReflection>(Kr, std::move(distribution), std::move(fresnel));
    
    // Note: Full BSSRDF integration would require additional scene integration
}

SubsurfaceMaterial* SubsurfaceMaterial::CreateSkin() {
    // Skin scattering parameters (simplified)
    Spectrum sigma_a = Spectrum::FromRGB(glm::vec3(0.0017f, 0.0025f, 0.0061f));
    Spectrum sigma_s = Spectrum::FromRGB(glm::vec3(2.55f, 3.21f, 3.77f));
    return new SubsurfaceMaterial(Spectrum(0.9f), Spectrum(0.0f), sigma_a, sigma_s, 0.0f, 1.33f, 1.0f);
}

SubsurfaceMaterial* SubsurfaceMaterial::CreateMilk() {
    // Milk scattering parameters
    Spectrum sigma_a = Spectrum::FromRGB(glm::vec3(0.0011f, 0.0024f, 0.014f));
    Spectrum sigma_s = Spectrum::FromRGB(glm::vec3(2.55f, 3.21f, 3.77f));
    return new SubsurfaceMaterial(Spectrum(0.9f), Spectrum(0.0f), sigma_a, sigma_s, 0.0f, 1.33f, 1.0f);
}

// EmissiveMaterial implementation
EmissiveMaterial::EmissiveMaterial(const Spectrum& Le, float power, bool twoSided)
    : emission(Le), power(power), twoSided(twoSided) {
}

void EmissiveMaterial::ComputeScatteringFunctions(SurfaceInteraction* si) const {
    // Emissive materials typically don't scatter light
    si->bsdf = nullptr;
}

Spectrum EmissiveMaterial::Le(const glm::vec3& wo) const {
    return emission * power;
}

EmissiveMaterial* EmissiveMaterial::CreateBlackbody(float temperature, float power) {
    Spectrum emission = Spectrum::FromBlackbody(temperature);
    return new EmissiveMaterial(emission, power);
}

EmissiveMaterial* EmissiveMaterial::CreateSun() {
    return CreateBlackbody(5778.0f, 1.0f);
}

EmissiveMaterial* EmissiveMaterial::CreateIncandescent() {
    return CreateBlackbody(2700.0f, 1.0f);
}

// MaterialFactory implementation
std::shared_ptr<AdvancedMaterial> MaterialFactory::CreatePlastic(const Spectrum& color, float roughness) {
    return std::make_shared<DisneyMaterial>(color, 0.0f, roughness, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.5f);
}

std::shared_ptr<AdvancedMaterial> MaterialFactory::CreateRubber(const Spectrum& color, float roughness) {
    return std::make_shared<DisneyMaterial>(color, 0.0f, roughness, 0.0f, 0.0f, 0.3f, 0.5f, 0.0f, 1.0f, 0.1f, 0.0f, 1.5f);
}

std::shared_ptr<AdvancedMaterial> MaterialFactory::CreateWater(float roughness) {
    Spectrum color = Spectrum::FromRGB(glm::vec3(0.8f, 0.9f, 1.0f));
    return std::make_shared<GlassMaterial>(Spectrum(0.02f), Spectrum(0.98f), 1.33f, false);
}

std::shared_ptr<AdvancedMaterial> MaterialFactory::CreateSkin(const std::string& type) {
    return std::shared_ptr<AdvancedMaterial>(SubsurfaceMaterial::CreateSkin());
}

// CheckerboardTexture implementation
CheckerboardTexture::CheckerboardTexture(const Spectrum& color1, const Spectrum& color2, float scale)
    : color1(color1), color2(color2), scale(scale) {
}

Spectrum CheckerboardTexture::Evaluate(const glm::vec2& uv) const {
    glm::vec2 scaledUV = uv * scale;
    int checkX = static_cast<int>(std::floor(scaledUV.x));
    int checkY = static_cast<int>(std::floor(scaledUV.y));
    
    bool isEven = (checkX + checkY) % 2 == 0;
    return isEven ? color1 : color2;
}

// HDRTexture implementation
HDRTexture::HDRTexture(const std::string& filename) : width(0), height(0) {
    LoadHDR(filename);
    ComputeLuminanceDistribution();
}

Spectrum HDRTexture::Evaluate(const glm::vec2& uv) const {
    if (pixels.empty()) return Spectrum(0.0f);
    
    // Clamp UV coordinates
    glm::vec2 clampedUV = glm::clamp(uv, glm::vec2(0.0f), glm::vec2(1.0f));
    
    // Convert to pixel coordinates
    int x = static_cast<int>(clampedUV.x * static_cast<float>(width - 1));
    int y = static_cast<int>(clampedUV.y * static_cast<float>(height - 1));
    
    // Clamp to valid range
    x = std::max(0, std::min(x, width - 1));
    y = std::max(0, std::min(y, height - 1));
    
    return pixels[y * width + x];
}

void HDRTexture::LoadHDR(const std::string& filename) {
    // Simplified HDR loading - real implementation would use proper HDR format
    std::cout << "Loading HDR texture: " << filename << " (placeholder implementation)" << std::endl;
    
    // Create a simple procedural HDR environment for testing
    width = 256;
    height = 128;
    pixels.resize(width * height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / (width - 1);
            float v = static_cast<float>(y) / (height - 1);
            
            // Create a simple sky gradient
            float skyIntensity = std::pow(1.0f - v, 2.0f) * 3.0f; // Brighter at horizon
            Spectrum skyColor = Spectrum::FromRGB(glm::vec3(0.5f, 0.7f, 1.0f)) * skyIntensity;
            
            // Add a bright sun
            float sunU = 0.75f, sunV = 0.8f;
            float sunDist = std::sqrt((u - sunU) * (u - sunU) + (v - sunV) * (v - sunV));
            if (sunDist < 0.05f) {
                skyColor = skyColor + Spectrum::FromRGB(glm::vec3(10.0f, 8.0f, 6.0f));
            }
            
            pixels[y * width + x] = skyColor;
        }
    }
}

void HDRTexture::ComputeLuminanceDistribution() {
    luminances.resize(width * height);
    
    for (int i = 0; i < width * height; ++i) {
        luminances[i] = pixels[i].luminance();
    }
}