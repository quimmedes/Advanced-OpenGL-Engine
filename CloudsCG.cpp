#include "CloudsCG.hpp"
#include <iostream>

CloudsCG::CloudsCG() 
    : vao(0), vbo(0), isInitialized(false) {
    
    // Default parameters - more visible clouds
    params.coverage = 0.8f;
    params.density = 1.5f;
    params.speed = 0.1f;
    params.altitude = 0.2f;
    params.time = 0.0f;
    
    // Default sky color (daytime)
    skyColor = glm::vec3(0.5f, 0.7f, 1.0f);
    lightDirection = glm::normalize(glm::vec3(-1.0f, -0.5f, -0.2f));
}

CloudsCG::~CloudsCG() {
    Cleanup();
}

bool CloudsCG::Initialize() {
    if (isInitialized) {
        Cleanup();
    }
    
    std::cout << "Initializing Clouds (Computer Graphics Book Method)..." << std::endl;
    std::cout << "- Using skybox technique from the book" << std::endl;
    std::cout << "- Procedural cloud generation" << std::endl;
    
    // Initialize shader following book's skybox pattern
    cloudShader = std::make_unique<Shader>();
    if (!cloudShader->InitFromFiles("shaders/clouds_cg.vert", "shaders/clouds_cg.frag")) {
        std::cerr << "Failed to initialize cloud shader!" << std::endl;
        return false;
    }
    
    // Create skybox geometry
    CreateSkyboxGeometry();
    SetupSkyboxVertices();
    
    isInitialized = true;
    std::cout << "Cloud system initialized successfully!" << std::endl;
    
    return true;
}

void CloudsCG::Cleanup() {
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    
    cloudShader.reset();
    isInitialized = false;
}

void CloudsCG::Update(float deltaTime) {
    if (!isInitialized) return;
    params.time += deltaTime * params.speed;
}

void CloudsCG::RenderSkybox(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    if (!isInitialized) return;
    
    // Skybox rendering setup from the book
    glDepthFunc(GL_LEQUAL);  // Change depth test for skybox
    glDisable(GL_CULL_FACE); // Disable face culling for skybox
    
    cloudShader->use();
    
    // Set shader uniforms
    SetShaderUniforms(viewMatrix, projMatrix);
    
    // Render skybox cube
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);  // 36 vertices for cube
    glBindVertexArray(0);
    
    glEnable(GL_CULL_FACE);  // Re-enable face culling
    glDepthFunc(GL_LESS);    // Restore normal depth testing
}

void CloudsCG::CreateSkyboxGeometry() {
    // Skybox cube vertices from the book (unit cube centered at origin)
    skyboxVertices = {
        // Back face
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        
        // Front face  
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        
        // Left face
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        
        // Right face
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        
        // Bottom face
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        
        // Top face
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f
    };
}

void CloudsCG::SetupSkyboxVertices() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, skyboxVertices.size() * sizeof(float), 
                 skyboxVertices.data(), GL_STATIC_DRAW);
    
    // Position attribute (location = 0) - only need position for skybox
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    std::cout << "Skybox geometry created: " << skyboxVertices.size()/3 << " vertices" << std::endl;
}

void CloudsCG::SetShaderUniforms(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    GLuint shaderProgram = cloudShader->shaderProgram;
    
    // Matrix uniforms - following book's skybox approach
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "mv_matrix"), 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "proj_matrix"), 1, GL_FALSE, &projMatrix[0][0]);
    
    // Cloud parameters
    glUniform1f(glGetUniformLocation(shaderProgram, "time"), params.time);
    glUniform1f(glGetUniformLocation(shaderProgram, "cloudCoverage"), params.coverage);
    glUniform1f(glGetUniformLocation(shaderProgram, "cloudDensity"), params.density);
    
    // Lighting and colors
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightDirection"), 1, &lightDirection[0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "skyColor"), 1, &skyColor[0]);
}

// Factory implementations following book's examples
CloudsCG::CloudParameters CloudsCGFactory::CreateClearSky() {
    CloudsCG::CloudParameters params;
    params.coverage = 0.2f;
    params.density = 0.3f;
    params.speed = 0.1f;
    params.altitude = 0.5f;
    return params;
}

CloudsCG::CloudParameters CloudsCGFactory::CreatePartlyCloudy() {
    CloudsCG::CloudParameters params;
    params.coverage = 0.8f;     // More cloud coverage
    params.density = 1.2f;      // Denser clouds
    params.speed = 0.1f;        // Slower movement
    params.altitude = 0.2f;     // Lower in sky for better visibility
    return params;
}

CloudsCG::CloudParameters CloudsCGFactory::CreateOvercast() {
    CloudsCG::CloudParameters params;
    params.coverage = 0.9f;
    params.density = 0.8f;
    params.speed = 0.2f;
    params.altitude = 0.3f;
    return params;
}

CloudsCG::CloudParameters CloudsCGFactory::CreateStormyClouds() {
    CloudsCG::CloudParameters params;
    params.coverage = 0.95f;
    params.density = 1.0f;
    params.speed = 0.8f;
    params.altitude = 0.2f;
    return params;
}

glm::vec3 CloudsCGFactory::GetDaySkyColor() {
    return glm::vec3(0.5f, 0.7f, 1.0f);  // Bright blue
}

glm::vec3 CloudsCGFactory::GetSunsetSkyColor() {
    return glm::vec3(1.0f, 0.6f, 0.3f);  // Orange-red
}

glm::vec3 CloudsCGFactory::GetNightSkyColor() {
    return glm::vec3(0.1f, 0.1f, 0.3f);  // Dark blue
}