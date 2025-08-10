#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertexColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 objectColor;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor * objectColor;
    
    // Set point size for stars (when rendering as GL_POINTS)
    // Vary star size based on brightness for more realism
    float brightness = (aColor.r + aColor.g + aColor.b) / 3.0;
    gl_PointSize = 1.0 + brightness * 3.0; // Size between 1-4 pixels
}