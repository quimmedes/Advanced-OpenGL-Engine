#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <string>
#include <limits>

// Spectral rendering based on PBR book Chapter 5
constexpr int SPECTRAL_SAMPLES = 60;
constexpr float LAMBDA_MIN = 400.0f; // nm
constexpr float LAMBDA_MAX = 700.0f; // nm

class Spectrum {
public:
    Spectrum(float value = 0.0f);
    Spectrum(const std::array<float, SPECTRAL_SAMPLES>& samples);
    
    // Factory methods for common spectra
    static Spectrum FromRGB(const glm::vec3& rgb);
    static Spectrum FromXYZ(const glm::vec3& xyz);
    static Spectrum FromBlackbody(float temperature); // Kelvin
    static Spectrum FromD65Illuminant();
    
    // Spectral operations
    Spectrum operator+(const Spectrum& other) const;
    Spectrum operator-(const Spectrum& other) const;
    Spectrum operator*(const Spectrum& other) const;
    Spectrum operator*(float scalar) const;
    Spectrum operator/(float scalar) const;
    Spectrum operator/(const Spectrum& other) const;
    
    // Assignment operators
    Spectrum& operator+=(const Spectrum& other);
    Spectrum& operator-=(const Spectrum& other);
    Spectrum& operator*=(const Spectrum& other);
    Spectrum& operator*=(float scalar);
    Spectrum& operator/=(float scalar);
    
    // Color space conversions
    glm::vec3 toRGB() const;
    glm::vec3 toXYZ() const;
    
    // Spectral utilities
    float integrate() const;
    float luminance() const;
    bool isBlack() const;
    void clamp(float min = 0.0f, float max = std::numeric_limits<float>::infinity());
    
    // Sample access
    float& operator[](int i) { return samples[i]; }
    const float& operator[](int i) const { return samples[i]; }
    
    // Wavelength utilities
    static float wavelengthToIndex(float lambda);
    static float indexToWavelength(int index);
    
private:
    std::array<float, SPECTRAL_SAMPLES> samples;
    
    // Color matching functions and illuminants
    static const std::array<float, SPECTRAL_SAMPLES> CIE_X;
    static const std::array<float, SPECTRAL_SAMPLES> CIE_Y;
    static const std::array<float, SPECTRAL_SAMPLES> CIE_Z;
    static const std::array<float, SPECTRAL_SAMPLES> D65_ILLUMINANT;
    
    // RGB to spectrum conversion matrices
    static const glm::mat3 RGB_TO_XYZ;
    static const glm::mat3 XYZ_TO_RGB;
};

// Microfacet distribution functions from PBR Chapter 8
class MicrofacetDistribution {
public:
    virtual ~MicrofacetDistribution() = default;
    
    // Core microfacet functions
    virtual float D(const glm::vec3& wh) const = 0; // Normal distribution
    virtual float Lambda(const glm::vec3& w) const = 0; // Masking function
    virtual glm::vec3 Sample_wh(const glm::vec3& wo, const glm::vec2& u) const = 0; // Importance sampling
    
    // Derived functions
    float G1(const glm::vec3& w) const { return 1.0f / (1.0f + Lambda(w)); }
    float G(const glm::vec3& wo, const glm::vec3& wi) const { return 1.0f / (1.0f + Lambda(wo) + Lambda(wi)); }
    float Pdf(const glm::vec3& wo, const glm::vec3& wh) const;
    
protected:
    bool sampleVisibleArea;
    
    MicrofacetDistribution(bool sampleVisibleArea = true) : sampleVisibleArea(sampleVisibleArea) {}
};

// Trowbridge-Reitz (GGX) distribution
class TrowbridgeReitzDistribution : public MicrofacetDistribution {
public:
    TrowbridgeReitzDistribution(float alphaX, float alphaY, bool sampleVis = true);
    
    float D(const glm::vec3& wh) const override;
    float Lambda(const glm::vec3& w) const override;
    glm::vec3 Sample_wh(const glm::vec3& wo, const glm::vec2& u) const override;
    
private:
    float alphaX, alphaY;
    
    float RoughnessToAlpha(float roughness) const { return std::sqrt(roughness); }
};

// Beckmann distribution for comparison
class BeckmannDistribution : public MicrofacetDistribution {
public:
    BeckmannDistribution(float alphaX, float alphaY, bool sampleVis = true);
    
    float D(const glm::vec3& wh) const override;
    float Lambda(const glm::vec3& w) const override;
    glm::vec3 Sample_wh(const glm::vec3& wo, const glm::vec2& u) const override;
    
private:
    float alphaX, alphaY;
};

// Fresnel equations from PBR Chapter 8
class Fresnel {
public:
    virtual ~Fresnel() = default;
    virtual Spectrum Evaluate(float cosThetaI) const = 0;
};

class FresnelConductor : public Fresnel {
public:
    FresnelConductor(const Spectrum& etaI, const Spectrum& etaT, const Spectrum& k);
    Spectrum Evaluate(float cosThetaI) const override;
    
private:
    Spectrum etaI, etaT, k;
};

class FresnelDielectric : public Fresnel {
public:
    FresnelDielectric(float etaI, float etaT);
    Spectrum Evaluate(float cosThetaI) const override;
    
private:
    float etaI, etaT;
};

