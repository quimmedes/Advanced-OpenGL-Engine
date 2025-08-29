#include "Mesh.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>

Mesh::Mesh() : vao(0), vbo(0), ebo(0), isLoaded(false)
{
    material = std::make_unique<Material>();
    std::cout << "Creating default material with simple shader..." << std::endl;
    if (!material->InitWithShader("shaders/simple.vert", "shaders/simple.frag")) {
        std::cout << "Simple shader failed, trying PBR..." << std::endl;
        if (!material->Init()) {
            std::cout << "❌ Failed to initialize any shader in Mesh constructor!" << std::endl;
        }
    }
    std::cout << "Mesh constructor - material shader ID: " << material->shader.shaderProgram << std::endl;
}

Mesh::Mesh(const std::string& filepath) : vao(0), vbo(0), ebo(0), isLoaded(false)
{
    material = std::make_unique<Material>();
    std::cout << "Creating material for file: " << filepath << std::endl;
    if (!material->InitWithShader("shaders/simple.vert", "shaders/simple.frag")) {
        std::cout << "Simple shader failed, trying PBR..." << std::endl;
        if (!material->Init()) {
            std::cout << "❌ Failed to initialize any shader in filepath constructor!" << std::endl;
        }
    }
    std::cout << "Filepath constructor - material shader ID: " << material->shader.shaderProgram << std::endl;
    loadFromFile(filepath);
}

Mesh::~Mesh()
{
    cleanup();
}

bool Mesh::loadFromFile(const std::string& filepath)
{
    cleanup();
    
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate | 
        aiProcess_GenNormals | 
        aiProcess_JoinIdenticalVertices);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return false;
    }

    std::cout << "Scene has " << scene->mNumTextures << " embedded textures" << std::endl;

    if (!scene->HasMeshes()) {
        std::cerr << "ERROR::MESH:: No meshes found in file: " << filepath << std::endl;
        return false;
    }
    
    vertices.clear();
    indices.clear();
    unsigned int vertexOffset = 0;
    
    // Extract directory for texture loading
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        modelDirectory = filepath.substr(0, lastSlash);
    } else {
        modelDirectory = ".";
    }
    
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        processMesh(scene->mMeshes[i], scene);
    }
    
    if (vertices.empty() || indices.empty()) {
        std::cerr << "ERROR::MESH:: No valid mesh data loaded from: " << filepath << std::endl;
        return false;
    }
    
    setupMesh();
    isLoaded = true;
    return true;
}

void Mesh::processMesh(aiMesh* mesh, const aiScene* scene)
{
    unsigned int vertexOffset = static_cast<unsigned int>(vertices.size());
    
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
        vertex.position.x = mesh->mVertices[i].x;
        vertex.position.y = mesh->mVertices[i].y;
        vertex.position.z = mesh->mVertices[i].z;
        
        if (mesh->HasNormals()) {
            vertex.normal.x = mesh->mNormals[i].x;
            vertex.normal.y = mesh->mNormals[i].y;
            vertex.normal.z = mesh->mNormals[i].z;
        } else {
            vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        
        if (mesh->mTextureCoords[0]) {
            vertex.texCoords.x = mesh->mTextureCoords[0][i].x;
            vertex.texCoords.y = mesh->mTextureCoords[0][i].y;
            
            // Debug: Print first few UV coordinates (commented out for performance)
            // if (i < 5) {
            //     std::cout << "UV[" << i << "]: (" << vertex.texCoords.x << ", " << vertex.texCoords.y << ")" << std::endl;
            // }
        } else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
            std::cout << "Warning: Mesh has no texture coordinates!" << std::endl;
        }
        
        vertices.push_back(vertex);
    }
    
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(vertexOffset + face.mIndices[j]);
        }
    }
    
    // Load material if available
    std::cout << "Mesh material index: " << mesh->mMaterialIndex << std::endl;
    std::cout << "Scene has " << scene->mNumMaterials << " materials" << std::endl;
    
    if (mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < (int)scene->mNumMaterials) {
        std::cout << "🔄 Loading material " << mesh->mMaterialIndex << " from scene" << std::endl;
        // Pass scene pointer to material loader so it can access embedded textures
        loadMaterialFromAssimp(scene->mMaterials[mesh->mMaterialIndex], scene, modelDirectory);
        std::cout << "✅ Material loading complete" << std::endl;
    } else {
        std::cout << "❌ No valid material index for this mesh (index: " << mesh->mMaterialIndex << ", available: " << scene->mNumMaterials << ")" << std::endl;
    }
}

