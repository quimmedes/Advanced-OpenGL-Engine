#pragma once
#include "Shader.hpp"
#include "Texture.hpp"
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <memory>

// Forward declaration
class AdvancedMaterial;

enum class MaterialType {
    PBR_BASIC,
    PBR_ADVANCED,
    DISNEY_BRDF,
    METAL_MATERIAL,
    GLASS_MATERIAL,
    SUBSURFACE_MATERIAL,
    EMISSIVE_MATERIAL
};

class Material
{
private:
    glm::vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    
    std::unique_ptr<Texture> diffuseTexture;
    std::unique_ptr<Texture> normalTexture;
    std::unique_ptr<Texture> specularTexture;
    std::unique_ptr<Texture> occlusionTexture;
    
    // Material type and advanced material integration
    MaterialType materialType;
    std::shared_ptr<AdvancedMaterial> advancedMaterial;
    
public:
    Shader shader;
    
    Material();
    Material(const glm::vec3& albedo, float metallic, float roughness, float ao);
    
    bool Init();
    bool InitWithShader(const std::string& vertexPath, const std::string& fragmentPath);
    void setUniforms(GLuint shaderProgram) const;
    void bindTextures() const;
    
    void setAlbedo(const glm::vec3& albedo) { this->albedo = albedo; }
    void setMetallic(float metallic) { this->metallic = metallic; }
    void setRoughness(float roughness) { this->roughness = roughness; }
    void setAO(float ao) { this->ao = ao; }
    
    void setDiffuseTexture(const std::string& texturePath);
    void setNormalTexture(const std::string& texturePath);
    void setSpecularTexture(const std::string& texturePath);
    void setOcclusionTexture(const std::string& texturePath);
    
    glm::vec3 getAlbedo() const { return albedo; }
    float getMetallic() const { return metallic; }
    float getRoughness() const { return roughness; }
    float getAO() const { return ao; }
    
    bool hasDiffuseTexture() const { return diffuseTexture && diffuseTexture->isValid(); }
    bool hasNormalTexture() const { return normalTexture && normalTexture->isValid(); }
    bool hasSpecularTexture() const { return specularTexture && specularTexture->isValid(); }
    bool hasOcclusionTexture() const { return occlusionTexture && occlusionTexture->isValid(); }
    
    // Material type management
    void setMaterialType(MaterialType type) { materialType = type; }
    MaterialType getMaterialType() const { return materialType; }
    
    // Advanced material integration
    void setAdvancedMaterial(std::shared_ptr<AdvancedMaterial> advanced);
    std::shared_ptr<AdvancedMaterial> getAdvancedMaterial() const { return advancedMaterial; }
    bool hasAdvancedMaterial() const { return advancedMaterial != nullptr; }
    
    // Enhanced setUniforms that works with both basic PBR and advanced materials
    void setUniformsAdvanced(GLuint shaderProgram) const;
    
private:
    // Helper methods for setting material-specific uniforms
    void setDisneyBRDFUniforms(GLuint shaderProgram) const;
    void setMetalMaterialUniforms(GLuint shaderProgram) const;
    void setGlassMaterialUniforms(GLuint shaderProgram) const;
    void setSubsurfaceMaterialUniforms(GLuint shaderProgram) const;
    void setEmissiveMaterialUniforms(GLuint shaderProgram) const;
};

