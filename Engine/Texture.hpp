#pragma once
#include <GL/glew.h>
#include <string>

class Texture
{
private:
    GLuint textureID;
    std::string filePath;
    int width, height, channels;
    bool isLoaded;
    
    // Helper method for creating fallback textures when image loading fails
    bool createFallbackTexture(const std::string& path);

public:
    Texture();
    Texture(const std::string& filePath);
    ~Texture();
    
    bool loadFromFile(const std::string& filePath);
    void bind(GLenum textureUnit = GL_TEXTURE0) const;
    void unbind() const;
    void cleanup();
    
    GLuint getID() const { return textureID; }
    bool isValid() const { return isLoaded && textureID != 0; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getChannels() const { return channels; }
    const std::string& getFilePath() const { return filePath; }
};