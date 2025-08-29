#include "OceanFFT.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

const float PI = 3.14159265359f;
const float GRAVITY = 9.81f;

OceanFFT::OceanFFT() 
    : VAO(0), VBO(0), EBO(0), heightmapTexture(0), displacementTexture(0),
      normalTexture(0), foamTexture(0), spectrumTexture(0), phaseTexture(0),
      framebuffer(0), time(0.0f), isInitialized(false), rng(42), gaussianDist(0.0f, 1.0f) {
}

OceanFFT::~OceanFFT() {
    Cleanup();
}

bool OceanFFT::Initialize(const OceanConfig& cfg) {
    if (isInitialized) {
        Cleanup();
    }
    
    config = cfg;
    
    // Validate configuration
    if (!IsPowerOfTwo(config.N)) {
        std::cerr << "Ocean FFT: Grid resolution must be a power of 2!" << std::endl;
        return false;
    }
    
    std::cout << "Initializing FFT Ocean System..." << std::endl;
    std::cout << "- Resolution: " << config.N << "x" << config.N << std::endl;
    std::cout << "- Ocean Size: " << config.oceanSize << " meters" << std::endl;
    std::cout << "- Choppiness: " << (config.enableChoppiness ? "Enabled" : "Disabled") << std::endl;
    std::cout << "- Foam: " << (config.enableFoam ? "Enabled" : "Disabled") << std::endl;
    
    // Initialize components
    if (!CreateGeometry()) return false;
    if (!CreateTextures()) return false;
    if (!CreateFramebuffers()) return false;
    if (!InitializeShaders()) return false;
    
    // Generate initial wave spectrum
    GenerateInitialSpectrum();
    
    isInitialized = true;
    std::cout << "FFT Ocean system initialized successfully!" << std::endl;
    
    return true;
}

void OceanFFT::Cleanup() {
    // Clean up textures
    if (heightmapTexture != 0) glDeleteTextures(1, &heightmapTexture);
    if (displacementTexture != 0) glDeleteTextures(1, &displacementTexture);
    if (normalTexture != 0) glDeleteTextures(1, &normalTexture);
    if (foamTexture != 0) glDeleteTextures(1, &foamTexture);
    if (spectrumTexture != 0) glDeleteTextures(1, &spectrumTexture);
    if (phaseTexture != 0) glDeleteTextures(1, &phaseTexture);
    
    // Clean up framebuffer
    if (framebuffer != 0) glDeleteFramebuffers(1, &framebuffer);
    
    // Clean up geometry
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
    
    // Reset shaders
    spectrumShader.reset();
    fftHorizontalShader.reset();
    fftVerticalShader.reset();
    combineMapsShader.reset();
    oceanVertexShader.reset();
    oceanFragmentShader.reset();
    
    isInitialized = false;
}

void OceanFFT::SetWaveParameters(const WaveParameters& params) {
    waveParams = params;
    if (isInitialized) {
        GenerateInitialSpectrum(); // Regenerate spectrum with new parameters
    }
}

void OceanFFT::SetOceanConfig(const OceanConfig& cfg) {
    config = cfg;
    if (isInitialized) {
        // Reinitialize if resolution changed
        if (cfg.N != config.N) {
            Initialize(cfg);
        }
    }
}

void OceanFFT::Update(float deltaTime) {
    if (!isInitialized) return;
    
    time += deltaTime * config.timeScale;
    
    // Update wave spectrum for current time
    UpdateSpectrum(time);
    
    // Perform FFT computation on GPU
    ComputeFFT();
}

