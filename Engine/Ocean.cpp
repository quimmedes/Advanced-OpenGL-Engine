#include "Ocean.hpp"
#include <iostream>
#include <cmath>

Ocean::Ocean() 
    : VAO(0), VBO(0), EBO(0), time(0.0f), isInitialized(false),
      dudvTexture(0), normalTexture(0), skyboxTexture(0) {
}

Ocean::~Ocean() {
    Cleanup();
}

bool Ocean::Initialize(const OceanConfig& cfg) {
    if (isInitialized) {
        Cleanup();
    }
    
    config = cfg;
    
    std::cout << "Initializing Ocean System..." << std::endl;
    std::cout << "- Resolution: " << config.resolution << "x" << config.resolution << std::endl;
    std::cout << "- Size: " << config.size << " units" << std::endl;
    std::cout << "- Wave Count: " << config.numWaves << std::endl;
    
    // Initialize shader with simpler version to avoid flickering
    oceanShader = std::make_unique<Shader>();
    if (!oceanShader->InitFromFiles("shaders/ocean_simple.vert", "shaders/ocean_simple.frag")) {
        std::cerr << "Failed to initialize ocean shader!" << std::endl;
        return false;
    }
    
    // Generate mesh
    GenerateMesh();
    SetupVertexData();
    CreateTextures();
    
    isInitialized = true;
    std::cout << "Ocean system initialized successfully!" << std::endl;
    
    return true;
}

void Ocean::Cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    
    if (dudvTexture != 0) {
        glDeleteTextures(1, &dudvTexture);
        dudvTexture = 0;
    }
    if (normalTexture != 0) {
        glDeleteTextures(1, &normalTexture);
        normalTexture = 0;
    }
    if (skyboxTexture != 0) {
        glDeleteTextures(1, &skyboxTexture);
        skyboxTexture = 0;
    }
    
    oceanShader.reset();
    oceanMaterial.reset();
    isInitialized = false;
}

void Ocean::Update(float deltaTime) {
    if (!isInitialized) return;
    time += deltaTime;
}

