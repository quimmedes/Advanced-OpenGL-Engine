#pragma once
#include "Spectrum.hpp"
#include "Light.hpp"
#include "Material.hpp"
#include <memory>
#include <vector>
#include <random>
#include "Mesh.hpp"

// Ray structure for path tracing
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float tMin = 0.001f;
    float tMax = std::numeric_limits<float>::infinity();
    
    Ray() = default;
    Ray(const glm::vec3& o, const glm::vec3& d, float tMin = 0.001f, float tMax = std::numeric_limits<float>::infinity())
        : origin(o), direction(d), tMin(tMin), tMax(tMax) {}
    
    glm::vec3 operator()(float t) const { return origin + t * direction; }
};

// Surface interaction
struct SurfaceInteraction {
    glm::vec3 p;           // Hit point
    glm::vec3 n;           // Surface normal
    glm::vec3 wo;          // Outgoing direction (toward camera)
    glm::vec2 uv;          // Texture coordinates
    float t;               // Ray parameter
    
    // Shading geometry
    glm::vec3 dpdu, dpdv;  // Partial derivatives
    glm::vec3 dndu, dndv;  // Normal derivatives
    
    // Material
    const Material* material = nullptr;
    
    // BSDF at interaction point
    std::unique_ptr<BRDF> bsdf;
    
    // Compute scattered ray
    Spectrum ComputeScatteringFunctions(const Ray& ray);
    
    // Spawn ray from surface
    Ray SpawnRay(const glm::vec3& d) const;
    Ray SpawnRayTo(const glm::vec3& p2) const;
};

// Light sampling
struct LightSample {
    Spectrum Li;           // Incident radiance
    glm::vec3 wi;         // Incident direction
    float pdf;            // Probability density
    glm::vec3 p;          // Light surface point
    bool isDelta = false; // Is delta light
};

// Volume scattering for participating media
struct VolumeInteraction {
    glm::vec3 p;          // Interaction point
    glm::vec3 wo;         // Outgoing direction
    float time;           // Time
    
    // Phase function for volume scattering
    float Phase(const glm::vec3& wo, const glm::vec3& wi) const;
    Spectrum SamplePhase(const glm::vec3& wo, glm::vec3* wi, const glm::vec2& u, float* pdf) const;
};

// Scene abstraction for ray tracing
class Scene {
public:
    virtual ~Scene() = default;
    
    // Ray intersection
    virtual bool Intersect(const Ray& ray, SurfaceInteraction* isect) const = 0;
    virtual bool IntersectP(const Ray& ray) const = 0; // Shadow rays
    
    // Light sampling
    virtual Spectrum SampleLight(const glm::vec2& u, LightSample* sample) const = 0;
    virtual float LightPdf(const LightSample& sample) const = 0;
    
    // Environment lighting
    virtual Spectrum Le(const Ray& ray) const { return Spectrum(0.0f); }
    
    // Volume properties
    virtual Spectrum Tr(const Ray& ray) const { return Spectrum(1.0f); } // Transmittance
    virtual Spectrum SampleVolumeScattering(const Ray& ray, VolumeInteraction* vi, const glm::vec2& u) const;
    
protected:
    std::vector<std::unique_ptr<Light>> lights;
    std::vector<std::unique_ptr<Mesh>> meshes;
};

// Path tracing integrator from PBR Chapter 14
class PathIntegrator {
public:
    PathIntegrator(int maxDepth = 8, float rrThreshold = 1.0f);
    
    // Main rendering function
    Spectrum Li(const Ray& ray, const Scene& scene, std::mt19937& rng, int depth = 0) const;
    
    // Render a tile of the image
    void RenderTile(int startX, int startY, int endX, int endY, 
                   const Scene& scene, float* pixels, int width, int height,
                   const glm::mat4& cameraToWorld, float fov) const;
    
    // Sample camera ray
    Ray GenerateCameraRay(int x, int y, int width, int height, 
                         const glm::mat4& cameraToWorld, float fov, 
                         const glm::vec2& sample = glm::vec2(0.5f)) const;
    
private:
    int maxDepth;
    float rrThreshold; // Russian roulette threshold
    
    // Direct lighting estimation
    Spectrum EstimateDirect(const SurfaceInteraction& it, const glm::vec2& uLight,
                           const glm::vec2& uBSDF, const Scene& scene, std::mt19937& rng) const;
    
    // Sample one light uniformly
    Spectrum SampleOneLight(const SurfaceInteraction& it, const Scene& scene, 
                           std::mt19937& rng) const;
    
    // Multiple importance sampling for light and BRDF
    Spectrum MISEstimate(const SurfaceInteraction& it, const glm::vec2& uLight,
                        const glm::vec2& uBSDF, const Scene& scene, std::mt19937& rng) const;
};

// Bidirectional path tracing for advanced effects
class BidirectionalPathIntegrator {
public:
    BidirectionalPathIntegrator(int maxDepth = 8);
    
    Spectrum Li(const Ray& ray, const Scene& scene, std::mt19937& rng) const;
    
private:
    int maxDepth;
    
    // Path vertex structure
    struct Vertex {
        glm::vec3 p;
        glm::vec3 n;
        glm::vec3 wo;
        Spectrum beta; // Path throughput
        float pdfFwd, pdfRev; // Forward and reverse PDFs
        bool isDelta = false;
    };
    
    // Generate light and camera subpaths
    int GenerateCameraSubpath(const Ray& ray, const Scene& scene, std::vector<Vertex>& path, std::mt19937& rng) const;
    int GenerateLightSubpath(const Scene& scene, std::vector<Vertex>& path, std::mt19937& rng) const;
    
