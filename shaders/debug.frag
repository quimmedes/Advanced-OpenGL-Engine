#version 420 core

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
out vec4 FragColor;

uniform vec3 material_albedo;
uniform sampler2D material_diffuseTexture;
uniform bool material_hasDiffuseTexture;

void main()
{
    // Debug different aspects based on FragPos
    float x = gl_FragCoord.x / 1940.0;
    
    if (x < 0.25) {
        // Show UV coordinates as color
        FragColor = vec4(TexCoords, 0.0, 1.0);
    }
    else if (x < 0.5) {
        // Show normals as color
        vec3 norm = normalize(Normal);
        FragColor = vec4(norm * 0.5 + 0.5, 1.0);
    }
    else if (x < 0.75) {
        // Show texture sampling
        if(material_hasDiffuseTexture) {
            vec3 texColor = texture(material_diffuseTexture, TexCoords).rgb;
            FragColor = vec4(texColor, 1.0);
        } else {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red if no texture
        }
    }
    else {
        // Show material albedo
        FragColor = vec4(material_albedo, 1.0);
    }
}