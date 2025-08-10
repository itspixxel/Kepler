#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

out vec3 FragPos;        // World space position
out vec3 Normal;         // World space normal
out vec3 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normalMatrix;  // For transforming normals
uniform vec3 objectColor;
uniform vec3 cameraPos;
uniform bool isPoint;       // True for stars

void main() {
    if (isPoint) {
        // For point rendering (stars)
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        vertexColor = aColor * objectColor;
        
        // Set point size for stars
        float brightness = (aColor.r + aColor.g + aColor.b) / 3.0;
        float baseSize = 1.0 + brightness * 3.0;
        float distance = length((model * vec4(aPos, 1.0)).xyz - cameraPos);
        gl_PointSize = clamp(baseSize * 60.0 / distance, 1.0, 8.0);
    } else {
        // For sphere rendering (planets/moons/sun)
        // Transform position to world space
        FragPos = vec3(model * vec4(aPos, 1.0));
        
        // Transform normal to world space
        Normal = mat3(normalMatrix) * aNormal;
        
        gl_Position = projection * view * vec4(FragPos, 1.0);
        vertexColor = aColor * objectColor;
    }
}