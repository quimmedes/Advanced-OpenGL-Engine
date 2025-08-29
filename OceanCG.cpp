#include "OceanCG.hpp"
#include <iostream>
#include <cmath>

OceanCG::OceanCG() 
    : vao(0), vbo(0), ebo(0), gridResolution(50), gridSize(50.0f), 
      fresnelPower(2.0f), isInitialized(false) {
    
    // Default lighting setup from the book
    light.ambient = glm::vec4(0.2f, 0.2f, 0.3f, 1.0f);
    light.diffuse = glm::vec4(1.0f, 1.0f, 0.9f, 1.0f);  
    light.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    light.position = glm::vec3(0.0f, 100.0f, 100.0f);
    
    // Enhanced water material with realistic properties
    material.ambient = glm::vec4(0.05f, 0.2f, 0.3f, 1.0f);
    material.diffuse = glm::vec4(0.1f, 0.4f, 0.6f, 1.0f);
    material.specular = glm::vec4(0.9f, 0.95f, 1.0f, 1.0f);
    material.shininess = 256.0f;  // Higher shininess for sharper reflections
    
    globalAmbient = glm::vec4(0.3f, 0.3f, 0.4f, 1.0f);
    
    // Enhanced ocean colors with more vibrant, realistic tones
    deepColor = glm::vec3(0.05f, 0.25f, 0.5f);   // Rich deep blue
    shallowColor = glm::vec3(0.3f, 0.7f, 0.9f);  // Bright tropical blue
}

OceanCG::~OceanCG() {
    Cleanup();
}

bool OceanCG::Initialize(int resolution, float size) {
    if (isInitialized) {
        Cleanup();
    }
    
    gridResolution = resolution;
    gridSize = size;
    
    std::cout << "Initializing Ocean (Computer Graphics Book Method)..." << std::endl;
    std::cout << "- Grid Resolution: " << gridResolution << "x" << gridResolution << std::endl;
    std::cout << "- Grid Size: " << gridSize << " units" << std::endl;
    
    // Initialize shader following book's pattern
    oceanShader = std::make_unique<Shader>();
    if (!oceanShader->InitFromFiles("shaders/ocean_cg.vert", "shaders/ocean_cg.frag")) {
        std::cerr << "Failed to initialize ocean shader!" << std::endl;
        return false;
    }
    
    // Create ocean mesh
    CreateOceanGrid();
    SetupVertexAttributes();
    
    isInitialized = true;
    std::cout << "Ocean system initialized successfully!" << std::endl;
    
    return true;
}

void OceanCG::Cleanup() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    
    oceanShader.reset();
    isInitialized = false;
}

void OceanCG::Update(float deltaTime) {
    if (!isInitialized) return;
    waves.time += deltaTime * waves.speed;
}

