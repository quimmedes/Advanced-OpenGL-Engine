#include <corecrt_math_defines.h>
#include "Spectrum.hpp"
#include <cmath>
#include <random>
#include <numeric>

// CIE color matching functions (sampled at 5nm intervals from 400-700nm)
const std::array<float, SPECTRAL_SAMPLES> Spectrum::CIE_X = {
    0.0143f, 0.0435f, 0.1344f, 0.2839f, 0.3483f, 0.3362f, 0.2908f, 0.1954f, 0.0956f, 0.0320f,
    0.0049f, 0.0093f, 0.0633f, 0.1655f, 0.2904f, 0.4334f, 0.5945f, 0.7621f, 0.9163f, 1.0263f,
    1.0622f, 1.0026f, 0.8544f, 0.6424f, 0.4479f, 0.2835f, 0.1649f, 0.0874f, 0.0468f, 0.0227f,
    0.0114f, 0.0058f, 0.0029f, 0.0014f, 0.0007f, 0.0003f, 0.0002f, 0.0001f, 0.0000f, 0.0000f,
    0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f,
    0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f
};

const std::array<float, SPECTRAL_SAMPLES> Spectrum::CIE_Y = {
    0.0004f, 0.0012f, 0.0040f, 0.0116f, 0.0230f, 0.0380f, 0.0600f, 0.0910f, 0.1390f, 0.2080f,
    0.3230f, 0.5030f, 0.7100f, 0.8620f, 0.9540f, 0.9950f, 0.9950f, 0.9520f, 0.8700f, 0.7570f,
    0.6310f, 0.5030f, 0.3810f, 0.2650f, 0.1750f, 0.1070f, 0.0610f, 0.0320f, 0.0170f, 0.0082f,
    0.0041f, 0.0021f, 0.0010f, 0.0005f, 0.0002f, 0.0001f, 0.0001f, 0.0000f, 0.0000f, 0.0000f,
    0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f,
    0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f
};

const std::array<float, SPECTRAL_SAMPLES> Spectrum::CIE_Z = {
    0.0679f, 0.2074f, 0.6456f, 1.3856f, 1.7471f, 1.7721f, 1.6692f, 1.2876f, 0.8130f, 0.4652f,
    0.2720f, 0.1582f, 0.0782f, 0.0422f, 0.0203f, 0.0087f, 0.0039f, 0.0021f, 0.0017f, 0.0011f,
    0.0008f, 0.0003f, 0.0002f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f,
    0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f,
    0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f,
    0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f, 0.0000f
};

// D65 standard illuminant
const std::array<float, SPECTRAL_SAMPLES> Spectrum::D65_ILLUMINANT = {
    82.75f, 87.12f, 91.49f, 92.46f, 93.43f, 90.06f, 86.68f, 95.77f, 104.86f, 110.94f,
    117.01f, 117.41f, 117.81f, 116.34f, 114.86f, 115.39f, 115.92f, 112.37f, 108.81f, 109.08f,
    109.35f, 108.58f, 107.80f, 106.30f, 104.79f, 106.24f, 107.69f, 106.05f, 104.41f, 104.23f,
    104.05f, 102.02f, 100.00f, 98.17f, 96.33f, 96.06f, 95.79f, 92.24f, 88.69f, 89.35f,
    90.01f, 89.80f, 89.60f, 88.65f, 87.70f, 85.49f, 83.29f, 83.49f, 83.70f, 81.86f,
    80.03f, 80.12f, 80.21f, 81.25f, 82.28f, 80.28f, 78.28f, 74.00f, 69.72f, 70.67f
};

// Color conversion matrices
const glm::mat3 Spectrum::RGB_TO_XYZ = glm::mat3(
    0.4124564f, 0.3575761f, 0.1804375f,
    0.2126729f, 0.7151522f, 0.0721750f,
    0.0193339f, 0.1191920f, 0.9503041f
);

