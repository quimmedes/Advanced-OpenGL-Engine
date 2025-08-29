#pragma once
#include <GL/glew.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class Shader
{
private:
	GLuint vertexShader;
	GLuint fragmentShader;
	
	bool checkCompileErrors(GLuint shader, const std::string& type);
	std::string loadShaderFromFile(const std::string& filePath);
	
public:
	GLuint shaderProgram;
	
	Shader();
	~Shader();
	
	bool Init(const char* vertexSource, const char* fragmentSource);
	bool InitFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
	void use() const;
	void cleanup();
};

