#include "CloudSystem.hpp"
#include <iostream>
#include <cmath>
#include <random>

// Helper functions for math operations
float fract(float x) {
    return x - floor(x);
}

glm::vec3 fract(const glm::vec3& v) {
    return glm::vec3(fract(v.x), fract(v.y), fract(v.z));
}

CloudSystem::CloudSystem() 
    : skyboxVAO(0), skyboxVBO(0), time(0.0f), isInitialized(false),
      noise3DTexture(0), worleyTexture(0), weatherTexture(0) {
}

CloudSystem::~CloudSystem() {
    Cleanup();
}

bool CloudSystem::Initialize(const CloudConfig& cfg) {
    if (isInitialized) {
        Cleanup();
    }
    
    config = cfg;
    
    std::cout << "Initializing Cloud System..." << std::endl;
    std::cout << "- Cloud Height: " << config.cloudHeight << "m" << std::endl;
    std::cout << "- Cloud Coverage: " << (config.cloudCoverage * 100.0f) << "%" << std::endl;
    std::cout << "- Ray March Steps: " << config.numSteps << std::endl;
    
    // Initialize shader with simpler version to avoid flickering
    cloudShader = std::make_unique<Shader>();
    if (!cloudShader->InitFromFiles("shaders/clouds_simple.vert", "shaders/clouds_simple.frag")) {
        std::cerr << "Failed to initialize cloud shader!" << std::endl;
        return false;
    }
    
    // Create skybox mesh for cloud rendering
    CreateSkyboxMesh();
    SetupVertexData();
    CreateNoiseTextures();
    
    isInitialized = true;
    std::cout << "Cloud system initialized successfully!" << std::endl;
    
    return true;
}

void CloudSystem::Cleanup() {
    if (skyboxVAO != 0) {
        glDeleteVertexArrays(1, &skyboxVAO);
        skyboxVAO = 0;
    }
    if (skyboxVBO != 0) {
        glDeleteBuffers(1, &skyboxVBO);
        skyboxVBO = 0;
    }
    
    if (noise3DTexture != 0) {
        glDeleteTextures(1, &noise3DTexture);
        noise3DTexture = 0;
    }
    if (worleyTexture != 0) {
        glDeleteTextures(1, &worleyTexture);
        worleyTexture = 0;
    }
    if (weatherTexture != 0) {
        glDeleteTextures(1, &weatherTexture);
        weatherTexture = 0;
    }
    
    cloudShader.reset();
    isInitialized = false;
}

void CloudSystem::Update(float deltaTime) {
    if (!isInitialized) return;
    time += deltaTime;
    
    // Update weather effects
    weather.windVelocity += glm::vec3(sin(time * 0.1f), 0.0f, cos(time * 0.15f)) * 0.1f;
    weather.turbulence = 0.3f + 0.2f * sin(time * 0.05f);
}