class FresnelNoOp : public Fresnel {
public:
    Spectrum Evaluate(float) const override { return Spectrum(1.0f); }
};

// Monte Carlo utilities from PBR Chapter 13
namespace MonteCarlo {
    // Low-discrepancy sampling
    glm::vec2 Hammersley(uint32_t i, uint32_t N);
    glm::vec2 VanDerCorput(uint32_t n, uint32_t base);
    
    // Hemisphere sampling
    glm::vec3 CosineSampleHemisphere(const glm::vec2& u);
    glm::vec3 UniformSampleHemisphere(const glm::vec2& u);
    glm::vec3 UniformSampleSphere(const glm::vec2& u);
    
    // PDF functions
    float CosineHemispherePdf(float cosTheta);
    float UniformHemispherePdf();
    float UniformSpherePdf();
    
    // Multiple importance sampling
    float PowerHeuristic(int nf, float fPdf, int ng, float gPdf);
    float BalanceHeuristic(int nf, float fPdf, int ng, float gPdf);
    
    // Stratified sampling
    void StratifiedSample1D(float* samples, int nSamples, bool jitter = true);
    void StratifiedSample2D(glm::vec2* samples, int nx, int ny, bool jitter = true);
}

// Advanced BRDF from PBR Chapter 8
class BRDF {
public:
    virtual ~BRDF() = default;
    
    // Core BRDF interface
    virtual Spectrum f(const glm::vec3& wo, const glm::vec3& wi) const = 0;
    virtual Spectrum Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const;
    virtual float Pdf(const glm::vec3& wo, const glm::vec3& wi) const;
    
    // BRDF properties
    virtual bool isDelta() const { return false; }
    virtual bool hasSpecular() const { return false; }
    virtual bool hasDiffuse() const { return false; }
};

// Lambertian diffuse BRDF
class LambertianReflection : public BRDF {
public:
    LambertianReflection(const Spectrum& R);
    
    Spectrum f(const glm::vec3& wo, const glm::vec3& wi) const override;
    Spectrum Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const override;
    
    bool hasDiffuse() const override { return true; }
    
private:
    Spectrum R; // Reflectance
};

// Oren-Nayar diffuse BRDF for rough surfaces
class OrenNayar : public BRDF {
public:
    OrenNayar(const Spectrum& R, float sigma);
    
    Spectrum f(const glm::vec3& wo, const glm::vec3& wi) const override;
    
    bool hasDiffuse() const override { return true; }
    
private:
    Spectrum R;
    float A, B; // Oren-Nayar coefficients
};

// Microfacet reflection BRDF
class MicrofacetReflection : public BRDF {
public:
    MicrofacetReflection(const Spectrum& R, std::unique_ptr<MicrofacetDistribution> distribution,
                        std::unique_ptr<Fresnel> fresnel);
    
    Spectrum f(const glm::vec3& wo, const glm::vec3& wi) const override;
    Spectrum Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const override;
    float Pdf(const glm::vec3& wo, const glm::vec3& wi) const override;
    
    bool hasSpecular() const override { return true; }
    
private:
    const Spectrum R;
    std::unique_ptr<MicrofacetDistribution> distribution;
    std::unique_ptr<Fresnel> fresnel;
};

// Microfacet transmission BRDF
class MicrofacetTransmission : public BRDF {
public:
    MicrofacetTransmission(const Spectrum& T, std::unique_ptr<MicrofacetDistribution> distribution,
                          float etaA, float etaB);
    
    Spectrum f(const glm::vec3& wo, const glm::vec3& wi) const override;
    Spectrum Sample_f(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const override;
    
private:
    const Spectrum T;
    std::unique_ptr<MicrofacetDistribution> distribution;
    float etaA, etaB;
};

// Utility functions
namespace SpectralUtils {
    // Coordinate system construction
    void CoordinateSystem(const glm::vec3& v1, glm::vec3* v2, glm::vec3* v3);
    glm::vec3 SphericalDirection(float sinTheta, float cosTheta, float phi);
    glm::vec3 SphericalDirection(float sinTheta, float cosTheta, float phi, 
                                const glm::vec3& x, const glm::vec3& y, const glm::vec3& z);
    
    // Reflection utilities
    float CosTheta(const glm::vec3& w);
    float Cos2Theta(const glm::vec3& w);
    float AbsCosTheta(const glm::vec3& w);
    float Sin2Theta(const glm::vec3& w);
    float SinTheta(const glm::vec3& w);
    float TanTheta(const glm::vec3& w);
    float Tan2Theta(const glm::vec3& w);
    float CosPhi(const glm::vec3& w);
    float SinPhi(const glm::vec3& w);
    
    bool SameHemisphere(const glm::vec3& w, const glm::vec3& wp);
    glm::vec3 Reflect(const glm::vec3& wo, const glm::vec3& n);
    bool Refract(const glm::vec3& wi, const glm::vec3& n, float eta, glm::vec3* wt);
    
    // Blackbody radiation
    float PlanckianLocus(float lambda, float temperature);
    
    // Color temperature utilities
    glm::vec3 ColorTemperatureToRGB(float temperature);
    float RGBToColorTemperature(const glm::vec3& rgb);
}

// Global operators for Spectrum
Spectrum operator*(float scalar, const Spectrum& spectrum);