const glm::mat3 Spectrum::XYZ_TO_RGB = glm::mat3(
    3.2404542f, -1.5371385f, -0.4985314f,
    -0.9692660f,  1.8760108f,  0.0415560f,
    0.0556434f, -0.2040259f,  1.0572252f
);

// Spectrum implementation
Spectrum::Spectrum(float value) {
    samples.fill(value);
}

Spectrum::Spectrum(const std::array<float, SPECTRAL_SAMPLES>& s) : samples(s) {
}

Spectrum Spectrum::FromRGB(const glm::vec3& rgb) {
    // Convert RGB to spectrum using Smits' method (simplified)
    Spectrum result;
    
    // White point scaling
    float white = std::max({rgb.r, rgb.g, rgb.b});
    if (white > 0.0f) {
        glm::vec3 normalized = rgb / white;
        
        // Simple RGB to spectrum conversion (could be improved with proper basis functions)
        for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
            float lambda = indexToWavelength(i);
            float r_weight = std::exp(-std::pow((lambda - 700.0f) / 100.0f, 2.0f));
            float g_weight = std::exp(-std::pow((lambda - 546.1f) / 100.0f, 2.0f));
            float b_weight = std::exp(-std::pow((lambda - 435.8f) / 100.0f, 2.0f));
            
            result[i] = white * (normalized.r * r_weight + normalized.g * g_weight + normalized.b * b_weight);
        }
    }
    
    return result;
}

Spectrum Spectrum::FromBlackbody(float temperature) {
    Spectrum result;
    
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        float lambda = indexToWavelength(i) * 1e-9f; // Convert nm to m
        result[i] = SpectralUtils::PlanckianLocus(lambda, temperature);
    }
    
    return result;
}

glm::vec3 Spectrum::toXYZ() const {
    glm::vec3 xyz(0.0f);
    float dlambda = (LAMBDA_MAX - LAMBDA_MIN) / SPECTRAL_SAMPLES;
    
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        xyz.x += samples[i] * CIE_X[i] * dlambda;
        xyz.y += samples[i] * CIE_Y[i] * dlambda;
        xyz.z += samples[i] * CIE_Z[i] * dlambda;
    }
    
    return xyz;
}

glm::vec3 Spectrum::toRGB() const {
    glm::vec3 xyz = toXYZ();
    return XYZ_TO_RGB * xyz;
}

float Spectrum::luminance() const {
    float y = 0.0f;
    float dlambda = (LAMBDA_MAX - LAMBDA_MIN) / SPECTRAL_SAMPLES;
    
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        y += samples[i] * CIE_Y[i] * dlambda;
    }
    
    return y;
}

// Spectrum operators
Spectrum Spectrum::operator+(const Spectrum& other) const {
    Spectrum result;
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        result[i] = samples[i] + other[i];
    }
    return result;
}

Spectrum Spectrum::operator*(const Spectrum& other) const {
    Spectrum result;
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        result[i] = samples[i] * other[i];
    }
    return result;
}

Spectrum Spectrum::operator*(float scalar) const {
    Spectrum result;
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        result[i] = samples[i] * scalar;
    }
    return result;
}

Spectrum Spectrum::operator-(const Spectrum& other) const {
    Spectrum result;
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        result[i] = samples[i] - other[i];
    }
    return result;
}

Spectrum Spectrum::operator/(float scalar) const {
    Spectrum result;
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        result[i] = samples[i] / scalar;
    }
    return result;
}

Spectrum Spectrum::operator/(const Spectrum& other) const {
    Spectrum result;
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        result[i] = other[i] != 0.0f ? samples[i] / other[i] : 0.0f;
    }
    return result;
}

// Assignment operators
Spectrum& Spectrum::operator+=(const Spectrum& other) {
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        samples[i] += other[i];
    }
    return *this;
}

Spectrum& Spectrum::operator-=(const Spectrum& other) {
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        samples[i] -= other[i];
    }
    return *this;
}

Spectrum& Spectrum::operator*=(const Spectrum& other) {
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        samples[i] *= other[i];
    }
    return *this;
}

Spectrum& Spectrum::operator*=(float scalar) {
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        samples[i] *= scalar;
    }
    return *this;
}