void OceanFFT::Render(const glm::mat4& view, const glm::mat4& projection, 
                      const glm::vec3& cameraPos, const glm::vec3& lightDir,
                      const glm::vec3& lightColor, const glm::vec3& skyColor) {
    if (!isInitialized) return;
    
    // Enable transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    
    // Use ocean shaders
    oceanVertexShader->use();
    
    // Set uniforms
    SetShaderUniforms(view, projection, cameraPos, lightDir, lightColor, skyColor);
    
    // Bind textures
    BindTextures();
    
    // Render ocean mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

float OceanFFT::SampleHeight(float x, float z, float currentTime) const {
    if (!isInitialized) return 0.0f;
    
    if (currentTime < 0.0f) currentTime = time;
    
    // Convert world coordinates to grid coordinates
    float u = (x / config.oceanSize + 0.5f) * config.N;
    float v = (z / config.oceanSize + 0.5f) * config.N;
    
    // Bilinear interpolation would be implemented here
    // For now, return a simple wave approximation
    float waveHeight = 0.0f;
    
    for (int i = 0; i < 8; i++) {
        float freq = 0.01f * (1 << i);
        float amp = waveParams.A / (1 << i);
        waveHeight += amp * sin(x * freq + currentTime) * cos(z * freq + currentTime * 0.7f);
    }
    
    return waveHeight;
}

glm::vec3 OceanFFT::SampleNormal(float x, float z, float currentTime) const {
    if (!isInitialized) return glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Calculate normal using finite differences
    const float epsilon = 0.1f;
    float h0 = SampleHeight(x, z, currentTime);
    float hx = SampleHeight(x + epsilon, z, currentTime);
    float hz = SampleHeight(x, z + epsilon, currentTime);
    
    glm::vec3 tangentX = glm::normalize(glm::vec3(epsilon, hx - h0, 0.0f));
    glm::vec3 tangentZ = glm::normalize(glm::vec3(0.0f, hz - h0, epsilon));
    
    return glm::normalize(glm::cross(tangentZ, tangentX));
}

glm::vec2 OceanFFT::SampleDisplacement(float x, float z, float currentTime) const {
    if (!isInitialized || !config.enableChoppiness) return glm::vec2(0.0f);
    
    // Simplified displacement calculation
    float dispX = waveParams.lambda * 0.1f * cos(x * 0.01f + currentTime);
    float dispZ = waveParams.lambda * 0.1f * sin(z * 0.01f + currentTime * 0.8f);
    
    return glm::vec2(dispX, dispZ);
}

bool OceanFFT::CreateGeometry() {
    vertices.clear();
    indices.clear();
    
    // Create ocean grid
    float halfSize = config.oceanSize * 0.5f;
    float step = config.oceanSize / (config.N - 1);
    
    // Generate vertices (position + texture coordinates)
    for (int z = 0; z < config.N; z++) {
        for (int x = 0; x < config.N; x++) {
            float worldX = -halfSize + x * step;
            float worldZ = -halfSize + z * step;
            
            vertices.push_back(worldX);     // x
            vertices.push_back(0.0f);       // y (height will be set by vertex shader)
            vertices.push_back(worldZ);     // z
            vertices.push_back(static_cast<float>(x) / (config.N - 1)); // u
            vertices.push_back(static_cast<float>(z) / (config.N - 1)); // v
        }
    }
    
    // Generate indices
    for (int z = 0; z < config.N - 1; z++) {
        for (int x = 0; x < config.N - 1; x++) {
            int topLeft = z * config.N + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * config.N + x;
            int bottomRight = bottomLeft + 1;
            
            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    SetupVertexData();
    
    std::cout << "Created FFT ocean grid: " << vertices.size()/5 << " vertices, " 
              << indices.size()/3 << " triangles" << std::endl;
    
    return true;
}

bool OceanFFT::CreateTextures() {
    // Create heightmap texture
    glGenTextures(1, &heightmapTexture);
    glBindTexture(GL_TEXTURE_2D, heightmapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, config.N, config.N, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Create displacement texture
    if (config.enableChoppiness) {
        glGenTextures(1, &displacementTexture);
        glBindTexture(GL_TEXTURE_2D, displacementTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, config.N, config.N, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    
    // Create normal texture
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, config.N, config.N, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Create foam texture
    if (config.enableFoam) {
        glGenTextures(1, &foamTexture);
        glBindTexture(GL_TEXTURE_2D, foamTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, config.N, config.N, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    
    // Create spectrum texture
    glGenTextures(1, &spectrumTexture);
    glBindTexture(GL_TEXTURE_2D, spectrumTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, config.N, config.N, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return true;
}

bool OceanFFT::CreateFramebuffers() {
    glGenFramebuffers(1, &framebuffer);
    return true;
}

bool OceanFFT::InitializeShaders() {
    // For now, create basic shaders - compute shaders will be added in next step
    oceanVertexShader = std::make_unique<Shader>();
    if (!oceanVertexShader->InitFromFiles("shaders/ocean_fft.vert", "shaders/ocean_fft.frag")) {
        std::cerr << "Failed to initialize FFT ocean shaders!" << std::endl;
        return false;
    }
    
    return true;
}

void OceanFFT::GenerateInitialSpectrum() {
    initialSpectrum.resize(config.N * config.N);
    
    for (int m = 0; m < config.N; m++) {
        for (int n = 0; n < config.N; n++) {
            glm::vec2 k = GetWaveVector(n, m);
            
            if (glm::length(k) < 0.000001f) {
                initialSpectrum[m * config.N + n] = Complex(0.0f, 0.0f);
            } else {
                float phillips = PhillipsSpectrum(k);
                Complex gaussianRand = GetGaussianRandom();
                
                initialSpectrum[m * config.N + n] = gaussianRand * sqrt(phillips * 0.5f);
            }
        }
    }
}

float OceanFFT::PhillipsSpectrum(const glm::vec2& k) const {
    float kLength = glm::length(k);
    if (kLength < 0.000001f) return 0.0f;
    
    float kLength2 = kLength * kLength;
    float kLength4 = kLength2 * kLength2;
    
    // Wind influence
    glm::vec2 kNormalized = k / kLength;
    glm::vec2 windNormalized = glm::normalize(waveParams.windDirection);
    float kDotWind = glm::dot(kNormalized, windNormalized);
    float kDotWind2 = kDotWind * kDotWind;
    
    // Phillips spectrum formula
    float L = (glm::length(waveParams.windSpeed) * glm::length(waveParams.windSpeed)) / waveParams.gravity;
    float damping = exp(-kLength2 * waveParams.damping * waveParams.damping);
    
    return waveParams.A * exp(-1.0f / (kLength2 * L * L)) / kLength4 * kDotWind2 * damping;
}

float OceanFFT::DispersionRelation(const glm::vec2& k) const {
    return sqrt(waveParams.gravity * glm::length(k));
}

OceanFFT::Complex OceanFFT::GetGaussianRandom() {
    return Complex(gaussianDist(rng), gaussianDist(rng));
}

void OceanFFT::UpdateSpectrum(float currentTime) {
    // This would update the time-dependent spectrum
    // For now, keep it simple
}

void OceanFFT::ComputeFFT() {
    // FFT computation will be implemented with compute shaders
    // For now, this is a placeholder
}

void OceanFFT::PerformFFTPass(GLuint inputTexture, GLuint outputTexture, 
                             const Shader& shader, bool horizontal) {
    // FFT pass implementation
}

void OceanFFT::SetupVertexData() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}

void OceanFFT::BindTextures() {
    if (heightmapTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, heightmapTexture);
        glUniform1i(glGetUniformLocation(oceanVertexShader->shaderProgram, "heightmapTexture"), 0);
    }
    
    if (displacementTexture != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, displacementTexture);
        glUniform1i(glGetUniformLocation(oceanVertexShader->shaderProgram, "displacementTexture"), 1);
    }
    
    if (normalTexture != 0) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
        glUniform1i(glGetUniformLocation(oceanVertexShader->shaderProgram, "normalTexture"), 2);
    }
    
    if (foamTexture != 0) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, foamTexture);
        glUniform1i(glGetUniformLocation(oceanVertexShader->shaderProgram, "foamTexture"), 3);
    }
}

void OceanFFT::SetShaderUniforms(const glm::mat4& view, const glm::mat4& projection,
                                 const glm::vec3& cameraPos, const glm::vec3& lightDir,
                                 const glm::vec3& lightColor, const glm::vec3& skyColor) {
    GLuint shaderProgram = oceanVertexShader->shaderProgram;
    
    // Matrices
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Ocean parameters
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), time);
    glUniform1f(glGetUniformLocation(shaderProgram, "oceanSize"), config.oceanSize);
    glUniform1f(glGetUniformLocation(shaderProgram, "choppiness"), waveParams.lambda);
    
    // Lighting
    glUniform3fv(glGetUniformLocation(shaderProgram, "cameraPos"), 1, &cameraPos[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightDir"), 1, &lightDir[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, &lightColor[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "skyColor"), 1, &skyColor[0]);
}

glm::vec2 OceanFFT::GetWaveVector(int n, int m) const {
    float kx = (2.0f * PI * (n - config.N / 2)) / config.oceanSize;
    float kz = (2.0f * PI * (m - config.N / 2)) / config.oceanSize;
    return glm::vec2(kx, kz);
}

unsigned int OceanFFT::ReverseBits(unsigned int value, int numBits) const {
    unsigned int result = 0;
    for (int i = 0; i < numBits; i++) {
        result = (result << 1) | (value & 1);
        value >>= 1;
    }
    return result;
}

bool OceanFFT::IsPowerOfTwo(int value) const {
    return value > 0 && (value & (value - 1)) == 0;
}

// Factory implementations
OceanFFT::WaveParameters OceanFFTFactory::CreateCalmSea() {
    OceanFFT::WaveParameters params;
    params.A = 0.0001f;
    params.windSpeed = glm::vec2(15.0f, 15.0f);
    params.windDirection = glm::vec2(1.0f, 0.0f);
    params.lambda = -0.5f;
    return params;
}

OceanFFT::WaveParameters OceanFFTFactory::CreateRoughSea() {
    OceanFFT::WaveParameters params;
    params.A = 0.001f;
    params.windSpeed = glm::vec2(35.0f, 35.0f);
    params.windDirection = glm::vec2(1.0f, 0.5f);
    params.lambda = -1.0f;
    return params;
}

OceanFFT::WaveParameters OceanFFTFactory::CreateStormySea() {
    OceanFFT::WaveParameters params;
    params.A = 0.005f;
    params.windSpeed = glm::vec2(60.0f, 60.0f);
    params.windDirection = glm::vec2(1.0f, 1.0f);
    params.lambda = -1.5f;
    return params;
}

OceanFFT::WaveParameters OceanFFTFactory::CreateTropicalWaves() {
    OceanFFT::WaveParameters params;
    params.A = 0.0005f;
    params.windSpeed = glm::vec2(25.0f, 25.0f);
    params.windDirection = glm::vec2(1.0f, 0.2f);
    params.lambda = -0.8f;
    return params;
}

OceanFFT::OceanConfig OceanFFTFactory::CreateHighDetailConfig() {
    OceanFFT::OceanConfig config;
    config.N = 1024;
    config.oceanSize = 2000.0f;
    config.enableChoppiness = true;
    config.enableFoam = true;
    return config;
}

OceanFFT::OceanConfig OceanFFTFactory::CreateMediumDetailConfig() {
    OceanFFT::OceanConfig config;
    config.N = 512;
    config.oceanSize = 1000.0f;
    config.enableChoppiness = true;
    config.enableFoam = true;
    return config;
}

OceanFFT::OceanConfig OceanFFTFactory::CreateLowDetailConfig() {
    OceanFFT::OceanConfig config;
    config.N = 256;
    config.oceanSize = 500.0f;
    config.enableChoppiness = false;
    config.enableFoam = false;
    return config;
}

OceanFFT::OceanConfig OceanFFTFactory::CreatePerformanceConfig() {
    OceanFFT::OceanConfig config;
    config.N = 128;
    config.oceanSize = 500.0f;
    config.enableChoppiness = false;
    config.enableFoam = false;
    return config;
}