    // Connect subpaths
    Spectrum ConnectBDPT(const std::vector<Vertex>& cameraPath, const std::vector<Vertex>& lightPath,
                        int s, int t, const Scene& scene) const;
};

// Volumetric path tracing for participating media
class VolumetricPathIntegrator {
public:
    VolumetricPathIntegrator(int maxDepth = 8);
    
    Spectrum Li(const Ray& ray, const Scene& scene, std::mt19937& rng) const;
    
private:
    int maxDepth;
    
    // Sample volume scattering
    Spectrum SampleVolumeScattering(const Ray& ray, const Scene& scene, std::mt19937& rng) const;
    
    // Phase function sampling
    float HenyeyGreenstein(float cosTheta, float g) const;
    float SampleHenyeyGreenstein(float g, const glm::vec2& u) const;
};

// Metropolis Light Transport for difficult scenes
class MetropolisIntegrator {
public:
    MetropolisIntegrator(int maxDepth = 8, float largeStepProbability = 0.3f);
    
    void Render(const Scene& scene, float* pixels, int width, int height,
               const glm::mat4& cameraToWorld, float fov, int numSamples) const;
    
private:
    int maxDepth;
    float largeStepProbability;
    
    // Path perturbation strategies
    struct PathSample {
        std::vector<glm::vec2> cameraSamples;
        std::vector<glm::vec2> lightSamples;
        std::vector<glm::vec2> bsdfSamples;
        Spectrum L;
        float pdf;
    };
    
    PathSample GeneratePath(const Scene& scene, const Ray& ray, std::mt19937& rng) const;
    PathSample PerturbPath(const PathSample& oldPath, std::mt19937& rng) const;
    float AcceptanceRatio(const PathSample& oldPath, const PathSample& newPath) const;
};

// Photon mapping for caustics and global illumination
class PhotonMappingIntegrator {
public:
    PhotonMappingIntegrator(int nPhotons = 1000000, int maxDepth = 8, float searchRadius = 0.1f);
    
    void Preprocess(const Scene& scene);
    Spectrum Li(const Ray& ray, const Scene& scene, std::mt19937& rng) const;
    
private:
    int nPhotons;
    int maxDepth;
    float searchRadius;
    
    struct Photon {
        glm::vec3 p;
        glm::vec3 wi;
        Spectrum phi; // Power
    };
    
    std::vector<Photon> globalPhotons;
    std::vector<Photon> causticPhotons;
    
    // K-d tree for photon storage
    class PhotonMap {
    public:
        void Store(const std::vector<Photon>& photons);
        Spectrum EstimateRadiance(const glm::vec3& p, const glm::vec3& n, float searchRadius, int maxPhotons) const;
    private:
        std::vector<Photon> photons;
        // K-d tree implementation would go here
    };
    
    PhotonMap globalMap, causticMap;
    
    void TracePhotons(const Scene& scene);
    Spectrum EstimateDirectLighting(const SurfaceInteraction& it, const Scene& scene, std::mt19937& rng) const;
    Spectrum EstimateIndirectLighting(const SurfaceInteraction& it) const;
};

// Subsurface scattering (BSSRDF) implementation
class SubsurfaceScattering {
public:
    SubsurfaceScattering(const Spectrum& sigma_a, const Spectrum& sigma_s, float g = 0.0f);
    
    // Evaluate BSSRDF
    Spectrum S(const glm::vec3& po, const glm::vec3& wo, const glm::vec3& pi, const glm::vec3& wi) const;
    
    // Sample scattering
    Spectrum Sample_S(const glm::vec3& po, const glm::vec3& wo, glm::vec3* pi, glm::vec3* wi, 
                     const glm::vec2& u, float* pdf) const;
    
private:
    Spectrum sigma_a, sigma_s; // Absorption and scattering coefficients
    Spectrum sigma_t;          // Extinction coefficient
    Spectrum albedo;           // Single scattering albedo
    float g;                   // Phase function asymmetry parameter
    
    // Diffusion approximation
    float FresnelMoment1(float eta) const;
    float FresnelMoment2(float eta) const;
    
    // Dipole approximation for subsurface scattering
    Spectrum Rd(float distance) const;
    float SampleDistance(float u) const;
};

// Denoising for Monte Carlo noise reduction
class Denoiser {
public:
    virtual ~Denoiser() = default;
    
    // Denoise rendered image
    virtual void Denoise(float* input, float* output, int width, int height, int channels) const = 0;
};

// Temporal denoising using previous frames
class TemporalDenoiser : public Denoiser {
public:
    TemporalDenoiser(float alpha = 0.1f);
    
    void Denoise(float* input, float* output, int width, int height, int channels) const override;
    void SetPreviousFrame(const float* prevFrame, int width, int height, int channels);
    
private:
    float alpha; // Temporal blending factor
    std::vector<float> previousFrame;
    int prevWidth = 0, prevHeight = 0, prevChannels = 0;
};

// Spatial denoising using edge-preserving filters
class SpatialDenoiser : public Denoiser {
public:
    SpatialDenoiser(float sigmaColor = 0.1f, float sigmaSpace = 2.0f, int kernelSize = 5);
    
    void Denoise(float* input, float* output, int width, int height, int channels) const override;
    
private:
    float sigmaColor, sigmaSpace;
    int kernelSize;
    
    // Bilateral filter implementation
    float BilateralWeight(const glm::vec3& center, const glm::vec3& neighbor, float spatialDist) const;
};