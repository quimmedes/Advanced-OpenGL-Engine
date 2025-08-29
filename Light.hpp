#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>
#include "Transform.hpp"

enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT
};

class Light : public Transform {
public:
    Light(LightType type);
    virtual ~Light() = default;

    // Common properties
    glm::vec3 color;
    float intensity;
    bool enabled;

    // Getters
    LightType getType() const { return type; }
    virtual glm::vec3 getPosition() const { return position; }
    virtual glm::vec3 getDirection() const { 
        // Transform default down direction by rotation
        glm::vec4 dir = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        glm::mat4 rotMatrix = glm::mat4(1.0f);
        rotMatrix = glm::rotate(rotMatrix, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        rotMatrix = glm::rotate(rotMatrix, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        rotMatrix = glm::rotate(rotMatrix, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        return glm::normalize(glm::vec3(rotMatrix * dir));
    }
    
    // Virtual methods for shader uniforms
    virtual void setUniforms(GLuint shaderProgram, int lightIndex = 0) const = 0;

protected:
    LightType type;
};

class DirectionalLight : public Light {
public:
    DirectionalLight();
    DirectionalLight(const glm::vec3& direction, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);

    glm::vec3 getDirection() const override { 
        // Use rotation from Transform base class, or explicit direction if set
        if (useTransformRotation) {
            return Light::getDirection();
        }
        return glm::normalize(baseDirection);
    }
    
    void setDirection(const glm::vec3& dir) { 
        baseDirection = glm::normalize(dir); 
        useTransformRotation = false;
    }
    
    void setUniforms(GLuint shaderProgram, int lightIndex = 0) const override;

private:
    glm::vec3 baseDirection;
    bool useTransformRotation;
};

class PointLight : public Light {
public:
    PointLight();
    PointLight(const glm::vec3& pos, const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);

    float constant;  // Attenuation factors
    float linear;
    float quadratic;

    void setUniforms(GLuint shaderProgram, int lightIndex = 0) const override;
};

class SpotLight : public Light {
public:
    SpotLight();
    SpotLight(const glm::vec3& pos, const glm::vec3& direction, 
              const glm::vec3& color = glm::vec3(1.0f), float intensity = 1.0f);

    float innerCone;  // Inner cone angle (cosine)
    float outerCone;  // Outer cone angle (cosine)
    float constant;   // Attenuation factors
    float linear;
    float quadratic;

    glm::vec3 getDirection() const override { 
        // Use rotation from Transform base class, or explicit direction if set
        if (useTransformRotation) {
            return Light::getDirection();
        }
        return glm::normalize(baseDirection);
    }
    
    void setDirection(const glm::vec3& dir) { 
        baseDirection = glm::normalize(dir); 
        useTransformRotation = false;
    }
    
    void setUniforms(GLuint shaderProgram, int lightIndex = 0) const override;

private:
    glm::vec3 baseDirection;
    bool useTransformRotation;
};