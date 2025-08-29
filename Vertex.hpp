#pragma once
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    
    Vertex() : position(0.0f), normal(0.0f, 0.0f, 1.0f), texCoords(0.0f) {}
    
    Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& tex)
        : position(pos), normal(norm), texCoords(tex) {}
};