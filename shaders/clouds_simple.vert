#version 420 core

layout (location = 0) in vec3 aPos;

out vec3 FragPos;
out vec3 ViewRay;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;

void main() {
    FragPos = (model * vec4(aPos, 1.0)).xyz;
    ViewRay = FragPos - viewPos;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}