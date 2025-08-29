#include "Texture.hpp"
#include <iostream>

// Include STB_IMAGE header - it should be available via vcpkg
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture::Texture() : textureID(0), width(0), height(0), channels(0), isLoaded(false)
{
}

Texture::Texture(const std::string& filePath) : textureID(0), width(0), height(0), channels(0), isLoaded(false)
{
    loadFromFile(filePath);
}

Texture::~Texture()
{
    cleanup();
}

bool Texture::loadFromFile(const std::string& path)
{
    cleanup();
    this->filePath = path;
    
    // Set STB_IMAGE to flip textures on Y axis to match OpenGL coordinate system
    stbi_set_flip_vertically_on_load(true);
    
    // Load image data using STB_IMAGE
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "ERROR::TEXTURE::FAILED_TO_LOAD: " << path << std::endl;
        std::cerr << "STB_IMAGE Error: " << stbi_failure_reason() << std::endl;
        
        // Create a fallback debug texture
        return createFallbackTexture(path);
    }
    
    // Generate OpenGL texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Determine the correct format based on number of channels
    GLenum format, internalFormat;
    switch (channels) {
        case 1:
            format = GL_RED;
            internalFormat = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            internalFormat = GL_RGB;
            break;
        case 4:
            format = GL_RGBA;
            internalFormat = GL_RGBA;
            break;
        default:
            std::cerr << "ERROR::TEXTURE::UNSUPPORTED_CHANNEL_COUNT: " << channels << " in " << path << std::endl;
            stbi_image_free(data);
            return createFallbackTexture(path);
    }
    
    // Upload texture data to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Free image data from memory
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    isLoaded = true;
    std::cout << "Successfully loaded texture: " << path 
              << " (" << width << "x" << height << ", " << channels << " channels, ID: " << textureID << ")" << std::endl;
    
    return true;
}

bool Texture::createFallbackTexture(const std::string& path)
{
    // Generate texture for fallback
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Create fallback texture data
    width = 256;
    height = 256;
    channels = 3;
    
    unsigned char* data = new unsigned char[width * height * channels];
    
    // Create distinctive patterns based on texture type for debugging
    if (path.find("diffuse") != std::string::npos || path.find("Terrain") != std::string::npos) {
        // Red and green checkerboard for diffuse textures
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = (y * width + x) * channels;
                bool checker = ((x / 32) + (y / 32)) % 2;
                if (checker) {
                    data[index] = 255;     // Red
                    data[index + 1] = 0;   // No green
                    data[index + 2] = 0;   // No blue
                } else {
                    data[index] = 0;       // No red
                    data[index + 1] = 255; // Green
                    data[index + 2] = 0;   // No blue
                }
            }
        }
    }
    else if (path.find("normal") != std::string::npos) {
        // Blue for normal maps (neutral normal = (0.5, 0.5, 1.0) in RGB)
        for (int i = 0; i < width * height * channels; i += channels) {
            data[i] = 128;      // 0.5 in [0,1] range
            data[i + 1] = 128;  // 0.5 in [0,1] range  
            data[i + 2] = 255;  // 1.0 in [0,1] range (pointing up)
        }
    }
    else if (path.find("specular") != std::string::npos) {
        // Yellow for specular maps
        for (int i = 0; i < width * height * channels; i += channels) {
            data[i] = 255;      // Red
            data[i + 1] = 255;  // Green (red + green = yellow)
            data[i + 2] = 0;    // No blue
        }
    }
    else {
        // Magenta for unknown texture types
        for (int i = 0; i < width * height * channels; i += channels) {
            data[i] = 255;      // Red
            data[i + 1] = 0;    // No green
            data[i + 2] = 255;  // Blue (red + blue = magenta)
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    delete[] data;
    glBindTexture(GL_TEXTURE_2D, 0);
    
    isLoaded = true;
    std::cout << "Created fallback texture for: " << path << " (ID: " << textureID << ")" << std::endl;
    
    return true;
}

void Texture::bind(GLenum textureUnit) const
{
    if (isLoaded) {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
}

void Texture::unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::cleanup()
{
    if (textureID) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    isLoaded = false;
}