#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

// ==================== SHADER LOADING HELPERS ==================== //
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

    int success;
    char infoLog[512];
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << "\n";
    }

    return id;
}

std::string loadShaderSource(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << "\n";
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vShaderCode = loadShaderSource(vertexPath);
    std::string fShaderCode = loadShaderSource(fragmentPath);

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vShaderCode.c_str());
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fShaderCode.c_str());

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// ==================== WINDOW RESIZE CALLBACK ==================== //
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Function to generate sphere vertices and indices
void generateSphere(float radius, int stacks, int sectors,
    std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    const float PI = 3.14159265359f;

    // Generate unique vertices
    for (int i = 0; i <= stacks; ++i) {
        float theta = PI * static_cast<float>(i) / stacks;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int j = 0; j <= sectors; ++j) {
            float phi = 2.0f * PI * static_cast<float>(j) / sectors;
            float x = radius * sinTheta * std::cos(phi);
            float y = radius * sinTheta * std::sin(phi);
            float z = radius * cosTheta;

            // Color: Gradient based on position (normalize to 0-1)
            float r = (x + radius) / (2.0f * radius);
            float g = (y + radius) / (2.0f * radius);
            float b = (z + radius) / (2.0f * radius);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);
        }
    }

    // Generate indices for triangles
    int verticesPerRow = sectors + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int topLeft = i * verticesPerRow + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * verticesPerRow + j;
            int bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

// Function to generate starry skybox sphere
void generateStarField(float radius, int numStars, std::vector<float>& vertices) {
    const float PI = 3.14159265359f;

    // Seed for consistent star pattern
    srand(42);

    vertices.clear();

    for (int i = 0; i < numStars; ++i) {
        // Random spherical coordinates
        float theta = static_cast<float>(rand()) / RAND_MAX * PI;           // 0 to PI
        float phi = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;     // 0 to 2*PI

        // Convert to Cartesian coordinates
        float x = radius * sin(theta) * cos(phi);
        float y = radius * sin(theta) * sin(phi);
        float z = radius * cos(theta);

        // Random brightness for stars (varying white intensity)
        float brightness = 0.3f + (static_cast<float>(rand()) / RAND_MAX) * 0.7f; // 0.3 to 1.0

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(brightness);  // Red
        vertices.push_back(brightness);  // Green  
        vertices.push_back(brightness);  // Blue
    }
}

// Planet data structure
struct Planet {
    float size;           // Relative size
    float orbitRadius;    // Distance from sun (scaled)
    float orbitSpeed;     // Orbital speed
    float rotationSpeed;  // Self-rotation speed
    glm::vec3 color;     // Planet color tint
    const char* name;    // For debugging
};

// ==================== MAIN FUNCTION ==================== //
int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 900, "Solar System - Kepler", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glViewport(0, 0, 1200, 900);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Generate sphere data (shared for all planets/sun)
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(1.0f, 64, 64, vertices, indices);

    // Generate star field
    std::vector<float> starVertices;
    generateStarField(80.0f, 2000, starVertices);  // Large radius, many stars

    // Setup planet/sun VAO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Setup star field VAO
    unsigned int starVAO, starVBO;
    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);

    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, starVertices.size() * sizeof(float), starVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int shaderProgram = createShaderProgram("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f); // Very dark space background

    // Enable point sprite for stars
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Get uniform locations once (for efficiency)
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    // Define all planets with realistic relative data
    // Distances scaled down significantly for visibility (real ratios maintained)
    // Orbital speeds inverted (closer planets orbit faster)
    std::vector<Planet> planets = {
        // size, orbit, orbit_speed, rotation_speed, color, name
        {0.15f, 2.5f,  4.15f, 8.0f,  {0.8f, 0.7f, 0.6f}, "Mercury"},
        {0.25f, 3.2f,  1.62f, 2.0f,  {1.0f, 0.8f, 0.4f}, "Venus"},
        {0.28f, 4.0f,  1.00f, 4.0f,  {0.2f, 0.6f, 1.0f}, "Earth"},
        {0.18f, 4.8f,  0.53f, 3.8f,  {1.0f, 0.4f, 0.2f}, "Mars"},
        {0.85f, 7.0f,  0.084f, 6.0f, {1.0f, 0.8f, 0.6f}, "Jupiter"},
        {0.75f, 9.5f,  0.034f, 5.5f, {1.0f, 0.9f, 0.7f}, "Saturn"},
        {0.45f, 12.0f, 0.012f, 4.5f, {0.4f, 0.8f, 1.0f}, "Uranus"},
        {0.42f, 15.0f, 0.006f, 4.2f, {0.2f, 0.4f, 1.0f}, "Neptune"},
        {0.08f, 18.0f, 0.004f, 2.0f, {0.8f, 0.7f, 0.6f}, "Pluto"}
    };

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        float time = (float)glfwGetTime() * 0.3f; // Slow down overall animation

        // Camera positioned above and at an angle to view entire solar system
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -25.0f));  // Pull back and up
        view = glm::rotate(view, glm::radians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Tilt down slightly

        glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 900.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // ===== RENDER STARS FIRST (SKYBOX) =====
        glDepthMask(GL_FALSE); // Don't write to depth buffer for skybox
        glBindVertexArray(starVAO);

        // Stars don't need model transformation, just use identity
        glm::mat4 starModel = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(starModel));
        glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f); // White multiplier for stars

        glDrawArrays(GL_POINTS, 0, starVertices.size() / 6); // Each star is 6 floats (pos + color)

        glDepthMask(GL_TRUE); // Re-enable depth writing for planets

        // ===== RENDER PLANETS =====
        glBindVertexArray(VAO);

        // Draw Sun at center
        glm::mat4 sunModel = glm::mat4(1.0f);
        sunModel = glm::rotate(sunModel, time * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));  // Slow self-rotation
        sunModel = glm::scale(sunModel, glm::vec3(1.2f, 1.2f, 1.2f));  // Sun size
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(sunModel));
        glUniform3f(colorLoc, 1.0f, 1.0f, 0.3f);  // Bright yellow
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Draw all planets
        for (const auto& planet : planets) {
            glm::mat4 planetModel = glm::mat4(1.0f);

            // Scale the planet
            planetModel = glm::scale(planetModel, glm::vec3(planet.size));

            // Self rotation
            planetModel = glm::rotate(planetModel, time * planet.rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f));

            // Position at orbit distance
            planetModel = glm::translate(planetModel, glm::vec3(planet.orbitRadius / planet.size, 0.0f, 0.0f));

            // Orbital rotation around sun
            planetModel = glm::rotate(planetModel, time * planet.orbitSpeed, glm::vec3(0.0f, 1.0f, 0.0f));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(planetModel));
            glUniform3f(colorLoc, planet.color.x, planet.color.y, planet.color.z);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &starVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &starVBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}