#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

// ==================== SCALING CONSTANTS ==================== //
// These constants define how real-world measurements are scaled for visualization
const float DISTANCE_SCALE = 1.0f / 1000000.0f;  // 1 million km = 1 unit
const float SIZE_SCALE = 1.0f / 20000.0f;         // 1 million km = 1 unit  
const float TIME_SCALE = 86400.0f * 365.25f;      // 1 year = 1 time unit
const float SPEED_SCALE = 1000000.0f;             // Speed multiplier for visibility

// Real astronomical constants
const float AU = 149597870.7f;  // 1 AU in km
const float EARTH_RADIUS = 6371.0f;  // km
const float SUN_RADIUS = 696340.0f;  // km

// ==================== SHADER CLASS ==================== //
class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        std::string vertexCode = loadShaderSource(vertexPath);
        std::string fragmentCode = loadShaderSource(fragmentPath);

        unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
        unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);

        int success;
        char infoLog[512];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << "\n";
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    void use() const {
        glUseProgram(ID);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), value.x, value.y, value.z);
    }

    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

private:
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
};

// ==================== TEXTURE LOADING ==================== //
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Successfully loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        // Create a simple colored texture as fallback
        unsigned char fallbackData[12] = { 255, 255, 255, 255, 200, 200, 200, 200, 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, fallbackData);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Using fallback texture for: " << path << std::endl;
    }
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureID;
}

// ==================== CAMERA CONTROLS ==================== //
struct Camera {
    float radius = 500.0f;
    float theta = 0.0f;
    float phi = glm::radians(25.0f);
    float targetRadius = 500.0f;
    float targetTheta = 0.0f;
    float targetPhi = glm::radians(25.0f);
    float minRadius = 0.1f;
    float maxRadius = 5000.0f;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 targetTarget = glm::vec3(0.0f, 0.0f, 0.0f); // New: smoothly interpolate target
    float smoothingSpeed = 8.0f;

    void update(float deltaTime) {
        float lerpFactor = 1.0f - exp(-smoothingSpeed * deltaTime);
        radius = glm::mix(radius, targetRadius, lerpFactor);
        theta = glm::mix(theta, targetTheta, lerpFactor);
        phi = glm::mix(phi, targetPhi, lerpFactor);
        target = glm::mix(target, targetTarget, lerpFactor); // Smoothly interpolate target
    }

    glm::vec3 getPosition() const {
        float x = radius * cos(phi) * cos(theta);
        float y = radius * sin(phi);
        float z = radius * cos(phi) * sin(theta);
        return target + glm::vec3(x, y, z);
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
    }
};

// Global camera instance
Camera camera;

// Mouse state
bool mousePressed = false;
float lastX, lastY;
bool validLastPos = false;

// Global projection matrix
glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1200.0f / 900.0f, 0.01f, 10000.0f);

// ==================== INPUT CALLBACKS ==================== //
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    float aspectRatio = (float)width / (height > 0 ? height : 1);
    projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.01f, 10000.0f);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!mousePressed) {
        validLastPos = false;
        return;
    }

    if (validLastPos) {
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        float sensitivity = 0.005f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        camera.targetTheta += xoffset;
        camera.targetPhi += yoffset;

        if (camera.targetPhi > glm::radians(89.0f))
            camera.targetPhi = glm::radians(89.0f);
        if (camera.targetPhi < glm::radians(-89.0f))
            camera.targetPhi = glm::radians(-89.0f);
    }

    lastX = xpos;
    lastY = ypos;
    validLastPos = true;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            validLastPos = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        else if (action == GLFW_RELEASE) {
            mousePressed = false;
            validLastPos = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.targetRadius -= (float)yoffset * (camera.targetRadius * 0.1f);
    if (camera.targetRadius < camera.minRadius)
        camera.targetRadius = camera.minRadius;
    if (camera.targetRadius > camera.maxRadius)
        camera.targetRadius = camera.maxRadius;
}