void OceanCG::Render(const glm::mat4& mvMatrix, const glm::mat4& projMatrix) {
    if (!isInitialized) return;
    
    // Enable enhanced blending for water transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE); // Don't write to depth buffer for transparency
    
    // Use ocean shader
    oceanShader->use();
    
    // Set shader uniforms using book's approach
    SetShaderUniforms(mvMatrix, projMatrix);
    
    // Render the ocean mesh
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Restore depth writing and disable blending
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void OceanCG::SetLighting(const LightInfo& lightInfo) {
    light = lightInfo;
}

void OceanCG::SetMaterial(const MaterialInfo& matInfo) {
    material = matInfo;
}

void OceanCG::SetOceanColors(const glm::vec3& deep, const glm::vec3& shallow) {
    deepColor = deep;
    shallowColor = shallow;
}

void OceanCG::CreateOceanGrid() {
    vertices.clear();
    indices.clear();
    
    // Create grid vertices following book's approach
    float halfSize = gridSize * 0.5f;
    float stepSize = gridSize / static_cast<float>(gridResolution);
    
    // Generate vertices (position, normal, texcoord)
    for (int z = 0; z <= gridResolution; z++) {
        for (int x = 0; x <= gridResolution; x++) {
            float xPos = -halfSize + x * stepSize;
            float zPos = -halfSize + z * stepSize;
            
            // Vertex position
            vertices.push_back(xPos);   // x
            vertices.push_back(0.0f);   // y (will be displaced by waves)
            vertices.push_back(zPos);   // z
            
            // Normal (initially pointing up, will be recalculated by shader)
            vertices.push_back(0.0f);   // nx
            vertices.push_back(1.0f);   // ny  
            vertices.push_back(0.0f);   // nz
            
            // Texture coordinates
            vertices.push_back(static_cast<float>(x) / gridResolution); // u
            vertices.push_back(static_cast<float>(z) / gridResolution); // v
        }
    }
    
    // Generate indices for triangles
    for (int z = 0; z < gridResolution; z++) {
        for (int x = 0; x < gridResolution; x++) {
            int topLeft = z * (gridResolution + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (gridResolution + 1) + x;
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
    
    std::cout << "Created ocean grid: " << vertices.size()/8 << " vertices, " << indices.size()/3 << " triangles" << std::endl;
}

void OceanCG::SetupVertexAttributes() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    
    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Element buffer  
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void OceanCG::SetShaderUniforms(const glm::mat4& mvMatrix, const glm::mat4& projMatrix) {
    GLuint shaderProgram = oceanShader->shaderProgram;
    
    // Matrix uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mv_matrix"), 1, GL_FALSE, &mvMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj_matrix"), 1, GL_FALSE, &projMatrix[0][0]);
    
    // Normal matrix calculation from the book
    glm::mat4 normalMatrix = CalculateNormalMatrix(mvMatrix);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "norm_matrix"), 1, GL_FALSE, &normalMatrix[0][0]);
    
    // Wave parameters
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), waves.time);
    glUniform1f(glGetUniformLocation(shaderProgram, "waveHeight"), waves.height);
    glUniform1f(glGetUniformLocation(shaderProgram, "waveLength"), waves.length);
    glUniform1f(glGetUniformLocation(shaderProgram, "waveSpeed"), waves.speed);
    
    // Lighting uniforms
    glUniform4fv(glGetUniformLocation(shaderProgram, "globalAmbient"), 1, &globalAmbient[0]);
    
    // Light properties
    glUniform4fv(glGetUniformLocation(shaderProgram, "light.ambient"), 1, &light.ambient[0]);
    glUniform4fv(glGetUniformLocation(shaderProgram, "light.diffuse"), 1, &light.diffuse[0]);
    glUniform4fv(glGetUniformLocation(shaderProgram, "light.specular"), 1, &light.specular[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "light.position"), 1, &light.position[0]);
    
    // Material properties
    glUniform4fv(glGetUniformLocation(shaderProgram, "material.ambient"), 1, &material.ambient[0]);
    glUniform4fv(glGetUniformLocation(shaderProgram, "material.diffuse"), 1, &material.diffuse[0]);
    glUniform4fv(glGetUniformLocation(shaderProgram, "material.specular"), 1, &material.specular[0]);
    glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), material.shininess);
    
    // Ocean-specific parameters
    glUniform3fv(glGetUniformLocation(shaderProgram, "oceanDeepColor"), 1, &deepColor[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "oceanShallowColor"), 1, &shallowColor[0]);
    glUniform1f(glGetUniformLocation(shaderProgram, "fresnelPower"), fresnelPower);
}

glm::mat4 OceanCG::CalculateNormalMatrix(const glm::mat4& mvMatrix) {
    // Normal matrix calculation as shown in the book
    return glm::transpose(glm::inverse(mvMatrix));
}

// Factory implementations following book's examples
OceanCG::WaveParameters OceanCGFactory::CreateCalmWaves() {
    OceanCG::WaveParameters waves;
    waves.height = 0.2f;
    waves.length = 12.0f;
    waves.speed = 0.3f;  // Gentle speed for calm water
    return waves;
}

OceanCG::WaveParameters OceanCGFactory::CreateRoughWaves() {
    OceanCG::WaveParameters waves;
    waves.height = 0.8f;
    waves.length = 6.0f;
    waves.speed = 0.6f;  // Moderate speed for rough waves
    return waves;
}

OceanCG::WaveParameters OceanCGFactory::CreateStormyWaves() {
    OceanCG::WaveParameters waves;
    waves.height = 1.5f;
    waves.length = 4.0f;
    waves.speed = 0.8f;  // Fast stormy waves
    return waves;
}

OceanCG::LightInfo OceanCGFactory::CreateSunlight() {
    OceanCG::LightInfo light;
    light.ambient = glm::vec4(0.3f, 0.3f, 0.4f, 1.0f);
    light.diffuse = glm::vec4(1.0f, 1.0f, 0.9f, 1.0f);
    light.specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    light.position = glm::vec3(100.0f, 100.0f, 50.0f);
    return light;
}

OceanCG::LightInfo OceanCGFactory::CreateMoonlight() {
    OceanCG::LightInfo light;
    light.ambient = glm::vec4(0.1f, 0.1f, 0.2f, 1.0f);
    light.diffuse = glm::vec4(0.6f, 0.7f, 0.8f, 1.0f);
    light.specular = glm::vec4(0.8f, 0.9f, 1.0f, 1.0f);
    light.position = glm::vec3(-50.0f, 80.0f, -30.0f);
    return light;
}

OceanCG::MaterialInfo OceanCGFactory::CreateWaterMaterial() {
    OceanCG::MaterialInfo material;
    material.ambient = glm::vec4(0.05f, 0.2f, 0.3f, 1.0f);
    material.diffuse = glm::vec4(0.1f, 0.4f, 0.6f, 1.0f);
    material.specular = glm::vec4(0.9f, 0.95f, 1.0f, 1.0f);
    material.shininess = 256.0f;  // High shininess for realistic water reflections
    return material;
}