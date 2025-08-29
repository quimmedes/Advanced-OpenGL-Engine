#include "Material.hpp"
#include <glm/gtc/type_ptr.hpp>

Material::Material() 
    : albedo(glm::vec3(0.2f, 0.4f, 0.8f)),  // Default to blue for visibility
      metallic(0.0f),
      roughness(0.5f),
      ao(1.0f),
      materialType(MaterialType::PBR_BASIC),
      advancedMaterial(nullptr)
{
}

Material::Material(const glm::vec3& albedo, float metallic, float roughness, float ao)
    : albedo(albedo), metallic(metallic), roughness(roughness), ao(ao),
      materialType(MaterialType::PBR_BASIC), advancedMaterial(nullptr)
{
}

bool Material::Init()
{
    // Use advanced shader by default for better material support
    return shader.InitFromFiles("shaders/pbr_advanced.vert", "shaders/pbr_advanced.frag");
}

bool Material::InitWithShader(const std::string& vertexPath, const std::string& fragmentPath)
{
    return shader.InitFromFiles(vertexPath, fragmentPath);
}

void Material::setUniforms(GLuint shaderProgram) const
{
    // Try PBR uniforms first
    GLint albedoLoc = glGetUniformLocation(shaderProgram, "material_albedo");
    GLint metallicLoc = glGetUniformLocation(shaderProgram, "material_metallic");
    GLint roughnessLoc = glGetUniformLocation(shaderProgram, "material_roughness");
    GLint aoLoc = glGetUniformLocation(shaderProgram, "material_ao");
    
    if (albedoLoc != -1) glUniform3fv(albedoLoc, 1, glm::value_ptr(albedo));
    if (metallicLoc != -1) glUniform1f(metallicLoc, metallic);
    if (roughnessLoc != -1) glUniform1f(roughnessLoc, roughness);
    if (aoLoc != -1) glUniform1f(aoLoc, ao);
    
    // Check for simple shader uniforms (Phong lighting)
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    GLint selectionHighlightLoc = glGetUniformLocation(shaderProgram, "selectionHighlight");
    
    if (objectColorLoc != -1) {
        
        // Check if albedo is problematic and override
        glm::vec3 finalColor = albedo;
        if (albedo.r > 0.95f && albedo.g > 0.95f && albedo.b > 0.95f) {
            finalColor = glm::vec3(1.0f, 0.0f, 1.0f); // Bright magenta for white materials
        } else if (albedo.r < 0.05f && albedo.g < 0.05f && albedo.b < 0.05f) {
            finalColor = glm::vec3(0.0f, 1.0f, 1.0f); // Bright cyan for black materials
        }
        
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(finalColor));
    }
    if (selectionHighlightLoc != -1) {
        glUniform3f(selectionHighlightLoc, 0.0f, 0.0f, 0.0f); // No highlight
    }
    
    // Set texture uniforms
    GLint diffuseTexLoc = glGetUniformLocation(shaderProgram, "material_diffuseTexture");
    GLint normalTexLoc = glGetUniformLocation(shaderProgram, "material_normalTexture");
    GLint specularTexLoc = glGetUniformLocation(shaderProgram, "material_specularTexture");
    GLint occlusionTexLoc = glGetUniformLocation(shaderProgram, "material_occlusionTexture");
    GLint hasDiffuseTexLoc = glGetUniformLocation(shaderProgram, "material_hasDiffuseTexture");
    GLint hasNormalTexLoc = glGetUniformLocation(shaderProgram, "material_hasNormalTexture");
    GLint hasSpecularTexLoc = glGetUniformLocation(shaderProgram, "material_hasSpecularTexture");
    GLint hasOcclusionTexLoc = glGetUniformLocation(shaderProgram, "material_hasOcclusionTexture");
    
    if (diffuseTexLoc != -1) glUniform1i(diffuseTexLoc, 0);
    if (normalTexLoc != -1) glUniform1i(normalTexLoc, 1);
    if (specularTexLoc != -1) glUniform1i(specularTexLoc, 2);
    if (occlusionTexLoc != -1) glUniform1i(occlusionTexLoc, 3);
    
    if (hasDiffuseTexLoc != -1) glUniform1i(hasDiffuseTexLoc, hasDiffuseTexture() ? 1 : 0);
    if (hasNormalTexLoc != -1) glUniform1i(hasNormalTexLoc, hasNormalTexture() ? 1 : 0);
    if (hasSpecularTexLoc != -1) glUniform1i(hasSpecularTexLoc, hasSpecularTexture() ? 1 : 0);
    if (hasOcclusionTexLoc != -1) glUniform1i(hasOcclusionTexLoc, hasOcclusionTexture() ? 1 : 0);
}

