#pragma once
#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>
#include <memory>
#include <iostream>
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Light.hpp"
#include "Ocean.hpp"
#include "CloudSystem.hpp"
#include "OceanCG.hpp"
#include "CloudsCG.hpp"
#include "OceanFFT.hpp"
#include <windows.h>
#include <glm/glm.hpp>

class OpenGL {
private:
    LARGE_INTEGER frequency, prevFrameTime, currentFrameTime;
    float deltaTime;
    
    DWORD lastFPSTime;
    int frameCount;
    double fps;
    
    void updateDeltaTime();
    void updateFPS(HWND hWnd);
    void setLightUniforms(Material* material, Camera* camera, const std::vector<std::unique_ptr<Light>>& lights);

public:
    OpenGL();
    ~OpenGL();
    
    bool Init();
    void Render(Camera* camera, const std::vector<std::unique_ptr<Mesh>>& meshes, 
                const std::vector<std::unique_ptr<Light>>& lights, 
                Ocean* ocean = nullptr, CloudSystem* cloudSystem = nullptr,
                OceanCG* oceanCG = nullptr, CloudsCG* cloudsCG = nullptr,
                OceanFFT* oceanFFT = nullptr);
    
    // Getter for delta time
    float getDeltaTime() const { return deltaTime; }
};