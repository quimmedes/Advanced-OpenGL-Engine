#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vector>
#include <corecrt_math_defines.h>
#include "Light.hpp"

class ShadowMap {
public:
    ShadowMap();
    virtual ~ShadowMap();
    
    virtual bool Init(int width, int height) = 0;
    virtual void BeginShadowPass() = 0;
    virtual void EndShadowPass() = 0;
    virtual void BindForReading(GLenum textureUnit) = 0;
    virtual glm::mat4 getLightSpaceMatrix() const = 0;
    
    void cleanup();
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    GLuint getDepthMap() const { return depthMap; }

protected:
    GLuint frameBuffer;
    GLuint depthMap;
    int width, height;
};

class DirectionalShadowMap : public ShadowMap {
public:
    DirectionalShadowMap(DirectionalLight* light);
    
    bool Init(int width = 2048, int height = 2048) override;
    void BeginShadowPass() override;
    void EndShadowPass() override;
    void BindForReading(GLenum textureUnit) override;
    glm::mat4 getLightSpaceMatrix() const override;
    
    void updateLightSpaceMatrix(const glm::vec3& sceneCenter = glm::vec3(0.0f), float sceneRadius = 50.0f);

private:
    DirectionalLight* light;
    glm::mat4 lightProjection;
    glm::mat4 lightView;
    glm::mat4 lightSpaceMatrix;
    
    // Shadow frustum bounds
    float near_plane, far_plane;
    float left, right, bottom, top;
};

class PointShadowMap : public ShadowMap {
public:
    PointShadowMap(PointLight* light);
    
    bool Init(int width = 1024, int height = 1024) override;
    void BeginShadowPass() override;
    void EndShadowPass() override;
    void BindForReading(GLenum textureUnit) override;
    glm::mat4 getLightSpaceMatrix() const override;
    
    // Get all 6 face matrices for cube mapping
    std::vector<glm::mat4> getLightSpaceMatrices() const;

private:
    PointLight* light;
    GLuint cubeMapTexture;
    float near_plane, far_plane;
    std::vector<glm::mat4> shadowTransforms;
    
    void updateShadowTransforms();
};