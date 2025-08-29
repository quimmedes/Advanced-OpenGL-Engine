#include "Light.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <string>

Light::Light(LightType type) : Transform(), type(type), color(1.0f, 1.0f, 1.0f), intensity(1.0f), enabled(true) {
}

// DirectionalLight Implementation
DirectionalLight::DirectionalLight() : Light(LightType::DIRECTIONAL), baseDirection(0.0f, -1.0f, 0.0f), useTransformRotation(true) {
}

DirectionalLight::DirectionalLight(const glm::vec3& dir, const glm::vec3& col, float intens) 
    : Light(LightType::DIRECTIONAL), baseDirection(glm::normalize(dir)), useTransformRotation(false) {
    color = col;
    intensity = intens;
}

void DirectionalLight::setUniforms(GLuint shaderProgram, int lightIndex) const {
    if (!enabled) return;

    std::string prefix = "dirLight";
    if (lightIndex > 0) {
        prefix = "dirLights[" + std::to_string(lightIndex) + "]";
    }

    GLint dirLoc = glGetUniformLocation(shaderProgram, (prefix + ".direction").c_str());
    GLint colorLoc = glGetUniformLocation(shaderProgram, (prefix + ".color").c_str());
    GLint intensityLoc = glGetUniformLocation(shaderProgram, (prefix + ".intensity").c_str());

    if (dirLoc != -1) glUniform3fv(dirLoc, 1, glm::value_ptr(getDirection()));
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));
    if (intensityLoc != -1) glUniform1f(intensityLoc, intensity);
}

// PointLight Implementation
PointLight::PointLight() : Light(LightType::POINT), constant(1.0f), linear(0.09f), quadratic(0.032f) {
}

PointLight::PointLight(const glm::vec3& pos, const glm::vec3& col, float intens) 
    : Light(LightType::POINT), constant(1.0f), linear(0.09f), quadratic(0.032f) {
    position = pos;
    color = col;
    intensity = intens;
}

void PointLight::setUniforms(GLuint shaderProgram, int lightIndex) const {
    if (!enabled) return;

    std::string prefix = "pointLight";
    if (lightIndex > 0) {
        prefix = "pointLights[" + std::to_string(lightIndex) + "]";
    }

    GLint posLoc = glGetUniformLocation(shaderProgram, (prefix + ".position").c_str());
    GLint colorLoc = glGetUniformLocation(shaderProgram, (prefix + ".color").c_str());
    GLint intensityLoc = glGetUniformLocation(shaderProgram, (prefix + ".intensity").c_str());
    GLint constantLoc = glGetUniformLocation(shaderProgram, (prefix + ".constant").c_str());
    GLint linearLoc = glGetUniformLocation(shaderProgram, (prefix + ".linear").c_str());
    GLint quadraticLoc = glGetUniformLocation(shaderProgram, (prefix + ".quadratic").c_str());

    if (posLoc != -1) glUniform3fv(posLoc, 1, glm::value_ptr(getPosition()));
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));
    if (intensityLoc != -1) glUniform1f(intensityLoc, intensity);
    if (constantLoc != -1) glUniform1f(constantLoc, constant);
    if (linearLoc != -1) glUniform1f(linearLoc, linear);
    if (quadraticLoc != -1) glUniform1f(quadraticLoc, quadratic);
}

// SpotLight Implementation
SpotLight::SpotLight() : Light(LightType::SPOT), baseDirection(0.0f, -1.0f, 0.0f), useTransformRotation(true),
                         innerCone(0.95f), outerCone(0.9f), constant(1.0f), linear(0.09f), quadratic(0.032f) {
}

SpotLight::SpotLight(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& col, float intens)
    : Light(LightType::SPOT), baseDirection(glm::normalize(dir)), useTransformRotation(false),
      innerCone(0.95f), outerCone(0.9f), constant(1.0f), linear(0.09f), quadratic(0.032f) {
    position = pos;
    color = col;
    intensity = intens;
}

void SpotLight::setUniforms(GLuint shaderProgram, int lightIndex) const {
    if (!enabled) return;

    std::string prefix = "spotLight";
    if (lightIndex > 0) {
        prefix = "spotLights[" + std::to_string(lightIndex) + "]";
    }

    GLint posLoc = glGetUniformLocation(shaderProgram, (prefix + ".position").c_str());
    GLint dirLoc = glGetUniformLocation(shaderProgram, (prefix + ".direction").c_str());
    GLint colorLoc = glGetUniformLocation(shaderProgram, (prefix + ".color").c_str());
    GLint intensityLoc = glGetUniformLocation(shaderProgram, (prefix + ".intensity").c_str());
    GLint innerLoc = glGetUniformLocation(shaderProgram, (prefix + ".innerCone").c_str());
    GLint outerLoc = glGetUniformLocation(shaderProgram, (prefix + ".outerCone").c_str());
    GLint constantLoc = glGetUniformLocation(shaderProgram, (prefix + ".constant").c_str());
    GLint linearLoc = glGetUniformLocation(shaderProgram, (prefix + ".linear").c_str());
    GLint quadraticLoc = glGetUniformLocation(shaderProgram, (prefix + ".quadratic").c_str());

    if (posLoc != -1) glUniform3fv(posLoc, 1, glm::value_ptr(getPosition()));
    if (dirLoc != -1) glUniform3fv(dirLoc, 1, glm::value_ptr(getDirection()));
    if (colorLoc != -1) glUniform3fv(colorLoc, 1, glm::value_ptr(color));
    if (intensityLoc != -1) glUniform1f(intensityLoc, intensity);
    if (innerLoc != -1) glUniform1f(innerLoc, innerCone);
    if (outerLoc != -1) glUniform1f(outerLoc, outerCone);
    if (constantLoc != -1) glUniform1f(constantLoc, constant);
    if (linearLoc != -1) glUniform1f(linearLoc, linear);
    if (quadraticLoc != -1) glUniform1f(quadraticLoc, quadratic);
}