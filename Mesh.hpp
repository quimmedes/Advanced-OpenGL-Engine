#pragma once  
#include "Transform.hpp"  
#include "Vertex.hpp"
#include "Material.hpp"
#include <GL/glew.h>
#include <vector>
#include <string>
#include <memory>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

class Mesh : public Transform  
{  
private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint vao, vbo, ebo;
    bool isLoaded;
    std::unique_ptr<Material> material;
    std::string modelDirectory;
    
    void setupMesh();
    void loadMesh(const std::string& filepath);
    void processMesh(aiMesh* mesh, const aiScene* scene);
    void loadMaterialFromAssimp(const aiMaterial* assimpMaterial, const aiScene* scene, const std::string& directory);
    bool fileExists(const std::string& path);

public:  
    Mesh();
    Mesh(const std::string& filepath);
    ~Mesh();
    
    bool loadFromFile(const std::string& filepath);
    void render() const;
    void cleanup();
    
    GLsizei getIndexCount() const { return static_cast<GLsizei>(indices.size()); }
    bool isValid() const { return isLoaded && !indices.empty(); }
    
    Material* getMaterial() const { return material.get(); }
    void setMaterial(std::unique_ptr<Material> mat) { material = std::move(mat); }
};
