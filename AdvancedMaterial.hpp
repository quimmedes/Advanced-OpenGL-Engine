#pragma once
#include "Material.hpp"
#include "Spectrum.hpp"
#include "Integrator.hpp"
#include <memory>
#include <vector>

// Advanced material system based on PBR principles
class AdvancedMaterial {
public:
    virtual ~AdvancedMaterial() = default;
    
    // Core material interface
    virtual void ComputeScatteringFunctions(SurfaceInteraction* si) const = 0;
    virtual Spectrum Le(const glm::vec3& wo) const { return Spectrum(0.0f); }
    virtual bool hasSubsurface() const { return false; }
    
    // Material properties
    virtual float getIOR() const { return 1.5f; }
    virtual bool isEmissive() const { return false; }
    virtual bool isTransparent() const { return false; }
    
protected:
    // Texture sampling utilities
    Spectrum SampleTexture(const std::shared_ptr<Texture>& texture, const glm::vec2& uv) const;
    glm::vec3 SampleNormalMap(const std::shared_ptr<Texture>& normalMap, const glm::vec2& uv,
                             const glm::vec3& normal, const glm::vec3& tangent) const;
};

// Disney BRDF material (principled approach)
class DisneyMaterial : public AdvancedMaterial {
public:
    DisneyMaterial(const Spectrum& baseColor, float metallic = 0.0f, float roughness = 0.5f,
                  float specular = 0.5f, float specularTint = 0.0f, float sheen = 0.0f,
                  float sheenTint = 0.5f, float clearcoat = 0.0f, float clearcoatGloss = 1.0f,
                  float subsurface = 0.0f, float transmission = 0.0f, float ior = 1.5f);
    
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    
    // Disney BRDF evaluation
    Spectrum EvaluateDisneyBRDF(const glm::vec3& wo, const glm::vec3& wi, const SurfaceInteraction& si) const;
    
    // Texture support
    void SetBaseColorTexture(std::shared_ptr<Texture> texture) { baseColorTexture = texture; }
    void SetMetallicTexture(std::shared_ptr<Texture> texture) { metallicTexture = texture; }
    void SetRoughnessTexture(std::shared_ptr<Texture> texture) { roughnessTexture = texture; }
    void SetNormalTexture(std::shared_ptr<Texture> texture) { normalTexture = texture; }
    
    bool hasSubsurface() const override { return subsurface > 0.0f; }
    bool isTransparent() const override { return transmission > 0.0f; }
    float getIOR() const override { return ior; }
    
private:
    // Disney parameters
    Spectrum baseColor;
    float metallic, roughness, specular, specularTint;
    float sheen, sheenTint, clearcoat, clearcoatGloss;
    float subsurface, transmission, ior;
    
    // Textures
    std::shared_ptr<Texture> baseColorTexture;
    std::shared_ptr<Texture> metallicTexture;
    std::shared_ptr<Texture> roughnessTexture;
    std::shared_ptr<Texture> normalTexture;
    
    // Disney BRDF components
    Spectrum DisneyDiffuse(const glm::vec3& wo, const glm::vec3& wi, const Spectrum& baseColor,
                          float roughness, float subsurface) const;
    Spectrum DisneyFakeSS(const glm::vec3& wo, const glm::vec3& wi, const Spectrum& baseColor,
                         float roughness) const;
    Spectrum DisneySheen(const glm::vec3& wo, const glm::vec3& wi, const Spectrum& baseColor,
                        float sheen, float sheenTint) const;
    Spectrum DisneyClearcoat(const glm::vec3& wo, const glm::vec3& wi, float clearcoat,
                            float clearcoatGloss) const;
};

// Measured material (using tabulated BRDF data)
class MeasuredMaterial : public AdvancedMaterial {
public:
    MeasuredMaterial(const std::string& filename);
    
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    
private:
    // MERL BRDF data
    struct BRDFData {
        int nThetaH, nThetaD, nPhiD;
        std::vector<Spectrum> data;
    } brdfData;
    
