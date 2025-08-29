#version 430

layout (location=0) in vec3 vertPos;
layout (location=1) in vec3 vertNormal;
layout (location=2) in vec2 vertTexCoord;

out vec3 varyingNormal;
out vec3 varyingLightDir;
out vec3 varyingVertPos;
out vec3 varyingHalfVector;
out vec2 tc;
out vec4 screenPos;      // For depth buffer sampling
out vec3 worldPos;       // World position for depth calculations
out float waterDepth;    // Water depth for translucency

struct PositionalLight
{	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 position;
};
struct Material
{	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;
uniform mat4 mv_matrix;
uniform mat4 proj_matrix;
uniform mat4 norm_matrix;
uniform float time;

// Wave parameters from the book
uniform float waveHeight;
uniform float waveLength;
uniform float waveSpeed;

void main(void)
{	
    vec4 P = vec4(vertPos, 1.0);
    
    // Implement sinusoidal wave displacement as shown in the book
    // Chapter on procedural textures and displacement mapping
    float wave1 = sin((P.x / waveLength + time * waveSpeed)) * waveHeight;
    float wave2 = sin((P.z / waveLength + time * waveSpeed * 0.7)) * waveHeight * 0.5;
    P.y += wave1 + wave2;
    
    // Calculate normal based on wave derivatives (from book's normal calculation)
    float dx = cos((P.x / waveLength + time * waveSpeed)) * (waveHeight / waveLength);
    float dz = cos((P.z / waveLength + time * waveSpeed * 0.7)) * (waveHeight * 0.5 / waveLength);
    vec3 waveNormal = normalize(vec3(-dx, 1.0, -dz));
    
    varyingVertPos = (mv_matrix * P).xyz;
    varyingLightDir = light.position - varyingVertPos;
    varyingNormal = (norm_matrix * vec4(waveNormal, 1.0)).xyz;
    varyingHalfVector = normalize(normalize(varyingLightDir) + normalize(-varyingVertPos));
    
    // Calculate additional outputs for depth-based effects
    worldPos = P.xyz;
    gl_Position = proj_matrix * mv_matrix * P;
    screenPos = gl_Position;
    
    // Calculate water depth (distance from original sea level)
    waterDepth = max(0.0, -worldPos.y);  // Assume sea level at y=0
    
    tc = vertTexCoord;
}