void CloudSystem::Render(const glm::mat4& view, const glm::mat4& projection, 
                         const glm::vec3& viewPos, const glm::vec3& lightDir, 
                         const glm::vec3& lightColor, const glm::vec3& skyColor) {
    if (!isInitialized) return;
    
    // Enable blending for transparent clouds
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth writing but keep depth testing for proper sorting
    glDepthMask(GL_FALSE);
    
    // Use cloud shader
    cloudShader->use();
    
    // Set matrices (remove translation from view matrix for skybox effect)
    glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));
    glm::mat4 model = glm::mat4(1.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(cloudShader->shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(cloudShader->shaderProgram, "view"), 1, GL_FALSE, &viewNoTranslation[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(cloudShader->shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Set time and cloud parameters
    glUniform1f(glGetUniformLocation(cloudShader->shaderProgram, "u_time"), time);
    glUniform3fv(glGetUniformLocation(cloudShader->shaderProgram, "viewPos"), 1, &viewPos[0]);
    glUniform3fv(glGetUniformLocation(cloudShader->shaderProgram, "lightDirection"), 1, &lightDir[0]);
    glUniform3fv(glGetUniformLocation(cloudShader->shaderProgram, "lightColor"), 1, &lightColor[0]);
    glUniform3fv(glGetUniformLocation(cloudShader->shaderProgram, "skyColor"), 1, &skyColor[0]);
    
    // Set simplified cloud properties for simple shader
    glUniform1f(glGetUniformLocation(cloudShader->shaderProgram, "u_cloudCoverage"), config.cloudCoverage);
    glUniform1f(glGetUniformLocation(cloudShader->shaderProgram, "u_cloudHeight"), config.cloudHeight);
    glUniform1f(glGetUniformLocation(cloudShader->shaderProgram, "u_cloudThickness"), config.cloudThickness);
    
    // Skip texture binding for now - use procedural noise in shaders
    // This prevents flickering from missing/invalid textures
    
    // Render skybox mesh
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(skyboxVertices.size()));
    glBindVertexArray(0);
    
    // Restore render state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void CloudSystem::SetConfig(const CloudConfig& cfg) {
    config = cfg;
}

void CloudSystem::SetWeather(const WeatherData& weatherData) {
    weather = weatherData;
    
    // Adjust cloud parameters based on weather
    config.cloudCoverage = glm::clamp(weather.humidity, 0.0f, 1.0f);
    config.windDirection = glm::normalize(weather.windVelocity);
    config.cloudSpeed = glm::length(weather.windVelocity) * 0.1f;
}

float CloudSystem::SampleCloudDensity(const glm::vec3& worldPos) const {
    // Simplified CPU-side cloud density sampling
    if (worldPos.y < config.cloudHeight - config.cloudThickness * 0.5f ||
        worldPos.y > config.cloudHeight + config.cloudThickness * 0.5f) {
        return 0.0f;
    }
    
    glm::vec3 samplePos = worldPos * config.cloudScale + config.windDirection * time * config.cloudSpeed;
    float noise = FractionalBrownianMotion(samplePos, config.octaves);
    
    return glm::clamp(noise - (1.0f - config.cloudCoverage), 0.0f, 1.0f) * config.cloudDensity;
}

glm::vec3 CloudSystem::SampleCloudVelocity(const glm::vec3& worldPos) const {
    return weather.windVelocity + glm::vec3(
        sin(worldPos.x * 0.01f + time * 0.1f) * weather.turbulence,
        cos(worldPos.y * 0.01f + time * 0.15f) * weather.turbulence * 0.5f,
        sin(worldPos.z * 0.01f + time * 0.12f) * weather.turbulence
    );
}

void CloudSystem::SetCloudiness(float cloudiness) {
    config.cloudCoverage = glm::clamp(cloudiness, 0.0f, 1.0f);
    config.cloudDensity = 0.5f + cloudiness * 0.5f;
}

void CloudSystem::SetWeatherCondition(const std::string& condition) {
    if (condition == "clear") {
        config = CloudFactory::CreateClearSky();
        weather = CloudFactory::CreateClearWeather();
    } else if (condition == "partly_cloudy") {
        config = CloudFactory::CreatePartlyCloudy();
        weather = CloudFactory::CreateClearWeather();
    } else if (condition == "overcast") {
        config = CloudFactory::CreateOvercast();
        weather = CloudFactory::CreateWindyWeather();
    } else if (condition == "stormy") {
        config = CloudFactory::CreateStormyClouds();
        weather = CloudFactory::CreateStormyWeather();
    }
}

bool CloudSystem::IsPointInClouds(const glm::vec3& worldPos) const {
    return SampleCloudDensity(worldPos) > 0.1f;
}

void CloudSystem::CreateSkyboxMesh() {
    // Create skybox cube vertices
    skyboxVertices = {
        // Far face
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        
        // Near face
        {-1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},
        
        // Left face
        {-1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f, -1.0f},
        {-1.0f, -1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        
        // Right face
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        
        // Bottom face
        {-1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f, -1.0f},
        { 1.0f, -1.0f,  1.0f},
        { 1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f,  1.0f},
        {-1.0f, -1.0f, -1.0f},
        
        // Top face
        {-1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f, -1.0f},
        { 1.0f,  1.0f,  1.0f},
        { 1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f,  1.0f},
        {-1.0f,  1.0f, -1.0f}
    };
}

void CloudSystem::SetupVertexData() {
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    
    glBufferData(GL_ARRAY_BUFFER, skyboxVertices.size() * sizeof(glm::vec3), 
                 skyboxVertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    std::cout << "Cloud system vertex data setup complete" << std::endl;
}

void CloudSystem::CreateNoiseTextures() {
    std::cout << "Creating cloud noise textures..." << std::endl;
    
    // Note: 3D texture creation requires significant memory and processing
    // For now, we'll create placeholder textures or skip them
    // In a production system, you'd want to pre-generate these or load them from files
    
    Generate3DNoise(64);      // Smaller size for performance
    GenerateWorleyNoise(32);  // Even smaller for detail noise
    GenerateWeatherTexture(256); // 2D texture for weather patterns
    
    std::cout << "Cloud noise textures created" << std::endl;
}

void CloudSystem::Generate3DNoise(int size) {
    // Create simple 3D noise texture
    std::vector<unsigned char> noiseData(size * size * size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    for (int i = 0; i < size * size * size; i++) {
        float noise = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        
        // Simple FBM
        for (int octave = 0; octave < 4; octave++) {
            noise += amplitude * dis(gen);
            amplitude *= 0.5f;
            frequency *= 2.0f;
        }
        
        noiseData[i] = static_cast<unsigned char>(glm::clamp(noise, 0.0f, 1.0f) * 255);
    }
    
    glGenTextures(1, &noise3DTexture);
    glBindTexture(GL_TEXTURE_3D, noise3DTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, size, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, noiseData.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
}

void CloudSystem::GenerateWorleyNoise(int size) {
    // Create Worley noise texture for cloud detail
    std::vector<unsigned char> worleyData(size * size * size);
    
    // Simplified Worley noise generation
    for (int z = 0; z < size; z++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                float minDist = 10.0f;
                
                // Check surrounding cells for feature points
                for (int oz = -1; oz <= 1; oz++) {
                    for (int oy = -1; oy <= 1; oy++) {
                        for (int ox = -1; ox <= 1; ox++) {
                            glm::vec3 cellPos = glm::vec3(x + ox, y + oy, z + oz);
                            float hash = Hash(cellPos.x + cellPos.y * 57.0f + cellPos.z * 113.0f);
                            glm::vec3 featurePoint = cellPos + glm::vec3(hash, Hash(hash * 2.0f), Hash(hash * 3.0f));
                            
                            float dist = glm::distance(glm::vec3(x, y, z), featurePoint);
                            minDist = glm::min(minDist, dist);
                        }
                    }
                }
                
                int index = x + y * size + z * size * size;
                worleyData[index] = static_cast<unsigned char>((1.0f - minDist / size) * 255);
            }
        }
    }
    
    glGenTextures(1, &worleyTexture);
    glBindTexture(GL_TEXTURE_3D, worleyTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, size, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, worleyData.data());
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
}

void CloudSystem::GenerateWeatherTexture(int size) {
    // Create 2D weather texture for large-scale cloud patterns
    std::vector<unsigned char> weatherData(size * size * 3);
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = (y * size + x) * 3;
            
            float fx = static_cast<float>(x) / size;
            float fy = static_cast<float>(y) / size;
            
            // Create weather patterns
            float coverage = FractionalBrownianMotion(glm::vec3(fx * 4.0f, fy * 4.0f, 0.0f), 3);
            float cloudType = FractionalBrownianMotion(glm::vec3(fx * 2.0f, fy * 2.0f, 1.0f), 2);
            float density = FractionalBrownianMotion(glm::vec3(fx * 8.0f, fy * 8.0f, 2.0f), 4);
            
            weatherData[index] = static_cast<unsigned char>(glm::clamp(coverage, 0.0f, 1.0f) * 255);
            weatherData[index + 1] = static_cast<unsigned char>(glm::clamp(cloudType, 0.0f, 1.0f) * 255);
            weatherData[index + 2] = static_cast<unsigned char>(glm::clamp(density, 0.0f, 1.0f) * 255);
        }
    }
    
    glGenTextures(1, &weatherTexture);
    glBindTexture(GL_TEXTURE_2D, weatherTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, weatherData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

float CloudSystem::Hash(float n) const {
    return fract(sin(n) * 43758.5453f);
}

float CloudSystem::Noise3D(const glm::vec3& pos) const {
    glm::vec3 p = glm::floor(pos);
    glm::vec3 f = glm::fract(pos);
    f = f * f * (3.0f - 2.0f * f);
    
    float n = p.x + p.y * 57.0f + 113.0f * p.z;
    return glm::mix(
        glm::mix(glm::mix(Hash(n + 0.0f), Hash(n + 1.0f), f.x),
                glm::mix(Hash(n + 57.0f), Hash(n + 58.0f), f.x), f.y),
        glm::mix(glm::mix(Hash(n + 113.0f), Hash(n + 114.0f), f.x),
                glm::mix(Hash(n + 170.0f), Hash(n + 171.0f), f.x), f.y), f.z);
}

float CloudSystem::FractionalBrownianMotion(const glm::vec3& pos, int octaves) const {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    
    for (int i = 0; i < octaves; i++) {
        value += amplitude * Noise3D(pos * frequency);
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    return value;
}

// Helper function implementations moved to top of file

// Cloud Factory Implementation
CloudSystem::CloudConfig CloudFactory::CreateClearSky() {
    CloudSystem::CloudConfig config;
    config.cloudCoverage = 0.1f;
    config.cloudDensity = 0.3f;
    config.numSteps = 32; // Fewer steps for performance
    return config;
}

CloudSystem::CloudConfig CloudFactory::CreatePartlyCloudy() {
    CloudSystem::CloudConfig config;
    config.cloudCoverage = 0.4f;
    config.cloudDensity = 0.6f;
    config.cloudScale = 0.001f;
    return config;
}

CloudSystem::CloudConfig CloudFactory::CreateOvercast() {
    CloudSystem::CloudConfig config;
    config.cloudCoverage = 0.8f;
    config.cloudDensity = 0.9f;
    config.cloudThickness = 1200.0f;
    config.numSteps = 80; // More steps for dense clouds
    return config;
}

CloudSystem::CloudConfig CloudFactory::CreateStormyClouds() {
    CloudSystem::CloudConfig config;
    config.cloudCoverage = 0.9f;
    config.cloudDensity = 1.2f;
    config.cloudThickness = 1500.0f;
    config.cloudSpeed = 8.0f;
    config.windDirection = glm::vec3(1.0f, 0.2f, 0.5f);
    config.numSteps = 96; // High quality for dramatic storms
    return config;
}

CloudSystem::CloudConfig CloudFactory::CreateHighAltitudeClouds() {
    CloudSystem::CloudConfig config;
    config.cloudHeight = 8000.0f;
    config.cloudThickness = 400.0f;
    config.cloudCoverage = 0.3f;
    config.cloudDensity = 0.4f;
    config.cloudScale = 0.0005f;
    return config;
}

CloudSystem::WeatherData CloudFactory::CreateClearWeather() {
    CloudSystem::WeatherData weather;
    weather.humidity = 0.3f;
    weather.temperature = 25.0f;
    weather.windVelocity = glm::vec3(3.0f, 0.0f, 1.0f);
    weather.turbulence = 0.1f;
    return weather;
}

CloudSystem::WeatherData CloudFactory::CreateRainyWeather() {
    CloudSystem::WeatherData weather;
    weather.humidity = 0.9f;
    weather.temperature = 15.0f;
    weather.windVelocity = glm::vec3(8.0f, -1.0f, 3.0f);
    weather.turbulence = 0.6f;
    weather.precipitation = 0.7f;
    return weather;
}

CloudSystem::WeatherData CloudFactory::CreateStormyWeather() {
    CloudSystem::WeatherData weather;
    weather.humidity = 0.95f;
    weather.temperature = 12.0f;
    weather.pressure = 990.0f; // Low pressure
    weather.windVelocity = glm::vec3(15.0f, -2.0f, 8.0f);
    weather.turbulence = 0.9f;
    weather.precipitation = 0.9f;
    return weather;
}

CloudSystem::WeatherData CloudFactory::CreateWindyWeather() {
    CloudSystem::WeatherData weather;
    weather.humidity = 0.5f;
    weather.temperature = 18.0f;
    weather.windVelocity = glm::vec3(12.0f, 1.0f, 6.0f);
    weather.turbulence = 0.7f;
    return weather;
}

// Weather Transition Implementation
WeatherTransition::WeatherTransition() : duration(0.0f), currentTime(0.0f), isActive(false) {}

void WeatherTransition::StartTransition(const CloudSystem::CloudConfig& from, 
                                       const CloudSystem::CloudConfig& to, 
                                       float transitionDuration) {
    startConfig = from;
    targetConfig = to;
    duration = transitionDuration;
    currentTime = 0.0f;
    isActive = true;
}

bool WeatherTransition::Update(float deltaTime, CloudSystem::CloudConfig& outConfig) {
    if (!isActive) return false;
    
    currentTime += deltaTime;
    float t = glm::clamp(currentTime / duration, 0.0f, 1.0f);
    
    // Smooth interpolation using smoothstep
    float smoothT = t * t * (3.0f - 2.0f * t);
    
    // Interpolate all config parameters
    outConfig.cloudCoverage = glm::mix(startConfig.cloudCoverage, targetConfig.cloudCoverage, smoothT);
    outConfig.cloudDensity = glm::mix(startConfig.cloudDensity, targetConfig.cloudDensity, smoothT);
    outConfig.cloudHeight = glm::mix(startConfig.cloudHeight, targetConfig.cloudHeight, smoothT);
    outConfig.cloudThickness = glm::mix(startConfig.cloudThickness, targetConfig.cloudThickness, smoothT);
    outConfig.cloudScale = glm::mix(startConfig.cloudScale, targetConfig.cloudScale, smoothT);
    outConfig.cloudSpeed = glm::mix(startConfig.cloudSpeed, targetConfig.cloudSpeed, smoothT);
    outConfig.windDirection = glm::mix(startConfig.windDirection, targetConfig.windDirection, smoothT);
    
    if (currentTime >= duration) {
        isActive = false;
        outConfig = targetConfig;
        return false; // Transition complete
    }
    
    return true; // Transition ongoing
}