// Function to generate sphere vertices and indices WITH NORMALS AND TEXTURE COORDINATES
void generateSphere(float radius, int stacks, int sectors,
    std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    constexpr float PI = glm::pi<float>();

    vertices.clear();
    indices.clear();

    for (int i = 0; i <= stacks; ++i) {
        float theta = PI * static_cast<float>(i) / stacks;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        float v = static_cast<float>(i) / stacks;

        for (int j = 0; j <= sectors; ++j) {
            float phi = 2.0f * PI * static_cast<float>(j) / sectors;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            float u = static_cast<float>(j) / sectors;

            float x = radius * sinTheta * cosPhi;
            float y = radius * cosTheta;
            float z = radius * sinTheta * sinPhi;

            float nx = sinTheta * cosPhi;
            float ny = cosTheta;
            float nz = sinTheta * sinPhi;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(nx);
            vertices.push_back(ny);
            vertices.push_back(nz);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    int verticesPerRow = sectors + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int topLeft = i * verticesPerRow + j;
            int topRight = topLeft + 1;
            int bottomLeft = (i + 1) * verticesPerRow + j;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
}

// Function to generate starry skybox sphere
void generateStarField(float innerRadius, float outerRadius, int numStars, std::vector<float>& vertices) {
    const float PI = 3.14159265359f;
    srand(42);
    vertices.clear();
    for (int i = 0; i < numStars; ++i) {
        float theta = static_cast<float>(rand()) / RAND_MAX * PI;
        float phi = static_cast<float>(rand()) / RAND_MAX * 2.0f * PI;
        float radius = innerRadius + (static_cast<float>(rand()) / RAND_MAX) * (outerRadius - innerRadius);
        float x = radius * sin(theta) * cos(phi);
        float y = radius * sin(theta) * sin(phi);
        float z = radius * cos(theta);
        float brightness = 0.3f + (static_cast<float>(rand()) / RAND_MAX) * 0.7f;
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(z);
        vertices.push_back(brightness);
        vertices.push_back(brightness);
        vertices.push_back(brightness);
    }
}

// ==================== MOON CLASS ==================== //
class Moon {
public:
    float size;
    float orbitRadius;
    float orbitalPeriod;  // in Earth days
    float rotationPeriod; // in Earth days
    float eccentricity;
    float axisTilt;       // in degrees
    std::string name;
    unsigned int textureID;

    // Add PBR texture IDs
    unsigned int albedoID = 0;
    unsigned int normalID = 0;
    unsigned int metallicID = 0;
    unsigned int roughnessID = 0;
    unsigned int aoID = 0;

    float ambientStrength;
    float specularStrength;
    int shininess;

    Moon(float radiusKm, float orbitRadiusKm, float orbitalPeriodDays,
        float rotationPeriodDays, float eccentricity, float axisTiltDeg,
        const std::string& name, const char* texturePath,
        float ambientStrength = 0.1f, float specularStrength = 0.2f, int shininess = 16,
        const char* albedoPath = nullptr, const char* normalPath = nullptr,
        const char* metallicPath = nullptr, const char* roughnessPath = nullptr,
        const char* aoPath = nullptr)
        : name(name), eccentricity(eccentricity), axisTilt(axisTiltDeg),
        ambientStrength(ambientStrength), specularStrength(specularStrength), shininess(shininess) {

        // Scale physical properties
        size = radiusKm * SIZE_SCALE;
        orbitRadius = orbitRadiusKm * DISTANCE_SCALE;
        orbitalPeriod = orbitalPeriodDays;
        rotationPeriod = rotationPeriodDays;

        // Load texture
        textureID = loadTexture(texturePath);
        if (textureID == 0) {
            std::cout << "Warning: Failed to load moon texture for " << name << std::endl;
        }
        // Load PBR textures if provided
        if (albedoPath) albedoID = loadTexture(albedoPath);
        if (normalPath) normalID = loadTexture(normalPath);
        if (metallicPath) metallicID = loadTexture(metallicPath);
        if (roughnessPath) roughnessID = loadTexture(roughnessPath);
        if (aoPath) aoID = loadTexture(aoPath);
    }

    glm::mat4 getTransform(float timeInDays, const glm::mat4& planetTransform) const {
        // Calculate mean anomaly based on orbital period
        float meanAnomaly = 2.0f * glm::pi<float>() * timeInDays / orbitalPeriod;

        // Solve Kepler's equation (simplified)
        float E = meanAnomaly;
        for (int i = 0; i < 5; ++i) {
            E = meanAnomaly + eccentricity * sin(E);
        }

        // Calculate position in orbit
        float a = orbitRadius;
        float x = a * (cos(E) - eccentricity);
        float z = a * sqrt(1.0f - eccentricity * eccentricity) * sin(E);

        // Rotation based on rotation period
        float rotationAngle = 2.0f * glm::pi<float>() * timeInDays / rotationPeriod;

        glm::mat4 moonModel = glm::mat4(1.0f);
        moonModel = glm::translate(moonModel, glm::vec3(x, 0.0f, z));
        moonModel = glm::rotate(moonModel, glm::radians(axisTilt), glm::vec3(1.0f, 0.0f, 0.0f));
        moonModel = glm::rotate(moonModel, rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        moonModel = glm::scale(moonModel, glm::vec3(size));

        return planetTransform * moonModel;
    }

    void render(const Shader& shader, float timeInDays, const glm::mat4& planetTransform,
        unsigned int indicesSize) const {
        glm::mat4 moonModel = getTransform(timeInDays, planetTransform);
        glm::mat4 moonNormalMatrix = glm::transpose(glm::inverse(moonModel));
        shader.setMat4("model", moonModel);
        shader.setMat4("normalMatrix", moonNormalMatrix);
        shader.setFloat("ambientStrength", ambientStrength);
        shader.setFloat("specularStrength", specularStrength);
        shader.setInt("shininess", shininess);

        // Bind PBR textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoID ? albedoID : textureID);
        shader.setInt("albedoMap", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalID);
        shader.setInt("normalMap", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, metallicID);
        shader.setInt("metallicMap", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, roughnessID);
        shader.setInt("roughnessMap", 3);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, aoID);
        shader.setInt("aoMap", 4);

        // Fallback legacy texture
        shader.setInt("texture1", 0);
        glDrawElements(GL_TRIANGLES, indicesSize, GL_UNSIGNED_INT, 0);

        // Unbind
        for (int i = 0; i < 5; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
};

// ==================== PLANET CLASS ==================== //
// Planet(size, orbitRadius, orbitalPeriod, rotationPeriod, eccentricity, axisTilt, rotationAxis, name, textureID, moons, ambientStrength, specularStrength, shininess)
class Planet {
public:
    float size;
    float orbitRadius;
    float orbitalPeriod;  // in Earth days
    float rotationPeriod; // in Earth days
    float eccentricity;
    float axisTilt;       // in degrees
    glm::vec3 rotationAxis;
    std::string name;
    unsigned int textureID;

    // Add PBR texture IDs
    unsigned int albedoID = 0;
    unsigned int normalID = 0;
    unsigned int metallicID = 0;
    unsigned int roughnessID = 0;
    unsigned int aoID = 0;

    std::vector<Moon> moons;
    float ambientStrength;
    float specularStrength;
    int shininess;

    Planet(float radiusKm, float orbitRadiusAU, float orbitalPeriodDays,
        float rotationPeriodHours, float eccentricity, float axisTiltDeg, const std::string& name,
        std::vector<Moon> moons = {},
        float ambientStrength = 0.1f, float specularStrength = 0.3f, int shininess = 32,
        const char* albedoPath = nullptr, const char* normalPath = nullptr,
        const char* metallicPath = nullptr, const char* roughnessPath = nullptr,
        const char* aoPath = nullptr)
        : name(name), eccentricity(eccentricity), axisTilt(axisTiltDeg), moons(moons),
        ambientStrength(ambientStrength), specularStrength(specularStrength), shininess(shininess) {

        // Scale physical properties
        size = radiusKm * SIZE_SCALE;
        orbitRadius = orbitRadiusAU * AU * DISTANCE_SCALE;
        orbitalPeriod = orbitalPeriodDays;
        rotationPeriod = rotationPeriodHours / 24.0f; // Convert hours to days

        // Calculate rotation axis from tilt
        float tiltRad = glm::radians(axisTilt);
        rotationAxis = glm::vec3(sin(tiltRad), cos(tiltRad), 0.0f);

        // Load PBR textures if provided
        if (albedoPath) albedoID = loadTexture(albedoPath);
        if (normalPath) normalID = loadTexture(normalPath);
        if (metallicPath) metallicID = loadTexture(metallicPath);
        if (roughnessPath) roughnessID = loadTexture(roughnessPath);
        if (aoPath) aoID = loadTexture(aoPath);
    }

    glm::mat4 getTransform(float timeInDays) const {
        // Calculate mean anomaly based on orbital period
        float meanAnomaly = 2.0f * glm::pi<float>() * timeInDays / orbitalPeriod;

        // Solve Kepler's equation (simplified)
        float E = meanAnomaly;
        for (int i = 0; i < 10; ++i) {
            E = meanAnomaly + eccentricity * sin(E);
        }

        // Calculate position in orbit
        float a = orbitRadius;
        float x = a * (cos(E) - eccentricity);
        float z = a * sqrt(1.0f - eccentricity * eccentricity) * sin(E);

        // Rotation based on rotation period
        float rotationAngle = 2.0f * glm::pi<float>() * timeInDays / rotationPeriod;

        glm::mat4 planetModel = glm::mat4(1.0f);
        planetModel = glm::translate(planetModel, glm::vec3(x, 0.0f, z));
        planetModel = glm::rotate(planetModel, rotationAngle, rotationAxis);
        planetModel = glm::scale(planetModel, glm::vec3(size));

        return planetModel;
    }

    void render(const Shader& shader, float timeInDays, unsigned int indicesSize) const {
        glm::mat4 planetModel = getTransform(timeInDays);
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(planetModel));

        shader.setMat4("model", planetModel);
        shader.setMat4("normalMatrix", normalMatrix);
        shader.setFloat("ambientStrength", ambientStrength);
        shader.setFloat("specularStrength", specularStrength);
        shader.setInt("shininess", shininess);

        // Bind PBR textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoID ? albedoID : textureID);
        shader.setInt("albedoMap", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalID);
        shader.setInt("normalMap", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, metallicID);
        shader.setInt("metallicMap", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, roughnessID);
        shader.setInt("roughnessMap", 3);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, aoID);
        shader.setInt("aoMap", 4);

        // Fallback legacy texture
        shader.setInt("texture1", 0);
        glDrawElements(GL_TRIANGLES, indicesSize, GL_UNSIGNED_INT, 0);

        // Unbind
        for (int i = 0; i < 5; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        // Render moons
        for (const auto& moon : moons) {
            moon.render(shader, timeInDays, planetModel, indicesSize);
        }
    }
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

    GLFWwindow* window = glfwCreateWindow(1200, 900, "Realistic Solar System - Kepler", NULL, NULL);
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
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Generate sphere data with texture coordinates
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    generateSphere(1.0f, 64, 64, vertices, indices);

    // Generate star field
    std::vector<float> starVertices;
    generateStarField(500.0f, 10000.0f, 3000, starVertices);

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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
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
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create shader program
    Shader shader("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
    glEnable(GL_PROGRAM_POINT_SIZE);

    // Load sun texture (handle missing textures gracefully)
    unsigned int sunTexture = 0;
    sunTexture = loadTexture("textures/sun.jpg");
    if (sunTexture == 0) {
        std::cout << "Warning: Could not load sun texture, using fallback" << std::endl;
    }

    // Create planets with realistic data (all measurements in real units)
    std::vector<Planet> planets = {

        // Mercury
        Planet(2440.0f, 0.39f, 88.0f, 1407.6f, 0.2056f, 0.034f, "Mercury", {},
               0.15f, 0.8f, 64, "textures/mercury.jpg", "textures/mercury_n.jpg"),

        // Venus  
        Planet(6051.8f, 0.723f, 224.7f, -5832.5f, 0.0067f, 177.36f, "Venus", {},
                0.2f, 0.1f, 8, "textures/venus.jpg", "textures/venus_n.jpg"),

        // Earth with Moon
        Planet(6371.0f, 1.0f, 365.26f, 23.93f, 0.0167f, 23.44f, "Earth",
                {
                    Moon(1737.4f, 384400.0f, 27.32f, 27.32f, 0.0549f, 0.0f,
                        "Moon", "textures/moon.jpg", 0.05f, 0.1f, 8)
                },
                0.1f, 0.6f, 32, "textures/earth.jpg", "textures/earth_n.jpg"),

        // Mars with moons
        Planet(3389.5f, 1.524f, 686.98f, 24.62f, 0.0935f, 25.19f, "Mars",
                {
                    Moon(11.1f, 9376.0f, 0.32f, 0.32f, 0.0151f, 0.0f,
                        "Phobos", "textures/phobos.png", 0.05f, 0.1f, 4),
                    Moon(6.2f, 23463.0f, 1.26f, 1.26f, 0.0002f, 0.0f,
                        "Deimos", "textures/deimos.png", 0.05f, 0.1f, 4)
                },
                0.12f, 0.2f, 16),

        // Jupiter with major moons
        Planet(69911.0f, 5.203f, 4332.59f, 9.93f, 0.0489f, 3.13f, "Jupiter",
                {
                    Moon(1821.6f, 421700.0f, 1.77f, 1.77f, 0.0041f, 0.0f,
                        "Io", "textures/io.png", 0.1f, 0.3f, 16),
                    Moon(1560.8f, 671034.0f, 3.55f, 3.55f, 0.0094f, 0.0f,
                        "Europa", "textures/europa.png", 0.08f, 0.9f, 128),
                    Moon(2634.1f, 1070412.0f, 7.15f, 7.15f, 0.0013f, 0.0f,
                        "Ganymede", "textures/ganymede.png", 0.1f, 0.4f, 32),
                    Moon(2410.3f, 1882709.0f, 16.69f, 16.69f, 0.0074f, 0.0f,
                        "Callisto", "textures/callisto.png", 0.08f, 0.2f, 8)
                },
                0.15f, 0.4f, 24),

        // Saturn with major moons
        Planet(58232.0f, 9.537f, 10759.22f, 10.66f, 0.0565f, 26.73f, "Saturn",
                {
                    Moon(2574.0f, 1221830.0f, 15.95f, 15.95f, 0.0288f, 0.0f,
                        "Titan", "textures/titan.png", 0.12f, 0.2f, 16),
                    Moon(763.8f, 527108.0f, 4.52f, 4.52f, 0.0010f, 0.0f,
                        "Rhea", "textures/rhea.png", 0.08f, 0.5f, 32),
                    Moon(734.5f, 3561300.0f, 79.33f, 79.33f, 0.0283f, 0.0f,
                        "Iapetus", "textures/iapetus.png", 0.1f, 0.3f, 16),
                    Moon(561.4f, 377396.0f, 2.74f, 2.74f, 0.0022f, 0.0f,
                        "Dione", "textures/dione.png", 0.08f, 0.6f, 64)
                },
                0.15f, 0.4f, 24),

        // Uranus with major moons
        Planet(25362.0f, 19.191f, 30688.5f, -17.24f, 0.0457f, 97.77f, "Uranus",
                {
                    Moon(788.4f, 435910.0f, 8.71f, 8.71f, 0.0011f, 0.0f,
                        "Titania", "textures/titania.png", 0.08f, 0.4f, 32),
                    Moon(761.4f, 583520.0f, 13.46f, 13.46f, 0.0014f, 0.0f,
                        "Oberon", "textures/oberon.png", 0.08f, 0.4f, 32)
                },
                0.12f, 0.5f, 32),

        // Neptune with Triton
        Planet(24622.0f, 30.069f, 60182.0f, 16.11f, 0.0113f, 28.32f, "Neptune",
                {
                    Moon(1353.4f, 354759.0f, -5.88f, -5.88f, 0.0000f, 0.0f,
                        "Triton", "textures/triton.png", 0.08f, 0.6f, 64)
                },
                0.12f, 0.5f, 32),

        // Pluto with Charon (dwarf planet)
        Planet(1188.3f, 39.482f, 90560.0f, -153.29f, 0.2488f, 122.53f, "Pluto",
                {
                    Moon(606.0f, 17536.0f, 6.39f, 6.39f, 0.0002f, 0.0f,
                        "Charon", "textures/charon.png", 0.08f, 0.3f, 16)
                },
                0.1f, 0.2f, 8)
    };

    // Timing for smooth interpolation
    float lastTime = 0.0f;
    float timeMultiplier = SPEED_SCALE; // Speed up time for visible orbits

    std::cout << "=== Realistic Solar System Simulation ===" << std::endl;
    std::cout << "Scaling factors:" << std::endl;
    std::cout << "- Distance: 1 unit = " << (1.0f / DISTANCE_SCALE) << " km" << std::endl;
    std::cout << "- Size: 1 unit = " << (1.0f / SIZE_SCALE) << " km" << std::endl;
    std::cout << "- Time: Accelerated by " << timeMultiplier << "x for visibility" << std::endl;
    std::cout << std::endl;

    std::cout << "Sun radius: " << SUN_RADIUS * SIZE_SCALE << " units (" << SUN_RADIUS << " km)" << std::endl;
    std::cout << "Earth-Sun distance: " << AU * DISTANCE_SCALE << " units (" << AU << " km)" << std::endl;
    std::cout << "Camera starting at distance: " << camera.radius << " units" << std::endl;
    std::cout << std::endl;

    std::cout << "Controls:" << std::endl;
    std::cout << "- Left click + drag: Rotate camera" << std::endl;
    std::cout << "- Scroll wheel: Zoom in/out" << std::endl;
    std::cout << "- ESC: Exit" << std::endl;
    std::cout << std::endl;

    int currentPlanetIndex = -1; // -1 means Sun, 0+ means planet index

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // --- Keyboard input for planet selection (keys 1-9) ---
        for (int i = 0; i < 9; ++i) {
            if (glfwGetKey(window, GLFW_KEY_1 + i) == GLFW_PRESS) {
                if (i < planets.size()) {
                    currentPlanetIndex = i;
                }
            }
        }
        // If a planet is selected, smoothly move camera target to planet position
        if (currentPlanetIndex >= 0 && currentPlanetIndex < planets.size()) {
            glm::mat4 planetTransform = planets[currentPlanetIndex].getTransform(currentTime * timeMultiplier / TIME_SCALE);
            glm::vec3 planetPos = glm::vec3(planetTransform[3]);
            camera.targetTarget = planetPos;
        } else {
            // Default: look at Sun (origin)
            camera.targetTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        camera.update(deltaTime);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Convert simulation time to days (accelerated)
        float timeInDays = currentTime * timeMultiplier / TIME_SCALE;

        glm::mat4 view = camera.getViewMatrix();

        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setVec3("lightPos", glm::vec3(0.0f, 0.0f, 0.0f));
        shader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.8f));
        glm::vec3 camPos = camera.getPosition();
        shader.setVec3("cameraPos", camPos);

        // Render stars
        glDepthMask(GL_FALSE);
        glBindVertexArray(starVAO);
        shader.setInt("isPoint", GL_TRUE);
        shader.setInt("isEmissive", GL_FALSE);
        shader.setFloat("time", currentTime);
        glm::mat4 starModel = glm::mat4(1.0f);
        shader.setMat4("model", starModel);
        shader.setInt("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDrawArrays(GL_POINTS, 0, starVertices.size() / 6);
        glDepthMask(GL_TRUE);

        // Render Sun with realistic size and rotation
        glBindVertexArray(VAO);
        shader.setInt("isPoint", GL_FALSE);
        shader.setInt("isEmissive", GL_TRUE);
        glm::mat4 sunModel = glm::mat4(1.0f);
        // Sun rotation period is about 25.05 days at equator
        float sunRotation = 2.0f * glm::pi<float>() * timeInDays / 25.05f;
        sunModel = glm::rotate(sunModel, sunRotation, glm::vec3(0.0f, 1.0f, 0.0f));
        float sunSize = SUN_RADIUS * SIZE_SCALE;
        sunModel = glm::scale(sunModel, glm::vec3(sunSize));
        shader.setMat4("model", sunModel);
        shader.setMat4("normalMatrix", glm::transpose(glm::inverse(sunModel)));
        shader.setInt("texture1", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sunTexture);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Debug output (only once)
        static bool debugPrinted = false;
        if (!debugPrinted) {
            std::cout << "Sun size in units: " << sunSize << std::endl;
            std::cout << "Camera distance: " << camera.radius << std::endl;
            std::cout << "First planet (Mercury) distance: " << planets[0].orbitRadius << std::endl;
            debugPrinted = true;
        }

        // Render planets and moons with realistic physics
        shader.setInt("isEmissive", GL_FALSE);
        for (const auto& planet : planets) {
            planet.render(shader, timeInDays, indices.size());
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

        // ESC to exit
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &starVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &starVBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shader.ID);
    glDeleteTextures(1, &sunTexture);
    for (const auto& planet : planets) {
        glDeleteTextures(1, &planet.textureID);
        for (const auto& moon : planet.moons) {
            glDeleteTextures(1, &moon.textureID);
        }
    }

    glfwTerminate();
    return 0;
}