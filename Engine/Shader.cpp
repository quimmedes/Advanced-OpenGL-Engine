#include "Shader.hpp"

Shader::Shader() : vertexShader(0), fragmentShader(0), shaderProgram(0)
{
}

Shader::~Shader()
{
    cleanup();
}

bool Shader::Init(const char* vertexSource, const char* fragmentSource)
{
    cleanup();
    
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    if (!checkCompileErrors(vertexShader, "VERTEX")) {
        return false;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    if (!checkCompileErrors(fragmentShader, "FRAGMENT")) {
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    if (!checkCompileErrors(shaderProgram, "PROGRAM")) {
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    vertexShader = 0;
    fragmentShader = 0;

    return true;
}

void Shader::use() const
{
    if (shaderProgram) {
        glUseProgram(shaderProgram);
    }
}

void Shader::cleanup()
{
    if (vertexShader) {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }
    if (fragmentShader) {
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

std::string Shader::loadShaderFromFile(const std::string& filePath)
{
    std::string shaderCode;
    std::ifstream shaderFile;
    
    // Ensure ifstream objects can throw exceptions
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        // Open files
        shaderFile.open(filePath);
        std::stringstream shaderStream;
        
        // Read file's buffer contents into streams
        shaderStream << shaderFile.rdbuf();
        
        // Close file handlers
        shaderFile.close();
        
        // Convert stream into string
        shaderCode = shaderStream.str();
        
        std::cout << "Successfully loaded shader: " << filePath << std::endl;
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << filePath << std::endl;
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
    return shaderCode;
}

bool Shader::InitFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexCode = loadShaderFromFile(vertexPath);
    std::string fragmentCode = loadShaderFromFile(fragmentPath);
    
    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cerr << "ERROR::SHADER::FAILED_TO_LOAD_SHADER_FILES" << std::endl;
        return false;
    }
    
    return Init(vertexCode.c_str(), fragmentCode.c_str());
}

bool Shader::checkCompileErrors(GLuint shader, const std::string& type)
{
    GLint success;
    GLchar infoLog[1024];
    
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << std::endl;
            return false;
        }
    }
    return true;
}