void Material::bindTextures() const
{
    if (hasDiffuseTexture()) {
        diffuseTexture->bind(GL_TEXTURE0);
        // std::cout << "Binding diffuse texture to unit 0" << std::endl;
    }
    if (hasNormalTexture()) {
        normalTexture->bind(GL_TEXTURE1);
        // std::cout << "Binding normal texture to unit 1" << std::endl;
    }
    if (hasSpecularTexture()) {
        specularTexture->bind(GL_TEXTURE2);
        // std::cout << "Binding specular texture to unit 2" << std::endl;
    }
    if (hasOcclusionTexture()) {
        occlusionTexture->bind(GL_TEXTURE3);
        // std::cout << "Binding occlusion texture to unit 3" << std::endl;
    }
}

void Material::setDiffuseTexture(const std::string& texturePath)
{
    diffuseTexture = std::make_unique<Texture>(texturePath);
    std::cout << "Set diffuse texture: " << texturePath << " (Valid: " << (diffuseTexture->isValid() ? "Yes" : "No") << ")" << std::endl;
}

void Material::setNormalTexture(const std::string& texturePath)
{
    normalTexture = std::make_unique<Texture>(texturePath);
    std::cout << "Set normal texture: " << texturePath << " (Valid: " << (normalTexture->isValid() ? "Yes" : "No") << ")" << std::endl;
}

void Material::setSpecularTexture(const std::string& texturePath)
{
    specularTexture = std::make_unique<Texture>(texturePath);
    std::cout << "Set specular texture: " << texturePath << " (Valid: " << (specularTexture->isValid() ? "Yes" : "No") << ")" << std::endl;
}

void Material::setOcclusionTexture(const std::string& texturePath)
{
    occlusionTexture = std::make_unique<Texture>(texturePath);
    std::cout << "Set occlusion texture: " << texturePath << " (Valid: " << (occlusionTexture->isValid() ? "Yes" : "No") << ")" << std::endl;
}

void Material::setAdvancedMaterial(std::shared_ptr<AdvancedMaterial> advanced)
{
    advancedMaterial = advanced;
    if (advanced) {
        materialType = MaterialType::PBR_ADVANCED;
        std::cout << "Advanced material attached to basic material system" << std::endl;
    }
}

void Material::setUniformsAdvanced(GLuint shaderProgram) const
{
    // Set basic PBR uniforms (always available)
    setUniforms(shaderProgram);
    
    // Set advanced material type uniform
    GLint materialTypeLoc = glGetUniformLocation(shaderProgram, "u_materialType");
    if (materialTypeLoc != -1) {
        glUniform1i(materialTypeLoc, static_cast<int>(materialType));
    }
    
    // Set advanced material parameters based on type
    if (hasAdvancedMaterial()) {
        GLint hasAdvancedLoc = glGetUniformLocation(shaderProgram, "u_hasAdvancedMaterial");
        if (hasAdvancedLoc != -1) {
            glUniform1i(hasAdvancedLoc, 1);
        }
        
        // Set type-specific parameters
        switch (materialType) {
            case MaterialType::DISNEY_BRDF:
                setDisneyBRDFUniforms(shaderProgram);
                break;
            case MaterialType::METAL_MATERIAL:
                setMetalMaterialUniforms(shaderProgram);
                break;
            case MaterialType::GLASS_MATERIAL:
                setGlassMaterialUniforms(shaderProgram);
                break;
            case MaterialType::SUBSURFACE_MATERIAL:
                setSubsurfaceMaterialUniforms(shaderProgram);
                break;
            case MaterialType::EMISSIVE_MATERIAL:
                setEmissiveMaterialUniforms(shaderProgram);
                break;
            default:
                break;
        }
    } else {
        GLint hasAdvancedLoc = glGetUniformLocation(shaderProgram, "u_hasAdvancedMaterial");
        if (hasAdvancedLoc != -1) {
            glUniform1i(hasAdvancedLoc, 0);
        }
    }
}