    // Lookup BRDF value
    Spectrum LookupBRDF(float thetaH, float thetaD, float phiD) const;
    void LoadMERLBRDF(const std::string& filename);
};

// Glass material with realistic dispersion
class GlassMaterial : public AdvancedMaterial {
public:
    GlassMaterial(const Spectrum& kr = Spectrum(1.0f), const Spectrum& kt = Spectrum(1.0f),
                 float eta = 1.5f, bool dispersive = false);
    
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    
    bool isTransparent() const override { return true; }
    float getIOR() const override { return eta; }
    
private:
    Spectrum Kr, Kt; // Reflection and transmission coefficients
    float eta;       // Index of refraction
    bool dispersive; // Enable chromatic dispersion
    
    // Sellmeier equation for dispersion
    float GetIORForWavelength(float lambda) const;
};

// Metal material with complex IOR
class MetalMaterial : public AdvancedMaterial {
public:
    MetalMaterial(const Spectrum& eta, const Spectrum& k, float roughness = 0.01f);
    
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    
    // Presets for common metals
    static MetalMaterial* CreateGold(float roughness = 0.01f);
    static MetalMaterial* CreateSilver(float roughness = 0.01f);
    static MetalMaterial* CreateCopper(float roughness = 0.01f);
    static MetalMaterial* CreateAluminum(float roughness = 0.01f);
    
private:
    Spectrum eta, k; // Complex index of refraction
    float roughness;
    std::shared_ptr<Texture> roughnessTexture;
};

// Subsurface scattering material
class SubsurfaceMaterial : public AdvancedMaterial {
public:
    SubsurfaceMaterial(const Spectrum& kr, const Spectrum& kt, const Spectrum& sigma_a,
                      const Spectrum& sigma_s, float g = 0.0f, float eta = 1.33f, float scale = 1.0f);
    
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    
    bool hasSubsurface() const override { return true; }
    float getIOR() const override { return eta; }
    
    // Material presets
    static SubsurfaceMaterial* CreateSkin();
    static SubsurfaceMaterial* CreateMilk();
    static SubsurfaceMaterial* CreateMarble();
    static SubsurfaceMaterial* CreateWax();
    
private:
    Spectrum Kr, Kt;           // Surface reflection/transmission
    Spectrum sigma_a, sigma_s; // Absorption and scattering coefficients
    float g;                   // Phase function asymmetry
    float eta;                 // Index of refraction
    float scale;               // Scale factor for subsurface scattering
    
    std::unique_ptr<SubsurfaceScattering> bssrdf;
};

// Emissive material for area lights
class EmissiveMaterial : public AdvancedMaterial {
public:
    EmissiveMaterial(const Spectrum& Le, float power = 1.0f, bool twoSided = false);
    
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    Spectrum Le(const glm::vec3& wo) const override;
    
    bool isEmissive() const override { return true; }
    
    // Temperature-based emission
    static EmissiveMaterial* CreateBlackbody(float temperature, float power = 1.0f);
    static EmissiveMaterial* CreateSun(); // 5778K blackbody
    static EmissiveMaterial* CreateIncandescent(); // 2700K
    static EmissiveMaterial* CreateFluorescent(); // 4100K
    
private:
    Spectrum emission;
    float power;
    bool twoSided;
    std::shared_ptr<Texture> emissionTexture;
};

// Layered material system
class LayeredMaterial : public AdvancedMaterial {
public:
    LayeredMaterial();
    
    void AddLayer(std::shared_ptr<AdvancedMaterial> material, float thickness = 0.0f);
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    
    bool hasSubsurface() const override;
    bool isTransparent() const override;
    
private:
    struct Layer {
        std::shared_ptr<AdvancedMaterial> material;
        float thickness;
    };
    
    std::vector<Layer> layers;
    
    // Adding-doubling method for layer combination
    void CombineLayers(SurfaceInteraction* si) const;
};

// Procedural noise material
class NoiseMaterial : public AdvancedMaterial {
public:
    NoiseMaterial(std::shared_ptr<AdvancedMaterial> mat1, std::shared_ptr<AdvancedMaterial> mat2,
                 float scale = 1.0f, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f);
    
