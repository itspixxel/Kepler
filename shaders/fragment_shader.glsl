#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 vertexColor;

out vec4 FragColor;

// Lighting uniforms
uniform vec3 lightPos;        // Sun position (0,0,0)
uniform vec3 lightColor;      // Sun light color
uniform vec3 cameraPos;       // Camera position
uniform bool isEmissive;      // True for Sun, false for planets
uniform bool isPoint;         // True for stars, false for spheres

// Material properties
uniform float ambientStrength;
uniform float specularStrength;
uniform int shininess;

void main() {
    // For point rendering (stars), just use vertex color
    if (isPoint) {
        FragColor = vec4(vertexColor, 1.0);
        return;
    }
    
    // For emissive objects (Sun), just use vertex color
    if (isEmissive) {
        FragColor = vec4(vertexColor, 1.0);
        return;
    }
    
    // === PHONG LIGHTING MODEL ===
    
    // Normalize the normal vector
    vec3 norm = normalize(Normal);
    
    // Calculate light direction (from fragment to light source)
    vec3 lightDir = normalize(lightPos - FragPos);
    
    // === AMBIENT LIGHTING ===
    vec3 ambient = ambientStrength * lightColor;
    
    // === DIFFUSE LIGHTING ===
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // === SPECULAR LIGHTING ===
    vec3 viewDir = normalize(cameraPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // === DISTANCE ATTENUATION ===
    // Very gentle attenuation for space (light travels far)
    float distance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.00001 * distance + 0.0000001 * distance * distance);
    
    // Combine all lighting components
    vec3 lighting = ambient + (diffuse + specular) * attenuation;
    vec3 result = lighting * vertexColor;
    
    FragColor = vec4(result, 1.0);
}