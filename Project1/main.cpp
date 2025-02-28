#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "objloader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "AxisRenderer.h"

// Axis Render Instance
AxisRenderer axisRenderer;

// Rendering mode: 0 = Points, 1 = Wireframe, 2 = Solid
int renderMode = 2;

// Camera & Projection
glm::mat4 modelMatrix = glm::mat4(1.0f);
glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;
float fov = 45.0f, nearClip = 0.1f, farClip = 100.0f;
glm::vec3 cameraPos(0.0f, 0.0f, 10.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
float cameraSpeed = 0.1f;
float sensitivity = 0.1f;

// Mouse Input
bool firstMouse = true;
double lastX = 400, lastY = 300;
float pitch = 0.0f, yaw = -90.0f;
bool rightMousePressed = false;

// OpenGL Objects
GLuint VAO, VBO, EBO;
GLuint shaderProgram;
ObjLoader objLoader;

// OpenGL Axis
GLuint axisVAO, axisVBO;

glm::vec3 modelCenter;

void setupOpenGL() {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    })";

    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
    })";

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 🔹 Load OBJ Model
    objLoader.load("cow.obj");

    // 🔹 Compute Model Center
    glm::vec3 minBound = objLoader.getMinBounds();
    glm::vec3 maxBound = objLoader.getMaxBounds();
    modelCenter = (minBound + maxBound) * 0.5f;

    // 🔹 Apply initial translation to center model
    modelMatrix = glm::translate(glm::mat4(1.0f), -modelCenter);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, objLoader.getVertCount() * sizeof(glm::vec3), objLoader.getPositions(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, objLoader.getIndexCount() * sizeof(unsigned int), objLoader.getFaces(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    axisRenderer.init();
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {

    if (!rightMousePressed) return;
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xOffset = xpos - lastX;
    float yOffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch += yOffset;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) rightMousePressed = true;
        else if (action == GLFW_RELEASE) {
            rightMousePressed = false;
            firstMouse = true;
        }
    }
}

void renderAxisIndicator() {

    glm::mat4 axisProjectionMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    glm::mat4 axisViewMatrix = glm::lookAt(glm::vec3(0.0f), cameraFront, cameraUp);

    glm::mat4 axisModelMatrix = glm::mat4(1.0f);

    glm::mat4 rotationMatrix = glm::mat4_cast(glm::quatLookAt(cameraFront, cameraUp)); // Quaternion rotation

    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f));

    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.75f, 0.75f, 0.0f));

    axisModelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

    glm::mat4 identityMatrix = glm::mat4(1.0f);

    axisRenderer.render(axisModelMatrix, identityMatrix, axisProjectionMatrix);
}

void renderGUI() {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(380, 300), ImGuiCond_Always);

    ImGui::Begin("Camera & Render Settings", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Use WASD to move, Q/E for up/down.");
    ImGui::Text("Use IJKL and UO,for model rotation.");
    ImGui::Text("Right-click and drag to rotate camera.");

    ImGui::SliderFloat("Camera Speed", &cameraSpeed, 0.01f, 1.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &sensitivity, 0.01f, 1.0f);

    ImGui::SliderFloat("Field of View", &fov, 30.0f, 120.0f);
    ImGui::SliderFloat("Near Clipping Plane", &nearClip, 0.01f, 5.0f);
    ImGui::SliderFloat("Far Clipping Plane", &farClip, 5.0f, 500.0f);

    if (ImGui::Button("Set Points Mode")) renderMode = 0;
    if (ImGui::Button("Set Wireframe Mode")) renderMode = 1;
    if (ImGui::Button("Set Solid Mode")) renderMode = 2;

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void processInput(GLFWwindow* window) {
    float speed = cameraSpeed * 0.05f;
    float rotationSpeed = glm::radians(2.0f); 

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= speed * right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += speed * right;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos -= speed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos += speed * cameraUp;

    glm::mat4 translationToOrigin = glm::translate(glm::mat4(1.0f), -modelCenter); 
    glm::mat4 translationBack = glm::translate(glm::mat4(1.0f), modelCenter);      

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) // Pitch Up (X-axis)
        modelMatrix = translationBack * glm::rotate(glm::mat4(1.0f), -rotationSpeed, glm::vec3(1.0f, 0.0f, 0.0f)) * translationToOrigin * modelMatrix;
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) // Pitch Down (X-axis)
        modelMatrix = translationBack * glm::rotate(glm::mat4(1.0f), rotationSpeed, glm::vec3(1.0f, 0.0f, 0.0f)) * translationToOrigin * modelMatrix;

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) // Yaw Left (Y-axis)
        modelMatrix = translationBack * glm::rotate(glm::mat4(1.0f), -rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f)) * translationToOrigin * modelMatrix;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) // Yaw Right (Y-axis)
        modelMatrix = translationBack * glm::rotate(glm::mat4(1.0f), rotationSpeed, glm::vec3(0.0f, 1.0f, 0.0f)) * translationToOrigin * modelMatrix;

    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) // Roll Left (Z-axis)
        modelMatrix = translationBack * glm::rotate(glm::mat4(1.0f), -rotationSpeed, glm::vec3(0.0f, 0.0f, 1.0f)) * translationToOrigin * modelMatrix;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) // Roll Right (Z-axis)
        modelMatrix = translationBack * glm::rotate(glm::mat4(1.0f), rotationSpeed, glm::vec3(0.0f, 0.0f, 1.0f)) * translationToOrigin * modelMatrix;
}

void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram);

    float aspectRatio = 800.0f / 600.0f;
    projectionMatrix = glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
    viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    glPolygonMode(GL_FRONT_AND_BACK, (renderMode == 0) ? GL_POINT : (renderMode == 1) ? GL_LINE : GL_FILL);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    renderAxisIndicator();

    glUseProgram(shaderProgram);

    renderGUI();

    glfwSwapBuffers(glfwGetCurrentContext());
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "OBJ Viewer", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    setupOpenGL();

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        renderScene();
        glfwPollEvents();
    }

    axisRenderer.cleanup();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