    void ComputeScatteringFunctions(SurfaceInteraction* si) const override;
    
private:
    std::shared_ptr<AdvancedMaterial> material1, material2;
    float scale, lacunarity, gain;
    int octaves;
    
    // Noise functions
    float Noise(const glm::vec3& p) const;
    float FBM(const glm::vec3& p) const; // Fractal Brownian Motion
    float Turbulence(const glm::vec3& p) const;
};

// Advanced texture system
class AdvancedTexture {
public:
    virtual ~AdvancedTexture() = default;
    virtual Spectrum Evaluate(const glm::vec2& uv) const = 0;
    virtual glm::vec3 EvaluateNormal(const glm::vec2& uv) const { return glm::vec3(0, 0, 1); }
};

// Procedural texture types
class CheckerboardTexture : public AdvancedTexture {
public:
    CheckerboardTexture(const Spectrum& color1, const Spectrum& color2, float scale = 1.0f);
    Spectrum Evaluate(const glm::vec2& uv) const override;
    
private:
    Spectrum color1, color2;
    float scale;
};

class MarbleTexture : public AdvancedTexture {
public:
    MarbleTexture(const Spectrum& color1, const Spectrum& color2, float scale = 1.0f, int octaves = 4);
    Spectrum Evaluate(const glm::vec2& uv) const override;
    
private:
    Spectrum color1, color2;
    float scale;
    int octaves;
};

class WoodTexture : public AdvancedTexture {
public:
    WoodTexture(const Spectrum& lightWood, const Spectrum& darkWood, float ringFreq = 8.0f);
    Spectrum Evaluate(const glm::vec2& uv) const override;
    
private:
    Spectrum lightWood, darkWood;
    float ringFreq;
};

// High Dynamic Range texture
class HDRTexture : public AdvancedTexture {
public:
    HDRTexture(const std::string& filename);
    Spectrum Evaluate(const glm::vec2& uv) const override;
    
    // Environment map sampling
    glm::vec3 SampleDirection(const glm::vec2& u, float* pdf) const;
    float Pdf(const glm::vec3& wi) const;
    
private:
    int width, height;
    std::vector<Spectrum> pixels;
    std::vector<float> luminances; // For importance sampling
    
    void LoadHDR(const std::string& filename);
    void ComputeLuminanceDistribution();
};

// Material factory for easy creation
class MaterialFactory {
public:
    // Standard material types
    static std::shared_ptr<AdvancedMaterial> CreatePlastic(const Spectrum& color, float roughness = 0.1f);
    static std::shared_ptr<AdvancedMaterial> CreateRubber(const Spectrum& color, float roughness = 0.7f);
    static std::shared_ptr<AdvancedMaterial> CreatePaint(const Spectrum& color, float metallic = 0.0f);
    static std::shared_ptr<AdvancedMaterial> CreateFabric(const Spectrum& color, float roughness = 0.8f);
    static std::shared_ptr<AdvancedMaterial> CreateCeramic(const Spectrum& color, float roughness = 0.02f);
    
    // Natural materials
    static std::shared_ptr<AdvancedMaterial> CreateWood(const std::string& type = "oak");
    static std::shared_ptr<AdvancedMaterial> CreateStone(const std::string& type = "granite");
    static std::shared_ptr<AdvancedMaterial> CreateSoil(const Spectrum& color);
    static std::shared_ptr<AdvancedMaterial> CreateWater(float roughness = 0.0f);
    
    // Organic materials
    static std::shared_ptr<AdvancedMaterial> CreateSkin(const std::string& type = "caucasian");
    static std::shared_ptr<AdvancedMaterial> CreateLeaves(const Spectrum& color);
    static std::shared_ptr<AdvancedMaterial> CreateFood(const std::string& type);
    
private:
    // Helper functions for material parameter lookup
    static void GetMaterialParameters(const std::string& name, Spectrum* color, float* roughness,
                                    float* metallic, float* ior);
};