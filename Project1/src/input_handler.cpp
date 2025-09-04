#include "input_handler.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::vec3 InputHandler::cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
glm::vec3 InputHandler::cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 InputHandler::cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float InputHandler::cameraSpeed = 0.1f;
float InputHandler::sensitivity = 0.1f;
bool InputHandler::firstMouse = true;
double InputHandler::lastX = 400, InputHandler::lastY = 300;
float InputHandler::pitch = 0.0f, InputHandler::yaw = -90.0f;
bool InputHandler::rightMousePressed = false;

void InputHandler::initialize(GLFWwindow* window) {
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
}

void InputHandler::processInput(GLFWwindow* window) {
    float speed = cameraSpeed * 0.05f;
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
}

void InputHandler::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
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

void InputHandler::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) rightMousePressed = true;
        else if (action == GLFW_RELEASE) {
            rightMousePressed = false;
            firstMouse = true;
        }
    }
}
