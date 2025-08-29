#include "Shadow.hpp"
#include <iostream>
#include <corecrt_math_defines.h>
#include <vector>
#include <cmath>

// Base ShadowMap Implementation
ShadowMap::ShadowMap() : frameBuffer(0), depthMap(0), width(0), height(0) {
}

ShadowMap::~ShadowMap() {
    cleanup();
}

void ShadowMap::cleanup() {
    if (frameBuffer) {
        glDeleteFramebuffers(1, &frameBuffer);
        frameBuffer = 0;
    }
    if (depthMap) {
        glDeleteTextures(1, &depthMap);
        depthMap = 0;
    }
}

// DirectionalShadowMap Implementation
DirectionalShadowMap::DirectionalShadowMap(DirectionalLight* light) 
    : light(light), near_plane(1.0f), far_plane(100.0f),
      left(-25.0f), right(25.0f), bottom(-25.0f), top(25.0f) {
}

bool DirectionalShadowMap::Init(int w, int h) {
    width = w;
    height = h;
    
    // Generate framebuffer for shadow mapping
    glGenFramebuffers(1, &frameBuffer);
    
    // Generate depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // Set border color to white (outside shadow = no shadow)
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::SHADOW::FRAMEBUFFER:: Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    updateLightSpaceMatrix();
    
    std::cout << "Directional shadow map initialized: " << width << "x" << height << std::endl;
    return true;
}

void DirectionalShadowMap::updateLightSpaceMatrix(const glm::vec3& sceneCenter, float sceneRadius) {
    // Create orthographic projection for directional light
    lightProjection = glm::ortho(left, right, bottom, top, near_plane, far_plane);
    
    // Create view matrix looking from light direction
    glm::vec3 lightPos = sceneCenter - light->getDirection() * (far_plane * 0.5f);
    lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    
    lightSpaceMatrix = lightProjection * lightView;
}

void DirectionalShadowMap::BeginShadowPass() {
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT); // Peter panning fix
}

void DirectionalShadowMap::EndShadowPass() {
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectionalShadowMap::BindForReading(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, depthMap);
}

glm::mat4 DirectionalShadowMap::getLightSpaceMatrix() const {
    return lightSpaceMatrix;
}

// PointShadowMap Implementation
PointShadowMap::PointShadowMap(PointLight* light) 
    : light(light), cubeMapTexture(0), near_plane(1.0f), far_plane(100.0f) {
}

bool PointShadowMap::Init(int w, int h) {
    width = w;
    height = h;
    
    // Generate framebuffer for shadow mapping
    glGenFramebuffers(1, &frameBuffer);
    
    // Generate cube map texture for omnidirectional shadows
    glGenTextures(1, &cubeMapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
    
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, 
                     width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    // Attach cube map to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubeMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR::SHADOW::CUBEMAP:: Framebuffer not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    depthMap = cubeMapTexture; // For compatibility
    
    updateShadowTransforms();
    
    std::cout << "Point light shadow map initialized: " << width << "x" << height << std::endl;
    return true;
}

void PointShadowMap::updateShadowTransforms() {
    glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)width / (float)height, near_plane, far_plane);
    glm::vec3 lightPos = light->getPosition();
    
    shadowTransforms.clear();
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
    shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
}

void PointShadowMap::BeginShadowPass() {
    updateShadowTransforms(); // Update in case light moved
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void PointShadowMap::EndShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PointShadowMap::BindForReading(GLenum textureUnit) {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
}

glm::mat4 PointShadowMap::getLightSpaceMatrix() const {
    // Return first transform as default (not used for point lights)
    return shadowTransforms.empty() ? glm::mat4(1.0f) : shadowTransforms[0];
}

std::vector<glm::mat4> PointShadowMap::getLightSpaceMatrices() const {
    return shadowTransforms;
}