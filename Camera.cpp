#include "Camera.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp> 


Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position), worldUp(up), yaw(yaw), pitch(pitch),
      movementSpeed(30.0f), mouseSensitivity(0.15f), zoom(60.0f), firstMouse(true)
{
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}


glm::mat4 Camera::getProjectionMatrix(float aspect, float n, float f) const  
{  
	return glm::perspective(glm::radians(zoom), aspect, n, f);
}

void Camera::processKeyboard(float deltaTime)
{
    glm::vec3 move(0.0f);
    
    if (GetAsyncKeyState('W') & 0x8000)
        move += front * movementSpeed * deltaTime;
    if (GetAsyncKeyState('S') & 0x8000)
        move -= front * movementSpeed * deltaTime;
    if (GetAsyncKeyState('A') & 0x8000)
        move -= glm::normalize(glm::cross(front, up)) * movementSpeed * deltaTime;
    if (GetAsyncKeyState('D') & 0x8000)
        move += glm::normalize(glm::cross(front, up)) * movementSpeed * deltaTime;
    if (GetAsyncKeyState('E') & 0x8000)
        move += up * movementSpeed * deltaTime;
    if (GetAsyncKeyState('Q') & 0x8000)
        move -= up * movementSpeed * deltaTime;
    
    if (glm::length(move) > 0.0f)
        position += move;
}

void Camera::processMouseMovement(HWND hWnd)
{
    POINT currentPos;
    GetCursorPos(&currentPos);
    
    RECT winRect;
    GetClientRect(hWnd, &winRect);
    POINT center;
    center.x = (winRect.right - winRect.left) / 2;
    center.y = (winRect.bottom - winRect.top) / 2;
    ClientToScreen(hWnd, &center);
    
    if (!firstMouse) {
        float xoffset = float(currentPos.x - center.x) * mouseSensitivity;
        float yoffset = float(center.y - currentPos.y) * mouseSensitivity;
        
        yaw += xoffset;
        pitch += yoffset;
        
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
        
        updateCameraVectors();
    }
    firstMouse = false;
    SetCursorPos(center.x, center.y);
}

void Camera::updateCameraVectors()
{
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
