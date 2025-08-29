#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "Shader.hpp"

// Cloud system based on "Computer Graphics Programming in OpenGL with C++"
// Implements skybox technique and procedural texturing from the book
class CloudsCG {
public:
    struct CloudParameters {
        float coverage = 0.6f;     // Cloud coverage amount
        float density = 0.8f;      // Cloud density
        float speed = 0.5f;        // Cloud movement speed
        float time = 0.0f;         // Animation time
        float altitude = 0.3f;     // Cloud layer altitude (0-1 in sky dome)
    };

private:
    // OpenGL objects for skybox (from book's skybox chapter)
    GLuint vao, vbo;
    std::unique_ptr<Shader> cloudShader;
    
    // Skybox cube vertices (from book)
    std::vector<float> skyboxVertices;
    
    // Cloud parameters
    CloudParameters params;
    glm::vec3 skyColor;
    glm::vec3 lightDirection;
    
    bool isInitialized;

public:
    CloudsCG();
    ~CloudsCG();
    
    // Initialization following book's skybox setup
    bool Initialize();
    void Cleanup();
    
    // Rendering with book's skybox technique
    void RenderSkybox(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    
    // Animation update
    void Update(float deltaTime);
    
    // Configuration
    void SetCloudParameters(const CloudParameters& cloudParams) { params = cloudParams; }
    void SetSkyColor(const glm::vec3& color) { skyColor = color; }
    void SetLightDirection(const glm::vec3& direction) { lightDirection = glm::normalize(direction); }
    
    // Getters
    const CloudParameters& GetParameters() const { return params; }
    bool IsInitialized() const { return isInitialized; }
    float GetCurrentTime() const { return params.time; }

private:
    // Skybox setup following book's method
    void CreateSkyboxGeometry();
    void SetupSkyboxVertices();
    void SetShaderUniforms(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
};

// Factory for creating cloud presets based on book's examples
class CloudsCGFactory {
public:
    // Weather presets from book's examples
    static CloudsCG::CloudParameters CreateClearSky();
    static CloudsCG::CloudParameters CreatePartlyCloudy();
    static CloudsCG::CloudParameters CreateOvercast();
    static CloudsCG::CloudParameters CreateStormyClouds();
    
    // Sky colors for different times of day
    static glm::vec3 GetDaySkyColor();
    static glm::vec3 GetSunsetSkyColor();  
    static glm::vec3 GetNightSkyColor();
};