#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "Shader.hpp"
#include "Material.hpp"

class Ocean {
public:
    // Ocean configuration
    struct OceanConfig {
        // Mesh properties
        int resolution = 256;           // Grid resolution (256x256)
        float size = 1000.0f;          // Ocean size in world units
        
        // Wave properties
        glm::vec2 windDirection = glm::vec2(1.0f, 0.5f);
        float windSpeed = 25.0f;
        float waveAmplitude = 2.0f;
        float waveFrequency = 0.02f;
        int numWaves = 6;
        
        // Visual properties
        glm::vec3 deepColor = glm::vec3(0.0f, 0.1f, 0.3f);
        glm::vec3 shallowColor = glm::vec3(0.1f, 0.6f, 0.8f);
        float fresnelStrength = 2.0f;
        float specularStrength = 100.0f;
        float roughness = 0.02f;
        float transparency = 0.8f;
        float refractionStrength = 0.1f;
    };

private:
    // OpenGL resources
    GLuint VAO, VBO, EBO;
    std::unique_ptr<Shader> oceanShader;
    std::unique_ptr<Material> oceanMaterial;
    
    // Mesh data
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;
    
    // Ocean properties
    OceanConfig config;
    float time;
    bool isInitialized;
    
    // Textures
    GLuint dudvTexture;
    GLuint normalTexture;
    GLuint skyboxTexture;

public:
    Ocean();
    ~Ocean();
    
    // Initialization and cleanup
    bool Initialize(const OceanConfig& cfg = OceanConfig{});
    void Cleanup();
    
    // Rendering
    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::vec3& viewPos, const glm::vec3& lightDir, 
                const glm::vec3& lightColor, const glm::vec3& skyColor);
    
    // Configuration
    void SetConfig(const OceanConfig& cfg);
    const OceanConfig& GetConfig() const { return config; }
    
    // Wave height sampling (useful for floating objects)
    float SampleWaveHeight(float x, float z, float currentTime = -1.0f) const;
    glm::vec3 SampleWaveNormal(float x, float z, float currentTime = -1.0f) const;
    
    // Utility functions
    bool IsInitialized() const { return isInitialized; }
    float GetCurrentTime() const { return time; }

private:
    // Mesh generation
    void GenerateMesh();
    void GenerateIndices();
    void CalculateNormals();
    void UpdateMeshData();
    
    // OpenGL setup
    void SetupVertexData();
    void CreateTextures();
    void SetupShaderUniforms();
    
    // Wave calculation (CPU-side for physics queries)
    struct Wave {
        glm::vec2 direction;
        float amplitude;
        float frequency;
        float phase;
        float steepness;
    };
    
    std::vector<Wave> GenerateWaveSet() const;
    glm::vec3 CalculateGerstnerWave(const glm::vec2& position, const Wave& wave, 
                                   float currentTime, glm::vec3& tangent, 
                                   glm::vec3& binormal) const;
};

// Ocean factory for creating different ocean types
class OceanFactory {
public:
    // Preset configurations
    static Ocean::OceanConfig CreateCalmOcean();
    static Ocean::OceanConfig CreateRoughSea();
    static Ocean::OceanConfig CreateStormyOcean();
    static Ocean::OceanConfig CreateTropicalOcean();
    static Ocean::OceanConfig CreateArcticOcean();
    
    // Utility for creating ocean with specific conditions
    static Ocean::OceanConfig CreateCustomOcean(float windSpeed, 
                                               const glm::vec2& windDir,
                                               float waveHeight,
                                               const glm::vec3& waterColor);
};