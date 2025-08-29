#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "Shader.hpp"

// Ocean implementation based on "Computer Graphics Programming in OpenGL with C++"
// Follows the book's approach to procedural surfaces and displacement mapping
class OceanCG {
public:
    struct LightInfo {
        glm::vec4 ambient;
        glm::vec4 diffuse; 
        glm::vec4 specular;
        glm::vec3 position;
    };
    
    struct MaterialInfo {
        glm::vec4 ambient;
        glm::vec4 diffuse;
        glm::vec4 specular;
        float shininess;
    };
    
    struct WaveParameters {
        float height = 0.5f;      // Wave amplitude
        float length = 8.0f;      // Wave length
        float speed = 2.0f;       // Wave speed
        float time = 0.0f;        // Animation time
    };

private:
    // OpenGL objects following book's patterns
    GLuint vao, vbo, ebo;
    std::unique_ptr<Shader> oceanShader;
    
    // Mesh data (grid-based as shown in book)
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    int gridResolution;
    float gridSize;
    
    // Rendering state
    WaveParameters waves;
    LightInfo light;
    MaterialInfo material;
    glm::vec4 globalAmbient;
    
    // Ocean appearance
    glm::vec3 deepColor;
    glm::vec3 shallowColor;
    float fresnelPower;
    
    bool isInitialized;

public:
    OceanCG();
    ~OceanCG();
    
    // Initialization following book's setup patterns
    bool Initialize(int resolution = 100, float size = 100.0f);
    void Cleanup();
    
    // Rendering with book's matrix approach
    void Render(const glm::mat4& mvMatrix, const glm::mat4& projMatrix);
    
    // Animation update
    void Update(float deltaTime);
    
    // Configuration methods from book's material system
    void SetLighting(const LightInfo& lightInfo);
    void SetMaterial(const MaterialInfo& matInfo);
    void SetGlobalAmbient(const glm::vec4& ambient) { globalAmbient = ambient; }
    void SetWaveParameters(const WaveParameters& params) { waves = params; }
    void SetOceanColors(const glm::vec3& deep, const glm::vec3& shallow);
    void SetFresnelPower(float power) { fresnelPower = power; }
    
    // Utility
    bool IsInitialized() const { return isInitialized; }
    float GetCurrentTime() const { return waves.time; }

private:
    // Mesh generation following book's grid creation
    void CreateOceanGrid();
    void SetupVertexAttributes();
    void SetShaderUniforms(const glm::mat4& mvMatrix, const glm::mat4& projMatrix);
    
    // Book's normal matrix calculation
    glm::mat4 CalculateNormalMatrix(const glm::mat4& mvMatrix);
};

// Factory for creating ocean presets following book's examples
class OceanCGFactory {
public:
    static OceanCG::WaveParameters CreateCalmWaves();
    static OceanCG::WaveParameters CreateRoughWaves();
    static OceanCG::WaveParameters CreateStormyWaves();
    
    static OceanCG::LightInfo CreateSunlight();
    static OceanCG::LightInfo CreateMoonlight();
    
    static OceanCG::MaterialInfo CreateWaterMaterial();
};