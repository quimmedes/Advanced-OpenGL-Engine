#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <complex>
#include <random>
#include <memory>
#include "Shader.hpp"

class OceanFFT {
public:
    // Tessendorf wave parameters
    struct WaveParameters {
        float A = 0.0001f;          // Wave amplitude scaling factor
        glm::vec2 windSpeed = glm::vec2(32.0f, 32.0f);  // Wind speed in m/s
        glm::vec2 windDirection = glm::vec2(1.0f, 1.0f); // Wind direction (normalized)
        float lambda = -1.0f;       // Choppiness parameter
        float L = 200.0f;           // Length scale for largest waves
        float damping = 0.001f;     // Wave damping factor
        float gravity = 9.81f;      // Gravitational constant
    };
    
    // Ocean configuration
    struct OceanConfig {
        int N = 512;                // Grid resolution (must be power of 2)
        float oceanSize = 1000.0f;  // Physical size in meters
        float timeScale = 1.0f;     // Time animation speed multiplier
        bool enableChoppiness = true; // Enable horizontal displacement
        bool enableFoam = true;     // Enable foam generation
        float foamThreshold = 0.8f; // Foam generation threshold
    };
    
    // Complex number for FFT calculations
    struct Complex {
        float real, imag;
        Complex(float r = 0.0f, float i = 0.0f) : real(r), imag(i) {}
        
        Complex operator+(const Complex& other) const {
            return Complex(real + other.real, imag + other.imag);
        }
        
        Complex operator*(const Complex& other) const {
            return Complex(real * other.real - imag * other.imag,
                          real * other.imag + imag * other.real);
        }
        
        Complex conjugate() const {
            return Complex(real, -imag);
        }
        
        float magnitude() const {
            return sqrt(real * real + imag * imag);
        }
    };

private:
    // Ocean parameters
    WaveParameters waveParams;
    OceanConfig config;
    
    // OpenGL resources
    GLuint VAO, VBO, EBO;
    GLuint heightmapTexture, displacementTexture, normalTexture, foamTexture;
    GLuint spectrumTexture, phaseTexture;
    GLuint framebuffer;
    
    // Compute shaders
    std::unique_ptr<Shader> spectrumShader;      // Generate initial spectrum
    std::unique_ptr<Shader> fftHorizontalShader; // Horizontal FFT pass
    std::unique_ptr<Shader> fftVerticalShader;   // Vertical FFT pass
    std::unique_ptr<Shader> combineMapsShader;   // Combine height/displacement maps
    
    // Rendering shaders
    std::unique_ptr<Shader> oceanVertexShader;
    std::unique_ptr<Shader> oceanFragmentShader;
    
    // CPU-side data
    std::vector<Complex> initialSpectrum;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Animation
    float time = 0.0f;
    bool isInitialized = false;
    
    // Random number generation
    std::mt19937 rng;
    std::normal_distribution<float> gaussianDist;

public:
    OceanFFT();
    ~OceanFFT();
    
    // Initialization and cleanup
    bool Initialize(const OceanConfig& cfg = OceanConfig{});
    void Cleanup();
    bool IsInitialized() const { return isInitialized; }
    
    // Wave parameter management
    void SetWaveParameters(const WaveParameters& params);
    const WaveParameters& GetWaveParameters() const { return waveParams; }
    
    void SetOceanConfig(const OceanConfig& cfg);
    const OceanConfig& GetOceanConfig() const { return config; }
    
    // Animation and rendering
    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::vec3& cameraPos, const glm::vec3& lightDir,
                const glm::vec3& lightColor, const glm::vec3& skyColor);
    
    // Wave sampling for physics/buoyancy
    float SampleHeight(float x, float z, float currentTime = -1.0f) const;
    glm::vec3 SampleNormal(float x, float z, float currentTime = -1.0f) const;
    glm::vec2 SampleDisplacement(float x, float z, float currentTime = -1.0f) const;
    
private:
    // Initialization helpers
    bool CreateGeometry();
    bool CreateTextures();
    bool CreateFramebuffers();
    bool InitializeShaders();
    
    // Wave spectrum generation
    void GenerateInitialSpectrum();
    float PhillipsSpectrum(const glm::vec2& k) const;
    float DispersionRelation(const glm::vec2& k) const;
    Complex GetGaussianRandom();
    
    // FFT computation
    void UpdateSpectrum(float currentTime);
    void ComputeFFT();
    void PerformFFTPass(GLuint inputTexture, GLuint outputTexture, 
                       const Shader& shader, bool horizontal);
    
    // Rendering helpers
    void SetupVertexData();
    void BindTextures();
    void SetShaderUniforms(const glm::mat4& view, const glm::mat4& projection,
                          const glm::vec3& cameraPos, const glm::vec3& lightDir,
                          const glm::vec3& lightColor, const glm::vec3& skyColor);
    
    // Utility functions
    glm::vec2 GetWaveVector(int n, int m) const;
    unsigned int ReverseBits(unsigned int value, int numBits) const;
    bool IsPowerOfTwo(int value) const;
};

// Factory for common ocean configurations
class OceanFFTFactory {
public:
    static OceanFFT::WaveParameters CreateCalmSea();
    static OceanFFT::WaveParameters CreateRoughSea();
    static OceanFFT::WaveParameters CreateStormySea();
    static OceanFFT::WaveParameters CreateTropicalWaves();
    
    static OceanFFT::OceanConfig CreateHighDetailConfig();
    static OceanFFT::OceanConfig CreateMediumDetailConfig();
    static OceanFFT::OceanConfig CreateLowDetailConfig();
    static OceanFFT::OceanConfig CreatePerformanceConfig();
};