Spectrum& Spectrum::operator/=(float scalar) {
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        samples[i] /= scalar;
    }
    return *this;
}

float Spectrum::integrate() const {
    float sum = 0.0f;
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        sum += samples[i];
    }
    return sum * (LAMBDA_MAX - LAMBDA_MIN) / SPECTRAL_SAMPLES;
}

bool Spectrum::isBlack() const {
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        if (samples[i] != 0.0f) return false;
    }
    return true;
}

void Spectrum::clamp(float min, float max) {
    for (int i = 0; i < SPECTRAL_SAMPLES; ++i) {
        if (samples[i] < min) samples[i] = min;
        else if (samples[i] > max) samples[i] = max;
    }
}

float Spectrum::indexToWavelength(int index) {
    return LAMBDA_MIN + (LAMBDA_MAX - LAMBDA_MIN) * index / (SPECTRAL_SAMPLES - 1);
}

// TrowbridgeReitzDistribution (GGX) implementation
TrowbridgeReitzDistribution::TrowbridgeReitzDistribution(float alphaX, float alphaY, bool sampleVis)
    : MicrofacetDistribution(sampleVis), alphaX(alphaX), alphaY(alphaY) {
}

float TrowbridgeReitzDistribution::D(const glm::vec3& wh) const {
    float tan2Theta = SpectralUtils::Tan2Theta(wh);
    if (std::isinf(tan2Theta)) return 0.0f;
    
    float cos4Theta = SpectralUtils::Cos2Theta(wh) * SpectralUtils::Cos2Theta(wh);
    float e = (SpectralUtils::Cos2Theta(wh) / (alphaX * alphaX) + 
               SpectralUtils::Sin2Theta(wh) / (alphaY * alphaY)) * tan2Theta;
    
    return 1.0f / (M_PI * alphaX * alphaY * cos4Theta * (1.0f + e) * (1.0f + e));
}

float TrowbridgeReitzDistribution::Lambda(const glm::vec3& w) const {
    float absTanTheta = std::abs(SpectralUtils::TanTheta(w));
    if (std::isinf(absTanTheta)) return 0.0f;
    
    float alpha = std::sqrt(SpectralUtils::Cos2Theta(w) * alphaX * alphaX + 
                           SpectralUtils::Sin2Theta(w) * alphaY * alphaY);
    float alpha2Tan2Theta = (alpha * absTanTheta) * (alpha * absTanTheta);
    
    return (-1.0f + std::sqrt(1.0f + alpha2Tan2Theta)) / 2.0f;
}

glm::vec3 TrowbridgeReitzDistribution::Sample_wh(const glm::vec3& wo, const glm::vec2& u) const {
    glm::vec3 wh;
    if (!sampleVisibleArea) {
        float cosTheta = 0, phi = (2 * M_PI) * u[1];
        if (alphaX == alphaY) {
            float tanTheta2 = alphaX * alphaX * u[0] / (1.0f - u[0]);
            cosTheta = 1 / std::sqrt(1 + tanTheta2);
        }
        // More complex sampling for anisotropic case would go here
        
        float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        wh = SpectralUtils::SphericalDirection(sinTheta, cosTheta, phi);
    } else {
        // Visible normal sampling (more advanced)
        // Simplified implementation
        float sinTheta = std::sqrt(u[0]);
        float cosTheta = std::sqrt(1.0f - u[0]);
        float phi = 2 * M_PI * u[1];
        wh = SpectralUtils::SphericalDirection(sinTheta, cosTheta, phi);
    }
    
    if (!SpectralUtils::SameHemisphere(wo, wh)) wh = -wh;
    return wh;
}

// Fresnel implementations
FresnelDielectric::FresnelDielectric(float etaI, float etaT) : etaI(etaI), etaT(etaT) {
}

FresnelConductor::FresnelConductor(const Spectrum& etaI, const Spectrum& etaT, const Spectrum& k)
    : etaI(etaI), etaT(etaT), k(k) {}