void Ocean::Render(const glm::mat4& view, const glm::mat4& projection, 
                   const glm::vec3& viewPos, const glm::vec3& lightDir, 
                   const glm::vec3& lightColor, const glm::vec3& skyColor) {
    if (!isInitialized) return;
    
    // Enable enhanced blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer for transparency
    
    // Use ocean shader
    oceanShader->use();
    
    // Set matrices
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(oceanShader->shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(oceanShader->shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(oceanShader->shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    
    // Set simplified uniforms for simple shader
    glUniform1f(glGetUniformLocation(oceanShader->shaderProgram, "u_time"), time);
    glUniform1f(glGetUniformLocation(oceanShader->shaderProgram, "u_waveAmplitude"), config.waveAmplitude);
    glUniform3fv(glGetUniformLocation(oceanShader->shaderProgram, "u_oceanDeepColor"), 1, &config.deepColor[0]);
    glUniform3fv(glGetUniformLocation(oceanShader->shaderProgram, "u_oceanShallowColor"), 1, &config.shallowColor[0]);
    
    // Set lighting
    glUniform3fv(glGetUniformLocation(oceanShader->shaderProgram, "viewPos"), 1, &viewPos[0]);
    glUniform3fv(glGetUniformLocation(oceanShader->shaderProgram, "lightDirection"), 1, &lightDir[0]);
    glUniform3fv(glGetUniformLocation(oceanShader->shaderProgram, "lightColor"), 1, &lightColor[0]);
    glUniform3fv(glGetUniformLocation(oceanShader->shaderProgram, "skyColor"), 1, &skyColor[0]);
    
    // Skip texture binding for now - use procedural effects in shader
    // This prevents flickering from missing textures
    
    // Render mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Restore depth writing and disable blending
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Ocean::SetConfig(const OceanConfig& cfg) {
    config = cfg;
    if (isInitialized) {
        GenerateMesh();
        UpdateMeshData();
    }
}

float Ocean::SampleWaveHeight(float x, float z, float currentTime) const {
    if (currentTime < 0.0f) currentTime = time;
    
    glm::vec2 pos(x, z);
    std::vector<Wave> waves = GenerateWaveSet();
    
    float height = 0.0f;
    for (const auto& wave : waves) {
        float c = sqrt(9.8f / wave.frequency);
        glm::vec2 d = glm::normalize(wave.direction);
        float theta = glm::dot(d, pos) * wave.frequency + currentTime * c + wave.phase;
        height += wave.amplitude * sin(theta);
    }
    
    return height;
}

glm::vec3 Ocean::SampleWaveNormal(float x, float z, float currentTime) const {
    if (currentTime < 0.0f) currentTime = time;
    
    // Calculate normal using finite differences
    const float epsilon = 0.1f;
    float h0 = SampleWaveHeight(x, z, currentTime);
    float hx = SampleWaveHeight(x + epsilon, z, currentTime);
    float hz = SampleWaveHeight(x, z + epsilon, currentTime);
    
    glm::vec3 tangentX = glm::normalize(glm::vec3(epsilon, hx - h0, 0.0f));
    glm::vec3 tangentZ = glm::normalize(glm::vec3(0.0f, hz - h0, epsilon));
    
    return glm::normalize(glm::cross(tangentZ, tangentX));
}

void Ocean::GenerateMesh() {
    vertices.clear();
    normals.clear();
    texCoords.clear();
    
    int numVertices = (config.resolution + 1) * (config.resolution + 1);
    vertices.reserve(numVertices);
    normals.reserve(numVertices);
    texCoords.reserve(numVertices);
    
    float halfSize = config.size * 0.5f;
    float stepSize = config.size / config.resolution;
    
    // Generate vertices
    for (int z = 0; z <= config.resolution; z++) {
        for (int x = 0; x <= config.resolution; x++) {
            float worldX = -halfSize + x * stepSize;
            float worldZ = -halfSize + z * stepSize;
            
            vertices.emplace_back(worldX, 0.0f, worldZ);
            normals.emplace_back(0.0f, 1.0f, 0.0f); // Will be calculated in shader
            texCoords.emplace_back(static_cast<float>(x) / config.resolution, 
                                 static_cast<float>(z) / config.resolution);
        }
    }
    
    GenerateIndices();
    
    std::cout << "Generated ocean mesh: " << vertices.size() << " vertices, " 
              << indices.size() << " indices" << std::endl;
}

void Ocean::GenerateIndices() {
    indices.clear();
    int numIndices = config.resolution * config.resolution * 6;
    indices.reserve(numIndices);
    
    for (int z = 0; z < config.resolution; z++) {
        for (int x = 0; x < config.resolution; x++) {
            int topLeft = z * (config.resolution + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (config.resolution + 1) + x;
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
}

void Ocean::CalculateNormals() {
    // Normals are calculated in the vertex shader for animated waves
    // This function is kept for potential CPU-side calculations
}

void Ocean::UpdateMeshData() {
    if (!isInitialized) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    // Update vertex positions (if needed for CPU-side wave calculation)
    // For GPU-based waves, mesh stays static and animation happens in vertex shader
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Ocean::SetupVertexData() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    // Create interleaved vertex data
    std::vector<float> vertexData;
    size_t totalVertices = vertices.size();
    vertexData.reserve(totalVertices * 8); // 3 pos + 3 normal + 2 texcoord
    
    for (size_t i = 0; i < totalVertices; i++) {
        // Position
        vertexData.push_back(vertices[i].x);
        vertexData.push_back(vertices[i].y);
        vertexData.push_back(vertices[i].z);
        
        // Normal
        vertexData.push_back(normals[i].x);
        vertexData.push_back(normals[i].y);
        vertexData.push_back(normals[i].z);
        
        // Texture coordinates
        vertexData.push_back(texCoords[i].x);
        vertexData.push_back(texCoords[i].y);
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), 
                 vertexData.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), 
                 indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    std::cout << "Ocean vertex data setup complete" << std::endl;
}

void Ocean::CreateTextures() {
    // Create procedural DUDV texture for water distortion
    const int texSize = 256;
    std::vector<unsigned char> dudvData(texSize * texSize * 3);
    
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            int index = (y * texSize + x) * 3;
            float fx = static_cast<float>(x) / texSize;
            float fy = static_cast<float>(y) / texSize;
            
            // Simple noise pattern for DUDV
            float noise1 = sin(fx * 12.0f + fy * 8.0f) * 0.5f + 0.5f;
            float noise2 = cos(fx * 8.0f - fy * 12.0f) * 0.5f + 0.5f;
            
            dudvData[index] = static_cast<unsigned char>(noise1 * 255);
            dudvData[index + 1] = static_cast<unsigned char>(noise2 * 255);
            dudvData[index + 2] = 127; // Neutral blue channel
        }
    }
    
    glGenTextures(1, &dudvTexture);
    glBindTexture(GL_TEXTURE_2D, dudvTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, dudvData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Create procedural normal map
    std::vector<unsigned char> normalData(texSize * texSize * 3);
    
    for (int y = 0; y < texSize; y++) {
        for (int x = 0; x < texSize; x++) {
            int index = (y * texSize + x) * 3;
            float fx = static_cast<float>(x) / texSize;
            float fy = static_cast<float>(y) / texSize;
            
            // Generate normal map from height field
            float height = sin(fx * 16.0f) * cos(fy * 16.0f) * 0.1f;
            float heightX = sin((fx + 1.0f/texSize) * 16.0f) * cos(fy * 16.0f) * 0.1f;
            float heightY = sin(fx * 16.0f) * cos((fy + 1.0f/texSize) * 16.0f) * 0.1f;
            
            glm::vec3 normal = glm::normalize(glm::vec3(height - heightX, height - heightY, 1.0f));
            normal = normal * 0.5f + 0.5f; // Convert to [0,1] range
            
            normalData[index] = static_cast<unsigned char>(normal.x * 255);
            normalData[index + 1] = static_cast<unsigned char>(normal.y * 255);
            normalData[index + 2] = static_cast<unsigned char>(normal.z * 255);
        }
    }
    
    glGenTextures(1, &normalTexture);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texSize, texSize, 0, GL_RGB, GL_UNSIGNED_BYTE, normalData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    std::cout << "Ocean textures created successfully" << std::endl;
}

std::vector<Ocean::Wave> Ocean::GenerateWaveSet() const {
    std::vector<Wave> waves;
    waves.reserve(config.numWaves);
    
    glm::vec2 windDir = glm::normalize(config.windDirection);
    float baseFreq = config.waveFrequency;
    float baseAmp = config.waveAmplitude;
    
    for (int i = 0; i < config.numWaves; i++) {
        Wave wave;
        float angle = static_cast<float>(i) * 0.785398f; // 45 degrees in radians
        glm::vec2 dir = glm::vec2(cos(angle), sin(angle));
        wave.direction = glm::mix(dir, windDir, 0.7f); // Bias towards wind direction
        
        wave.frequency = baseFreq * (1.0f + static_cast<float>(i) * 0.3f);
        wave.amplitude = baseAmp / (1.0f + static_cast<float>(i) * 0.5f);
        wave.phase = static_cast<float>(i) * 1.57079f; // 90 degrees
        wave.steepness = 0.8f / (wave.frequency * wave.amplitude * static_cast<float>(config.numWaves));
        
        waves.push_back(wave);
    }
    
    return waves;
}

glm::vec3 Ocean::CalculateGerstnerWave(const glm::vec2& position, const Wave& wave, 
                                      float currentTime, glm::vec3& tangent, 
                                      glm::vec3& binormal) const {
    float c = sqrt(9.8f / wave.frequency);
    glm::vec2 d = glm::normalize(wave.direction);
    float f = wave.frequency;
    float a = wave.amplitude;
    float phi = wave.phase;
    float q = wave.steepness / (wave.frequency * a * static_cast<float>(config.numWaves));
    
    float theta = glm::dot(d, position) * f + currentTime * c + phi;
    float sinTheta = sin(theta);
    float cosTheta = cos(theta);
    
    // Displacement
    glm::vec3 displacement;
    displacement.x = q * a * d.x * cosTheta;
    displacement.z = q * a * d.y * cosTheta;
    displacement.y = a * sinTheta;
    
    // Update tangent and binormal for normal calculation
    tangent.x += -q * d.x * d.x * f * a * sinTheta;
    tangent.y += d.x * f * a * cosTheta;
    tangent.z += -q * d.x * d.y * f * a * sinTheta;
    
    binormal.x += -q * d.x * d.y * f * a * sinTheta;
    binormal.y += d.y * f * a * cosTheta;
    binormal.z += -q * d.y * d.y * f * a * sinTheta;
    
    return displacement;
}

// Ocean Factory Implementation
Ocean::OceanConfig OceanFactory::CreateCalmOcean() {
    Ocean::OceanConfig config;
    config.windSpeed = 5.0f;
    config.waveAmplitude = 0.3f;
    config.waveFrequency = 0.05f;
    config.numWaves = 3;
    config.deepColor = glm::vec3(0.0f, 0.2f, 0.4f);
    config.shallowColor = glm::vec3(0.3f, 0.7f, 0.9f);
    return config;
}

Ocean::OceanConfig OceanFactory::CreateRoughSea() {
    Ocean::OceanConfig config;
    config.windSpeed = 35.0f;
    config.waveAmplitude = 3.0f;
    config.waveFrequency = 0.015f;
    config.numWaves = 8;
    config.deepColor = glm::vec3(0.0f, 0.1f, 0.2f);
    config.shallowColor = glm::vec3(0.1f, 0.5f, 0.7f);
    return config;
}

Ocean::OceanConfig OceanFactory::CreateStormyOcean() {
    Ocean::OceanConfig config;
    config.windSpeed = 50.0f;
    config.waveAmplitude = 5.0f;
    config.waveFrequency = 0.01f;
    config.numWaves = 10;
    config.deepColor = glm::vec3(0.0f, 0.05f, 0.1f);
    config.shallowColor = glm::vec3(0.1f, 0.3f, 0.5f);
    config.roughness = 0.1f;
    return config;
}

Ocean::OceanConfig OceanFactory::CreateTropicalOcean() {
    Ocean::OceanConfig config;
    config.windSpeed = 15.0f;
    config.waveAmplitude = 1.0f;
    config.waveFrequency = 0.03f;
    config.numWaves = 5;
    config.deepColor = glm::vec3(0.0f, 0.3f, 0.6f);
    config.shallowColor = glm::vec3(0.4f, 0.8f, 1.0f);
    return config;
}

Ocean::OceanConfig OceanFactory::CreateArcticOcean() {
    Ocean::OceanConfig config;
    config.windSpeed = 25.0f;
    config.waveAmplitude = 1.5f;
    config.waveFrequency = 0.025f;
    config.numWaves = 6;
    config.deepColor = glm::vec3(0.0f, 0.1f, 0.3f);
    config.shallowColor = glm::vec3(0.2f, 0.4f, 0.6f);
    config.transparency = 0.9f;
    return config;
}

Ocean::OceanConfig OceanFactory::CreateCustomOcean(float windSpeed, 
                                                  const glm::vec2& windDir,
                                                  float waveHeight,
                                                  const glm::vec3& waterColor) {
    Ocean::OceanConfig config;
    config.windSpeed = windSpeed;
    config.windDirection = windDir;
    config.waveAmplitude = waveHeight;
    config.deepColor = waterColor * 0.3f;
    config.shallowColor = waterColor;
    
    // Adjust other parameters based on wind speed
    if (windSpeed < 10.0f) {
        config.waveFrequency = 0.05f;
        config.numWaves = 3;
    } else if (windSpeed < 30.0f) {
        config.waveFrequency = 0.02f;
        config.numWaves = 6;
    } else {
        config.waveFrequency = 0.01f;
        config.numWaves = 8;
    }
    
    return config;
}