void Material::setDisneyBRDFUniforms(GLuint shaderProgram) const
{
    // Disney BRDF specific uniforms
    GLint subsurfaceLoc = glGetUniformLocation(shaderProgram, "u_disney_subsurface");
    GLint sheenLoc = glGetUniformLocation(shaderProgram, "u_disney_sheen");
    GLint sheenTintLoc = glGetUniformLocation(shaderProgram, "u_disney_sheenTint");
    GLint clearcoatLoc = glGetUniformLocation(shaderProgram, "u_disney_clearcoat");
    GLint clearcoatGlossLoc = glGetUniformLocation(shaderProgram, "u_disney_clearcoatGloss");
    GLint specularTintLoc = glGetUniformLocation(shaderProgram, "u_disney_specularTint");
    GLint transmissionLoc = glGetUniformLocation(shaderProgram, "u_disney_transmission");
    GLint iorLoc = glGetUniformLocation(shaderProgram, "u_disney_ior");
    
    // Set default Disney parameters (can be enhanced to read from AdvancedMaterial)
    if (subsurfaceLoc != -1) glUniform1f(subsurfaceLoc, 0.2f);
    if (sheenLoc != -1) glUniform1f(sheenLoc, 0.0f);
    if (sheenTintLoc != -1) glUniform1f(sheenTintLoc, 0.5f);
    if (clearcoatLoc != -1) glUniform1f(clearcoatLoc, 0.0f);
    if (clearcoatGlossLoc != -1) glUniform1f(clearcoatGlossLoc, 1.0f);
    if (specularTintLoc != -1) glUniform1f(specularTintLoc, 0.0f);
    if (transmissionLoc != -1) glUniform1f(transmissionLoc, 0.0f);
    if (iorLoc != -1) glUniform1f(iorLoc, 1.5f);
}

void Material::setMetalMaterialUniforms(GLuint shaderProgram) const
{
    // Metal material specific uniforms
    GLint etaLoc = glGetUniformLocation(shaderProgram, "u_metal_eta");
    GLint kLoc = glGetUniformLocation(shaderProgram, "u_metal_k");
    
    // Default metal properties (Gold)
    if (etaLoc != -1) {
        glm::vec3 eta(0.1431f, 0.3749f, 1.4424f);
        glUniform3fv(etaLoc, 1, glm::value_ptr(eta));
    }
    if (kLoc != -1) {
        glm::vec3 k(3.9831f, 2.3856f, 1.6038f);
        glUniform3fv(kLoc, 1, glm::value_ptr(k));
    }
}

void Material::setGlassMaterialUniforms(GLuint shaderProgram) const
{
    // Glass material specific uniforms
    GLint iorLoc = glGetUniformLocation(shaderProgram, "u_glass_ior");
    GLint transmissionLoc = glGetUniformLocation(shaderProgram, "u_glass_transmission");
    
    if (iorLoc != -1) glUniform1f(iorLoc, 1.5f);
    if (transmissionLoc != -1) glUniform1f(transmissionLoc, 0.95f);
}

void Material::setSubsurfaceMaterialUniforms(GLuint shaderProgram) const
{
    // Subsurface scattering uniforms
    GLint sigmaALoc = glGetUniformLocation(shaderProgram, "u_subsurface_sigmaA");
    GLint sigmaSLoc = glGetUniformLocation(shaderProgram, "u_subsurface_sigmaS");
    GLint scaleLoc = glGetUniformLocation(shaderProgram, "u_subsurface_scale");
    
    if (sigmaALoc != -1) {
        glm::vec3 sigmaA(0.0017f, 0.0025f, 0.0061f); // Skin absorption
        glUniform3fv(sigmaALoc, 1, glm::value_ptr(sigmaA));
    }
    if (sigmaSLoc != -1) {
        glm::vec3 sigmaS(2.55f, 3.21f, 3.77f); // Skin scattering
        glUniform3fv(sigmaSLoc, 1, glm::value_ptr(sigmaS));
    }
    if (scaleLoc != -1) glUniform1f(scaleLoc, 1.0f);
}

void Material::setEmissiveMaterialUniforms(GLuint shaderProgram) const
{
    // Emissive material uniforms
    GLint emissionLoc = glGetUniformLocation(shaderProgram, "u_emission_color");
    GLint emissionPowerLoc = glGetUniformLocation(shaderProgram, "u_emission_power");
    
    if (emissionLoc != -1) {
        glm::vec3 emission(1.0f, 0.8f, 0.6f); // Warm light
        glUniform3fv(emissionLoc, 1, glm::value_ptr(emission));
    }
    if (emissionPowerLoc != -1) glUniform1f(emissionPowerLoc, 2.0f);
}