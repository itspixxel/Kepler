#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

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

// ==================== MAIN FUNCTION ==================== //
int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Kepler", NULL, NULL);
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

    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Cube vertex data with corrected winding order for outward normals
    float vertices[] = {
        // positions           // colors
        // Front face
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,   0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,   0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,   1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,   1.0f, 0.0f, 0.0f,

        // Back face
         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,   1.0f, 0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,   0.5f, 0.5f, 0.5f,
        -0.5f,  0.5f, -0.5f,   0.5f, 0.5f, 0.5f,
         0.5f,  0.5f, -0.5f,   0.2f, 0.8f, 0.3f,
         0.5f, -0.5f, -0.5f,   0.0f, 1.0f, 1.0f,

         // Left face
         -0.5f, -0.5f, -0.5f,   0.3f, 0.1f, 0.7f,
         -0.5f, -0.5f,  0.5f,   0.9f, 0.6f, 0.1f,
         -0.5f,  0.5f,  0.5f,   0.8f, 0.2f, 0.5f,
         -0.5f,  0.5f,  0.5f,   0.8f, 0.2f, 0.5f,
         -0.5f,  0.5f, -0.5f,   0.1f, 0.3f, 0.9f,
         -0.5f, -0.5f, -0.5f,   0.3f, 0.1f, 0.7f,

         // Right face
          0.5f, -0.5f,  0.5f,   0.4f, 0.9f, 0.4f,
          0.5f, -0.5f, -0.5f,   0.2f, 0.2f, 0.8f,
          0.5f,  0.5f, -0.5f,   0.7f, 0.1f, 0.1f,
          0.5f,  0.5f, -0.5f,   0.7f, 0.1f, 0.1f,
          0.5f,  0.5f,  0.5f,   0.6f, 0.6f, 0.0f,
          0.5f, -0.5f,  0.5f,   0.4f, 0.9f, 0.4f,

          // Bottom face
          -0.5f, -0.5f, -0.5f,   0.1f, 0.5f, 0.5f,
           0.5f, -0.5f, -0.5f,   0.5f, 0.1f, 0.3f,
           0.5f, -0.5f,  0.5f,   0.2f, 0.9f, 0.7f,
           0.5f, -0.5f,  0.5f,   0.2f, 0.9f, 0.7f,
          -0.5f, -0.5f,  0.5f,   0.9f, 0.3f, 0.3f,
          -0.5f, -0.5f, -0.5f,   0.1f, 0.5f, 0.5f,

          // Top face
          -0.5f,  0.5f, -0.5f,   0.5f, 0.8f, 0.1f,
           0.5f,  0.5f, -0.5f,   0.6f, 0.2f, 0.8f,
           0.5f,  0.5f,  0.5f,   0.9f, 0.9f, 0.2f,
           0.5f,  0.5f,  0.5f,   0.9f, 0.9f, 0.2f,
          -0.5f,  0.5f,  0.5f,   0.4f, 0.4f, 0.9f,
          -0.5f,  0.5f, -0.5f,   0.5f, 0.8f, 0.1f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    unsigned int shaderProgram = createShaderProgram("shaders/vertex_shader.glsl", "shaders/fragment_shader.glsl");

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    glfwTerminate();
    return 0;
}
