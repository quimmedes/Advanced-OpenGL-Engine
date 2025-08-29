#include "OpenGL.hpp"
#include "WindowWin.hpp"
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <memory>

extern HWND hWndGlobal;

OpenGL::OpenGL() : deltaTime(0.0f), frameCount(0), fps(0.0f)
{
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevFrameTime);
    lastFPSTime = GetTickCount64();
}

OpenGL::~OpenGL()
{
}

bool OpenGL::Init() {
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    glEnable(GL_DEPTH_TEST);
    
    typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) wglSwapIntervalEXT(0);
    
    ShowCursor(FALSE);
    
    return true;
}


void OpenGL::updateDeltaTime() {
    QueryPerformanceCounter(&currentFrameTime);
    deltaTime = static_cast<float>(currentFrameTime.QuadPart - prevFrameTime.QuadPart) / frequency.QuadPart;
    prevFrameTime = currentFrameTime;
}

void OpenGL::updateFPS(HWND hWnd) {
    frameCount++;
    DWORD now = GetTickCount64();
    if (now - lastFPSTime >= 1000) {
        fps = frameCount * 1000.0 / double(now - lastFPSTime);
        frameCount = 0;
        lastFPSTime = now;
        
        char fpsText[64];
        sprintf_s(fpsText, "FPS: %.1f", fps);
        SetWindowTextA(hWnd, fpsText);
    }
}

void OpenGL::setLightUniforms(Material* material, Camera* camera, const std::vector<std::unique_ptr<Light>>& lights) {
    GLint viewPosLoc = glGetUniformLocation(material->shader.shaderProgram, "viewPos");
    if (viewPosLoc != -1) glUniform3fv(viewPosLoc, 1, glm::value_ptr(camera->getPosition()));
    
    // Check for simple shader lighting uniforms first
    GLint lightPosLoc = glGetUniformLocation(material->shader.shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(material->shader.shaderProgram, "lightColor");
    
    if (lightPosLoc != -1 && lightColorLoc != -1 && !lights.empty()) {
        // Simple shader - use first enabled light
        for (const auto& light : lights) {
            if (light->enabled) {
                glm::vec3 lightPos = light->position;
                glm::vec3 lightColor = light->color * light->intensity;
                
                glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
                glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
                
                break;
            }
        }
        return; // Don't set PBR uniforms if we're using simple shader
    }
    
    // Set number of lights uniforms for PBR shaders
    GLint numDirLightsLoc = glGetUniformLocation(material->shader.shaderProgram, "numDirLights");
    GLint numPointLightsLoc = glGetUniformLocation(material->shader.shaderProgram, "numPointLights");
    GLint numSpotLightsLoc = glGetUniformLocation(material->shader.shaderProgram, "numSpotLights");
    
    int dirLightCount = 0, pointLightCount = 0, spotLightCount = 0;
    
    // Count lights by type and set their uniforms
    for (const auto& light : lights) {
        if (!light->enabled) continue;
        
        switch (light->getType()) {
            case LightType::DIRECTIONAL:
                light->setUniforms(material->shader.shaderProgram, dirLightCount);
                dirLightCount++;
                break;
            case LightType::POINT:
                light->setUniforms(material->shader.shaderProgram, pointLightCount);
                pointLightCount++;
                break;
            case LightType::SPOT:
                light->setUniforms(material->shader.shaderProgram, spotLightCount);
                spotLightCount++;
                break;
        }
    }
    
    // Set light counts
    if (numDirLightsLoc != -1) glUniform1i(numDirLightsLoc, dirLightCount);
    if (numPointLightsLoc != -1) glUniform1i(numPointLightsLoc, pointLightCount);
    if (numSpotLightsLoc != -1) glUniform1i(numSpotLightsLoc, spotLightCount);
}

void OpenGL::Render(Camera* camera, const std::vector<std::unique_ptr<Mesh>>& meshes, 
                    const std::vector<std::unique_ptr<Light>>& lights, 
                    Ocean* ocean, CloudSystem* cloudSystem,
                    OceanCG* oceanCG, CloudsCG* cloudsCG, OceanFFT* oceanFFT) {
    if (!camera) return;
    
    updateDeltaTime();
    updateFPS(hWndGlobal);
    
    camera->processKeyboard(deltaTime);
    camera->processMouseMovement(hWndGlobal);
    
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f); // Bright daytime sky blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = camera->getProjectionMatrix(1940.0f / 1080.0f);
    
    // Extract lighting information for environmental systems
    glm::vec3 lightDir = glm::vec3(0.0f, -1.0f, 0.0f);  // Default downward light
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f); // Default white light
    glm::vec3 skyColor = glm::vec3(0.6f, 0.8f, 1.0f);   // Sky blue
    
    // Get actual light information from the first directional light
    for (const auto& light : lights) {
        if (light->enabled && light->getType() == LightType::DIRECTIONAL) {
            lightDir = dynamic_cast<DirectionalLight*>(light.get())->getDirection();
            lightColor = light->color * light->intensity;
            break;
        }
    }
    
    // Render regular meshes first (opaque objects)
    for (const auto& mesh : meshes) {
        if (mesh->isValid() && mesh->getMaterial()) {
            Material* material = mesh->getMaterial();
            
            glUseProgram(material->shader.shaderProgram);
            
            GLint viewLoc = glGetUniformLocation(material->shader.shaderProgram, "view");
            GLint projLoc = glGetUniformLocation(material->shader.shaderProgram, "projection");
            GLint modelLoc = glGetUniformLocation(material->shader.shaderProgram, "model");
            
            if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
            
            // Use advanced material uniforms if available, otherwise fall back to basic
            material->setUniformsAdvanced(material->shader.shaderProgram);
            material->bindTextures();
            setLightUniforms(material, camera, lights);
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, mesh->position);
            model = glm::rotate(model, glm::radians(mesh->rotation.x), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(mesh->rotation.y), glm::vec3(0, 1, 0));
            model = glm::rotate(model, glm::radians(mesh->rotation.z), glm::vec3(0, 0, 1));
            model = glm::scale(model, mesh->scale);
            
            if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            mesh->render();
        }
    }
    
    // Render transparent ocean after all opaque objects (proper transparency order)
    if (ocean && ocean->IsInitialized()) {
        ocean->Render(view, projection, camera->getPosition(), lightDir, lightColor, skyColor);
    }
    
    // Render book-based ocean (transparent)
    if (oceanCG && oceanCG->IsInitialized()) {
        oceanCG->Render(view, projection);
    }
    
    // Render FFT-based ocean (transparent, most advanced)
    if (oceanFFT && oceanFFT->IsInitialized()) {
        oceanFFT->Render(view, projection, camera->getPosition(), lightDir, lightColor, skyColor);
    }
    
    // Render volumetric clouds last (skybox, rendered after transparent objects)
    if (cloudSystem && cloudSystem->IsInitialized()) {
        cloudSystem->Render(view, projection, camera->getPosition(), lightDir, lightColor, skyColor);
    }
    
    // Render book-based clouds (skybox)
    if (cloudsCG && cloudsCG->IsInitialized()) {
        cloudsCG->RenderSkybox(view, projection);
    }
    
    SwapBuffers(hDCGlobal);
}