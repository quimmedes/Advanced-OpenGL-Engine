#version 430

layout (location=0) in vec3 vertPos;

uniform mat4 mv_matrix;
uniform mat4 proj_matrix;

out vec3 tc;

void main(void)
{
    // Skybox technique from the book - Chapter on Environment Mapping
    // Remove translation from the view matrix for skybox effect
    mat4 v3_matrix = mat4(mat3(mv_matrix));
    vec4 pos = proj_matrix * v3_matrix * vec4(vertPos, 1.0);
    
    // Ensure the skybox is always at maximum depth
    gl_Position = pos.xyww;
    
    // Use vertex position as texture coordinate for procedural generation
    tc = vertPos;
}