#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include "Shader.hpp"

class CloudSystem {
public:
    // Cloud configuration
    struct CloudConfig {
        // Volume properties
        float cloudHeight = 1500.0f;       // Base height of cloud layer
        float cloudThickness = 800.0f;     // Thickness of cloud layer
        glm::vec3 volumeSize = glm::vec3(2000.0f, 800.0f, 2000.0f); // Cloud volume bounds
        
        // Cloud appearance
        float cloudCoverage = 0.45f;       // How much of sky is covered (0-1)
        float cloudDensity = 1.0f;         // Overall cloud density
        float cloudScale = 0.0008f;        // Scale of cloud formations
        float cloudSpeed = 2.0f;           // Speed of cloud movement
        glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.5f); // Wind direction
        
        // Noise parameters
        float noiseScale = 1.0f;           // Primary noise scale
        float noiseStrength = 0.4f;        // Detail noise contribution
        int octaves = 4;                   // Number of noise octaves
        
        // Rendering parameters
        int numSteps = 64;                 // Ray marching steps
        int numLightSteps = 6;             // Light sampling steps
        float maxDistance = 3000.0f;       // Maximum ray march distance
        
        // Performance options
        bool enableDetailNoise = true;     // Use detail noise for cloud structure
        bool enableLightScattering = true; // Enable multiple scattering
        float lodDistance = 1000.0f;       // Distance for level-of-detail switching
    };
    
    // Weather system for dynamic clouds
    struct WeatherData {
        float humidity = 0.6f;             // Affects cloud formation
        float temperature = 20.0f;         // Temperature in Celsius
        float pressure = 1013.25f;         // Atmospheric pressure
        glm::vec3 windVelocity = glm::vec3(5.0f, 0.0f, 2.0f); // 3D wind
        float turbulence = 0.3f;           // Atmospheric turbulence
        float precipitation = 0.0f;        // Rain/snow intensity
    };

private:
    // OpenGL resources
    GLuint skyboxVAO, skyboxVBO;
    std::unique_ptr<Shader> cloudShader;
    
    // Cloud volume representation (using billboards/skybox)
    std::vector<glm::vec3> skyboxVertices;
    
    // Cloud properties
    CloudConfig config;
    WeatherData weather;
    float time;
    bool isInitialized;
    
    // Noise textures for cloud generation
    GLuint noise3DTexture;
    GLuint worleyTexture;
    GLuint weatherTexture;

public:
    CloudSystem();
    ~CloudSystem();
    
    // Initialization and cleanup
    bool Initialize(const CloudConfig& cfg = CloudConfig{});
    void Cleanup();
    
    // Rendering
    void Update(float deltaTime);
    void Render(const glm::mat4& view, const glm::mat4& projection, 
                const glm::vec3& viewPos, const glm::vec3& lightDir, 
                const glm::vec3& lightColor, const glm::vec3& skyColor);
    
    // Configuration
    void SetConfig(const CloudConfig& cfg);
    const CloudConfig& GetConfig() const { return config; }
    
    void SetWeather(const WeatherData& weatherData);
    const WeatherData& GetWeather() const { return weather; }
    
    // Cloud density sampling (for gameplay/physics integration)
    float SampleCloudDensity(const glm::vec3& worldPos) const;
    glm::vec3 SampleCloudVelocity(const glm::vec3& worldPos) const;
    
    // Weather effects
    void SetCloudiness(float cloudiness); // 0 = clear sky, 1 = overcast
    void SetWeatherCondition(const std::string& condition); // "clear", "partly_cloudy", "overcast", "stormy"
    void AnimateWeatherTransition(const std::string& fromCondition, const std::string& toCondition, float duration);
    
    // Utility functions
    bool IsInitialized() const { return isInitialized; }
    float GetCurrentTime() const { return time; }
    bool IsPointInClouds(const glm::vec3& worldPos) const;

private:
    // Mesh and rendering setup
    void CreateSkyboxMesh();
    void SetupVertexData();
    void CreateNoiseTextures();
    
    // Noise generation
    void Generate3DNoise(int size);
    void GenerateWorleyNoise(int size);
    void GenerateWeatherTexture(int size);
    
    // Utility functions
    float Hash(float n) const;
    float Noise3D(const glm::vec3& pos) const;
    float FractionalBrownianMotion(const glm::vec3& pos, int octaves) const;
    
    // Weather interpolation
    CloudConfig InterpolateWeatherConfigs(const CloudConfig& from, const CloudConfig& to, float t) const;
};

// Cloud factory for creating different cloud types and weather conditions
class CloudFactory {
public:
    // Preset configurations
    static CloudSystem::CloudConfig CreateClearSky();
    static CloudSystem::CloudConfig CreatePartlyCloudy();
    static CloudSystem::CloudConfig CreateOvercast();
    static CloudSystem::CloudConfig CreateStormyClouds();
    static CloudSystem::CloudConfig CreateHighAltitudeClouds();
    static CloudSystem::CloudConfig CreateCumulusClouds();
    static CloudSystem::CloudConfig CreateStratusClouds();
    static CloudSystem::CloudConfig CreateCirrusClouds();
    
    // Weather conditions
    static CloudSystem::WeatherData CreateClearWeather();
    static CloudSystem::WeatherData CreateRainyWeather();
    static CloudSystem::WeatherData CreateStormyWeather();
    static CloudSystem::WeatherData CreateWindyWeather();
    
    // Utility for creating custom weather
    static CloudSystem::CloudConfig CreateCustomClouds(float coverage, float density, 
                                                       float height, const glm::vec3& windDir);
};

// Weather transition system for smooth weather changes
class WeatherTransition {
private:
    CloudSystem::CloudConfig startConfig;
    CloudSystem::CloudConfig targetConfig;
    float duration;
    float currentTime;
    bool isActive;

public:
    WeatherTransition();
    
    void StartTransition(const CloudSystem::CloudConfig& from, 
                        const CloudSystem::CloudConfig& to, float transitionDuration);
    bool Update(float deltaTime, CloudSystem::CloudConfig& outConfig);
    
    bool IsActive() const { return isActive; }
    float GetProgress() const { return isActive ? (currentTime / duration) : 1.0f; }
};