Spectrum FresnelDielectric::Evaluate(float cosThetaI) const {
    cosThetaI = glm::clamp(cosThetaI, -1.0f, 1.0f);
    
    bool entering = cosThetaI > 0.0f;
    float etaI_local = entering ? etaI : etaT;
    float etaT_local = entering ? etaT : etaI;
    
    float sinThetaI = std::sqrt(std::max(0.0f, 1 - cosThetaI * cosThetaI));
    float sinThetaT = etaI_local / etaT_local * sinThetaI;
    
    if (sinThetaT >= 1) return Spectrum(1.0f); // Total internal reflection
    
    float cosThetaT = std::sqrt(std::max(0.0f, 1 - sinThetaT * sinThetaT));
    
    float Rparl = ((etaT_local * cosThetaI) - (etaI_local * cosThetaT)) /
                  ((etaT_local * cosThetaI) + (etaI_local * cosThetaT));
    float Rperp = ((etaI_local * cosThetaI) - (etaT_local * cosThetaT)) /
                  ((etaI_local * cosThetaI) + (etaT_local * cosThetaT));
    
    return Spectrum((Rparl * Rparl + Rperp * Rperp) / 2.0f);
}

// MicrofacetReflection constructor implementation
MicrofacetReflection::MicrofacetReflection(const Spectrum& R,
    std::unique_ptr<MicrofacetDistribution> distribution,
    std::unique_ptr<Fresnel> fresnel)
    : R(R), distribution(std::move(distribution)), fresnel(std::move(fresnel)) {}

// MicrofacetTransmission constructor implementation
MicrofacetTransmission::MicrofacetTransmission(const Spectrum& T,
    std::unique_ptr<MicrofacetDistribution> distribution,
    float etaA, float etaB)
    : T(T), distribution(std::move(distribution)), etaA(etaA), etaB(etaB) {}

// FresnelConductor::Evaluate implementation
Spectrum FresnelConductor::Evaluate(float cosThetaI) const {
    // Schlick's approximation for conductor
    // This is a simplified version, for full accuracy use complex IOR equations
    float clampedCosThetaI = glm::clamp(cosThetaI, -1.0f, 1.0f);
    Spectrum eta = etaT / etaI;
    Spectrum etaK = k / etaI;
    float cosThetaI2 = clampedCosThetaI * clampedCosThetaI;
    Spectrum tmp = (eta * eta + etaK * etaK);
    Spectrum rParl2 = (tmp - 2.0f * eta * clampedCosThetaI + cosThetaI2) /
                      (tmp + 2.0f * eta * clampedCosThetaI + cosThetaI2);
    Spectrum rPerp2 = (tmp * cosThetaI2 - 2.0f * eta * clampedCosThetaI + 1.0f) /
                      (tmp * cosThetaI2 + 2.0f * eta * clampedCosThetaI + 1.0f);
    return (rParl2 + rPerp2) * 0.5f;
}

// LambertianReflection constructor (já implementado acima, mas duplicado para garantir)
LambertianReflection::LambertianReflection(const Spectrum& R) : R(R) {}

// MicrofacetReflection methods (stubs)
Spectrum MicrofacetReflection::f(const glm::vec3& wo, const glm::vec3& wi) const {
    // Implementação simplificada: retorna R
    return R;
}
Spectrum MicrofacetReflection::Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const {
    // Implementação simplificada: amostra uniforme
    *wi = MonteCarlo::CosineSampleHemisphere(u);
    *pdf = MonteCarlo::CosineHemispherePdf(SpectralUtils::CosTheta(*wi));
    return f(wo, *wi);
}
float MicrofacetReflection::Pdf(const glm::vec3& wo, const glm::vec3& wi) const {
    // Implementação simplificada: PDF padrão
    return MonteCarlo::CosineHemispherePdf(SpectralUtils::CosTheta(wi));
}