void Mesh::setupMesh()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void Mesh::render() const
{
    if (!isValid()) return;
    
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::cleanup()
{
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (ebo) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    vertices.clear();
    indices.clear();
    isLoaded = false;
}

bool Mesh::fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

void Mesh::loadMaterialFromAssimp(const aiMaterial* assimpMaterial, const aiScene* scene, const std::string& directory)
{
    if (!assimpMaterial) {
        std::cout << "ERROR: No material provided to loadMaterialFromAssimp" << std::endl;
        return;
    }
    
    std::cout << "\n=== DEBUGGING ASSIMP MATERIAL LOADING ===" << std::endl;
    std::cout << "Directory: " << directory << std::endl;
    
    // Print material name if available
    aiString materialName;
    if (assimpMaterial->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS) {
        std::cout << "Material name: " << materialName.C_Str() << std::endl;
    }
    
    // Print all available properties
    std::cout << "Material has " << assimpMaterial->mNumProperties << " properties:" << std::endl;
    for (unsigned int i = 0; i < assimpMaterial->mNumProperties; ++i) {
        aiMaterialProperty* prop = assimpMaterial->mProperties[i];
        std::cout << "  Property " << i << ": " << prop->mKey.C_Str() << std::endl;
    }
    
    // Load diffuse color
    aiColor3D diffuseColor;
    if (assimpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS) {
        glm::vec3 colorVec = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
        material->setAlbedo(colorVec);
        std::cout << "✅ Loaded diffuse color: (" << diffuseColor.r << ", " << diffuseColor.g << ", " << diffuseColor.b << ")" << std::endl;
        
        // Check if color is too dark or white
        if (colorVec.r < 0.01f && colorVec.g < 0.01f && colorVec.b < 0.01f) {
            std::cout << "⚠️  Color is very dark - setting to bright blue for visibility" << std::endl;
            material->setAlbedo(glm::vec3(0.0f, 0.5f, 1.0f)); // Bright blue
        } else if (colorVec.r > 0.7f && colorVec.g > 0.7f && colorVec.b > 0.7f) {
            std::cout << "⚠️  Color is white - setting to material-specific color for visibility" << std::endl;
            // Use different colors based on material name for variety
            if (assimpMaterial) {
                aiString materialName;
                if (assimpMaterial->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS) {
                    std::string name = materialName.C_Str();
                    if (name.find("SWORD") != std::string::npos) {
                        material->setAlbedo(glm::vec3(0.8f, 0.8f, 0.9f)); // Silver
                    } else if (name.find("shield") != std::string::npos || name.find("Crest") != std::string::npos) {
                        material->setAlbedo(glm::vec3(0.6f, 0.3f, 0.1f)); // Brown
                    } else {
                        material->setAlbedo(glm::vec3(0.2f, 0.6f, 0.9f)); // Blue
                    }
                } else {
                    material->setAlbedo(glm::vec3(0.2f, 0.6f, 0.9f)); // Default blue
                }
            }
        }
    } else {
        std::cout << "❌ Failed to load diffuse color - setting bright orange as default" << std::endl;
        material->setAlbedo(glm::vec3(1.0f, 0.5f, 0.0f)); // Bright orange as fallback
    }
    
    // Load metallic factor
    float metallicFactor = 0.0f;
    if (assimpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS) {
        material->setMetallic(metallicFactor);
        std::cout << "✅ Loaded metallic factor: " << metallicFactor << std::endl;
    } else {
        std::cout << "❌ Failed to load metallic factor" << std::endl;
    }
    
    // Load roughness factor
    float roughnessFactor = 0.5f;
    if (assimpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS) {
        material->setRoughness(roughnessFactor);
        std::cout << "✅ Loaded roughness factor: " << roughnessFactor << std::endl;
    } else {
        std::cout << "❌ Failed to load roughness factor" << std::endl;
    }
    
    // Check all texture types that might be available
    // Check for embedded textures first
    std::cout << "\n--- CHECKING EMBEDDED TEXTURES ---" << std::endl;
    std::cout << "Scene has " << scene->mNumTextures << " embedded textures" << std::endl;
    for (unsigned int i = 0; i < scene->mNumTextures; ++i) {
        aiTexture* embeddedTex = scene->mTextures[i];
        std::cout << "  Embedded texture " << i << ": " << embeddedTex->mFilename.C_Str() 
                  << " (" << embeddedTex->mWidth << "x" << embeddedTex->mHeight << ")" << std::endl;
    }
    
    std::cout << "\n--- CHECKING ALL TEXTURE TYPES ---" << std::endl;
    
    const aiTextureType textureTypes[] = {
        aiTextureType_DIFFUSE, aiTextureType_SPECULAR, aiTextureType_AMBIENT, aiTextureType_EMISSIVE,
        aiTextureType_HEIGHT, aiTextureType_NORMALS, aiTextureType_SHININESS, aiTextureType_OPACITY,
        aiTextureType_DISPLACEMENT, aiTextureType_LIGHTMAP, aiTextureType_REFLECTION,
        aiTextureType_BASE_COLOR, aiTextureType_NORMAL_CAMERA, aiTextureType_EMISSION_COLOR,
        aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_AMBIENT_OCCLUSION,
        aiTextureType_UNKNOWN
    };
    
    const char* textureTypeNames[] = {
        "DIFFUSE", "SPECULAR", "AMBIENT", "EMISSIVE", "HEIGHT", "NORMALS", "SHININESS", "OPACITY",
        "DISPLACEMENT", "LIGHTMAP", "REFLECTION", "BASE_COLOR", "NORMAL_CAMERA", "EMISSION_COLOR",
        "METALNESS", "DIFFUSE_ROUGHNESS", "AMBIENT_OCCLUSION", "UNKNOWN"
    };
    
    for (int i = 0; i < 18; ++i) {
        unsigned int textureCount = assimpMaterial->GetTextureCount(textureTypes[i]);
        if (textureCount > 0) {
            std::cout << textureTypeNames[i] << " textures: " << textureCount << std::endl;
            for (unsigned int j = 0; j < textureCount; ++j) {
                aiString texPath;
                if (assimpMaterial->GetTexture(textureTypes[i], j, &texPath) == AI_SUCCESS) {
                    std::cout << "  [" << j << "] Path: " << texPath.C_Str() << std::endl;
                }
            }
        }
    }
    
    std::cout << "\n--- LOADING TEXTURES ---" << std::endl;
    
    // Load diffuse texture
    aiString diffuseTexturePath;
    if (assimpMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &diffuseTexturePath) == AI_SUCCESS) {
        std::string texPath = std::string(diffuseTexturePath.C_Str());
        std::cout << "Found diffuse texture: " << texPath << std::endl;
        
        // Handle both absolute and relative paths
        std::vector<std::string> pathsToTry;
        
        // Check if path is absolute (starts with drive letter on Windows or / on Unix)
        if ((texPath.length() >= 3 && texPath[1] == ':') || texPath[0] == '/') {
            // Absolute path - try as-is first, then extract filename
            pathsToTry.push_back(texPath);
            size_t lastSlash = texPath.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                std::string filename = texPath.substr(lastSlash + 1);
                pathsToTry.push_back(directory + "/" + filename);
                pathsToTry.push_back(directory + "\\" + filename);
                pathsToTry.push_back(filename);
            }
        } else {
            // Relative path - try with directory prefixes
            pathsToTry = {
                directory + "/" + texPath,
                directory + "\\" + texPath,
                texPath,
                directory + "/../" + texPath,
                directory + "\\..\\" + texPath
            };
        }
        
        bool loaded = false;
        for (const auto& path : pathsToTry) {
            std::cout << "Trying path: " << path << std::endl;
            if (fileExists(path)) {
                material->setDiffuseTexture(path);
                std::cout << "✅ Loaded diffuse texture from: " << path << std::endl;
                loaded = true;
                break;
            }
        }
        
        if (!loaded) {
            std::cout << "❌ Failed to find diffuse texture in any location" << std::endl;
        }
    } else {
        std::cout << "❌ No diffuse texture found in material" << std::endl;
    }
    
    // Load base color texture (PBR alternative to diffuse)
    aiString baseColorTexturePath;
    if (assimpMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &baseColorTexturePath) == AI_SUCCESS && !material->hasDiffuseTexture()) {
        std::string texPath = std::string(baseColorTexturePath.C_Str());
        std::cout << "Found base color texture: " << texPath << std::endl;
        
        std::vector<std::string> pathsToTry = {
            directory + "/" + texPath,
            directory + "\\" + texPath,
            texPath
        };
        
        for (const auto& path : pathsToTry) {
            if (fileExists(path)) {
                material->setDiffuseTexture(path);
                std::cout << "✅ Loaded base color texture as diffuse: " << path << std::endl;
                break;
            }
        }
    }
    
    // Load normal texture
    aiString normalTexturePath;
    if (assimpMaterial->GetTexture(aiTextureType_NORMALS, 0, &normalTexturePath) == AI_SUCCESS) {
        std::string texPath = std::string(normalTexturePath.C_Str());
        std::vector<std::string> pathsToTry = {
            directory + "/" + texPath,
            directory + "\\" + texPath,
            texPath
        };
        
        for (const auto& path : pathsToTry) {
            if (fileExists(path)) {
                material->setNormalTexture(path);
                std::cout << "✅ Loaded normal texture: " << path << std::endl;
                break;
            }
        }
    }
    
    // Ensure the material has a proper shader loaded
    if (!material->shader.shaderProgram) {
        std::cout << "Material shader not initialized - trying simple shader first" << std::endl;
        if (!material->InitWithShader("shaders/simple.vert", "shaders/simple.frag")) {
            std::cout << "❌ Failed to load simple shader, trying PBR..." << std::endl;
            if (!material->Init()) {
                std::cout << "❌ Failed to initialize any material shader!" << std::endl;
            } else {
                std::cout << "✅ PBR shader loaded as fallback" << std::endl;
            }
        } else {
            std::cout << "✅ Simple shader loaded successfully" << std::endl;
        }
    } else {
        std::cout << "✅ Material already has shader program: " << material->shader.shaderProgram << std::endl;
    }
    
    // Final material state check
    std::cout << "\n--- FINAL MATERIAL STATE ---" << std::endl;
    std::cout << "Albedo: (" << material->getAlbedo().r << ", " << material->getAlbedo().g << ", " << material->getAlbedo().b << ")" << std::endl;
    std::cout << "Metallic: " << material->getMetallic() << std::endl;
    std::cout << "Roughness: " << material->getRoughness() << std::endl;
    std::cout << "Has Diffuse Texture: " << (material->hasDiffuseTexture() ? "Yes" : "No") << std::endl;
    std::cout << "Has Normal Texture: " << (material->hasNormalTexture() ? "Yes" : "No") << std::endl;
    std::cout << "Material Type: " << (int)material->getMaterialType() << std::endl;
    std::cout << "Shader Program ID: " << material->shader.shaderProgram << std::endl;
    
    std::cout << "=== MATERIAL LOADING DEBUG COMPLETE ===" << std::endl;
    
    // Add a summary line for easy spotting
}