// MicrofacetTransmission methods (stubs)
Spectrum MicrofacetTransmission::f(const glm::vec3& wo, const glm::vec3& wi) const {
    // Implementação simplificada: retorna T
    return T;
}
Spectrum MicrofacetTransmission::Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const {
    // Implementação simplificada: amostra uniforme
    *wi = MonteCarlo::CosineSampleHemisphere(u);
    *pdf = MonteCarlo::CosineHemispherePdf(SpectralUtils::CosTheta(*wi));
    return f(wo, *wi);
}

// BRDF default implementations
Spectrum BRDF::Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const {
    *wi = MonteCarlo::CosineSampleHemisphere(u);
    *pdf = MonteCarlo::CosineHemispherePdf(SpectralUtils::CosTheta(*wi));
    return f(wo, *wi);
}
float BRDF::Pdf(const glm::vec3& wo, const glm::vec3& wi) const {
    return MonteCarlo::CosineHemispherePdf(SpectralUtils::CosTheta(wi));
}

// LambertianReflection implementations
Spectrum LambertianReflection::f(const glm::vec3& wo, const glm::vec3& wi) const {
    return R * (1.0f / M_PI);
}
Spectrum LambertianReflection::Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const {
    *wi = MonteCarlo::CosineSampleHemisphere(u);
    if (wo.z < 0) wi->z *= -1;
    *pdf = Pdf(wo, *wi);
    return f(wo, *wi);
}

// Utility functions
namespace SpectralUtils {
    float CosTheta(const glm::vec3& w) { return w.z; }
    float Cos2Theta(const glm::vec3& w) { return w.z * w.z; }
    float AbsCosTheta(const glm::vec3& w) { return std::abs(w.z); }
    float Sin2Theta(const glm::vec3& w) { return std::max(0.0f, 1.0f - Cos2Theta(w)); }
    float SinTheta(const glm::vec3& w) { return std::sqrt(Sin2Theta(w)); }
    float TanTheta(const glm::vec3& w) { return SinTheta(w) / CosTheta(w); }
    float Tan2Theta(const glm::vec3& w) { return Sin2Theta(w) / Cos2Theta(w); }
    bool SameHemisphere(const glm::vec3& w, const glm::vec3& wp) { return w.z * wp.z > 0; }
    glm::vec3 SphericalDirection(float sinTheta, float cosTheta, float phi) {
        return glm::vec3(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
    }
    float PlanckianLocus(float lambda, float temperature) {
        const float h = 6.62607015e-34f; // Planck constant
        const float c = 299792458.0f;    // Speed of light
        const float k = 1.380649e-23f;   // Boltzmann constant
        float numerator = 2 * h * c * c / (lambda * lambda * lambda * lambda * lambda);
        float denominator = std::exp(h * c / (lambda * k * temperature)) - 1.0f;
        return numerator / denominator;
    }
}

// Monte Carlo utilities
namespace MonteCarlo {
    glm::vec3 CosineSampleHemisphere(const glm::vec2& u) {
        glm::vec2 d = 2.0f * u - glm::vec2(1.0f);
        if (d.x == 0 && d.y == 0) return glm::vec3(0, 0, 1);
        float radius, theta;
        if (std::abs(d.x) > std::abs(d.y)) {
            radius = d.x;
            theta = (M_PI / 4) * (d.y / d.x);
        } else {
            radius = d.y;
            theta = (M_PI / 2) - (M_PI / 4) * (d.x / d.y);
        }
        float x = radius * std::cos(theta);
        float y = radius * std::sin(theta);
        float z = std::sqrt(std::max(0.0f, 1 - x * x - y * y));
        return glm::vec3(x, y, z);
    }
    glm::vec3 UniformSampleSphere(const glm::vec2& u) {
        float z = 1 - 2 * u[0];
        float r = std::sqrt(std::max(0.0f, 1 - z * z));
        float phi = 2 * M_PI * u[1];
        return glm::vec3(r * std::cos(phi), r * std::sin(phi), z);
    }
    float CosineHemispherePdf(float cosTheta) {
        return cosTheta / M_PI;
    }
}

// Global operators implementation
Spectrum operator*(float scalar, const Spectrum& spectrum) {
    return spectrum * scalar;
}