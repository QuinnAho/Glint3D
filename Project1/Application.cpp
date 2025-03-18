#include "Application.h"
#include <iostream>

// ImGui includes (optional, if you want them in this file)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Application::Application()
    : m_window(nullptr)
    , m_windowWidth(800)
    , m_windowHeight(600)
    , m_shaderProgram(0)
    , m_VAO(0)
    , m_VBO(0)
    , m_EBO(0)
    , m_renderMode(2) // 2 = Solid
    , m_modelMatrix(1.0f)
    , m_viewMatrix(1.0f)
    , m_projectionMatrix(1.0f)
    , m_cameraPos(0.0f, 0.0f, 10.0f)
    , m_cameraFront(0.0f, 0.0f, -1.0f)
    , m_cameraUp(0.0f, 1.0f, 0.0f)
    , m_fov(45.0f)
    , m_nearClip(0.1f)
    , m_farClip(100.0f)
    , m_cameraSpeed(0.1f)
    , m_sensitivity(0.1f)
    , m_firstMouse(true)
    , m_rightMousePressed(false)
    , m_leftMousePressed(false)
    , m_lastX(400.0)
    , m_lastY(300.0)
    , m_pitch(0.0f)
    , m_yaw(-90.0f)
    , m_modelCenter(0.0f)
{
}

Application::~Application()
{
    cleanup();
}

bool Application::init(const std::string& windowTitle, int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;

    // Init GLFW & create window
    if (!initGLFW(windowTitle, width, height))
    {
        std::cerr << "Failed to initialize GLFW or create window.\n";
        return false;
    }

    // Init GLAD for OpenGL function loading
    if (!initGLAD())
    {
        std::cerr << "Failed to initialize GLAD.\n";
        return false;
    }

    // Init ImGui
    initImGui();

    // Setup our OpenGL resources (compile shaders, load model, etc.)
    setupOpenGL();

    return true;
}

bool Application::initGLFW(const std::string& windowTitle, int width, int height)
{
    if (!glfwInit())
        return false;

    // Create a windowed mode window and its OpenGL context
    m_window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_window);

    // Store a pointer to 'this' in the GLFW window user pointer,
    // so we can retrieve it in static callbacks
    glfwSetWindowUserPointer(m_window, this);

    // Register mouse callbacks
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);

    return true;
}

bool Application::initGLAD()
{
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
}

void Application::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Application::setupOpenGL()
{
    // Load the texture and compile shaders (handled inside Texture class)
    if (!m_cowTexture.loadFromFile("cow-tex-fin.jpg"))
    {
        std::cerr << "Failed to load cow texture.\n";
    }

    if (!m_cowTexture.initShaders())
    {
        std::cerr << "Failed to compile texture shaders.\n";
    }

    // Retrieve the shader program from Texture class
    GLuint shaderProgram = m_cowTexture.getShaderProgram();
    m_useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

    // Load the OBJ model
    m_objLoader.load("cow.obj");

    glm::vec3 minBound = m_objLoader.getMinBounds();
    glm::vec3 maxBound = m_objLoader.getMaxBounds();
    m_modelCenter = (minBound + maxBound) * 0.5f;

    m_light.setPosition(glm::vec3(2.0f, 5.0f, 3.0f)); // Move it above and to the side
    m_light.setColor(glm::vec3(1.0f, 1.0f, 1.0f));   // White light
    m_light.setIntensity(1.5f);  // Bright light

    // Translate model so that its center is at origin
    m_modelMatrix = glm::translate(glm::mat4(1.0f), -m_modelCenter);

    // Generate VAO/VBO/EBO
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    // Vertex buffer (assuming normal support is added)
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        m_objLoader.getVertCount() * sizeof(glm::vec3),
        m_objLoader.getPositions(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        m_objLoader.getIndexCount() * sizeof(unsigned int),
        m_objLoader.getFaces(),
        GL_STATIC_DRAW
    );

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    // Initialize axis renderer
    m_axisRenderer.init();
}

void Application::cleanup()
{
    // Cleanup axis
    m_axisRenderer.cleanup();

    // Cleanup VAO, VBO, EBO
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Destroy window
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void Application::run()
{
    // Main loop
    while (!glfwWindowShouldClose(m_window))
    {
        glfwPollEvents();
        processInput();
        renderScene();
    }
    // On exit, cleanup resources
    cleanup();
}

void Application::processInput()
{
    float speed = m_cameraSpeed * 0.05f; // Movement speed scaling

    // Cross product for right vector
    glm::vec3 rightVec = glm::normalize(glm::cross(m_cameraFront, m_cameraUp));

    // WASD, Q/E for camera movement
    if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += speed * m_cameraFront;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= speed * m_cameraFront;
    if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= speed * rightVec;
    if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += speed * rightVec;
    if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
        m_cameraPos -= speed * m_cameraUp;
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS)
        m_cameraPos += speed * m_cameraUp;
}

void Application::renderScene()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use texture shader
    GLuint shaderProgram = m_cowTexture.getShaderProgram();
    glUseProgram(shaderProgram);

    // Send Light properties to shader
    m_light.applyLight(shaderProgram);

    // Set up transformation matrices
    float aspectRatio = (float)m_windowWidth / (float)m_windowHeight;
    m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspectRatio, m_nearClip, m_farClip);
    m_viewMatrix = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);

    GLuint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLuint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));

    // Set rendering mode
    switch (m_renderMode)
    {
    case 0:
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glUniform1i(m_useTextureLocation, GL_FALSE);
        break;
    case 1:
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform1i(m_useTextureLocation, GL_FALSE);
        break;
    default:
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        m_cowTexture.bind(0);
        glUniform1i(glGetUniformLocation(shaderProgram, "cowTexture"), 0);
        glUniform1i(m_useTextureLocation, GL_TRUE);
        break;
    }

    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, m_objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    renderAxisIndicator();
    renderGUI();
    glfwSwapBuffers(m_window);
}

void Application::renderGUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(380, 300), ImGuiCond_Always);

    ImGui::Begin("Camera & Render Settings", nullptr, ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Use WASD to move, Q/E for up/down.");
    ImGui::Text("Left-click & drag to rotate model.");
    ImGui::Text("Right-click & drag to rotate camera.");

    ImGui::SliderFloat("Camera Speed", &m_cameraSpeed, 0.01f, 1.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_sensitivity, 0.01f, 1.0f);

    ImGui::SliderFloat("Field of View", &m_fov, 30.0f, 120.0f);
    ImGui::SliderFloat("Near Clipping Plane", &m_nearClip, 0.01f, 5.0f);
    ImGui::SliderFloat("Far Clipping Plane", &m_farClip, 5.0f, 500.0f);

    if (ImGui::Button("Set Points Mode")) { m_renderMode = 0; }
    if (ImGui::Button("Set Wireframe Mode")) { m_renderMode = 1; }
    if (ImGui::Button("Set Solid Mode")) { m_renderMode = 2; }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::renderAxisIndicator()
{
    // Orthographic in [-1,1] range
    glm::mat4 axisProjection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

    // Build a small transform that moves a tiny axis to top-right corner
    glm::mat4 rotation = glm::mat4_cast(glm::quatLookAt(m_cameraFront, m_cameraUp));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.15f));
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.75f, 0.75f, 0.0f));

    glm::mat4 axisModel = trans * rotation * scale;
    glm::mat4 identity = glm::mat4(1.0f);

    m_axisRenderer.render(axisModel, identity, axisProjection);
}

// Static callback - retrieve 'this' from the GLFW window user pointer
Application* Application::getApplication(GLFWwindow* window)
{
    return reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
}

// Static mouse position callback
void Application::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Application* app = getApplication(window);
    if (!app) return;

    // If it's the first time moving mouse, reset lastX/lastY
    if (app->m_firstMouse)
    {
        app->m_lastX = xpos;
        app->m_lastY = ypos;
        app->m_firstMouse = false;
    }

    float xOffset = static_cast<float>(xpos - app->m_lastX);
    float yOffset = static_cast<float>(app->m_lastY - ypos);
    app->m_lastX = xpos;
    app->m_lastY = ypos;

    xOffset *= app->m_sensitivity;
    yOffset *= app->m_sensitivity;

    // Right mouse drag: rotate camera
    if (app->m_rightMousePressed)
    {
        app->m_yaw += xOffset;
        app->m_pitch += yOffset;
        app->m_pitch = glm::clamp(app->m_pitch, -89.0f, 89.0f);

        glm::vec3 direction;
        direction.x = cos(glm::radians(app->m_yaw)) * cos(glm::radians(app->m_pitch));
        direction.y = sin(glm::radians(app->m_pitch));
        direction.z = sin(glm::radians(app->m_yaw)) * cos(glm::radians(app->m_pitch));
        app->m_cameraFront = glm::normalize(direction);
    }

    // Left mouse drag: rotate model
    if (app->m_leftMousePressed)
    {
        glm::mat4 toOrigin = glm::translate(glm::mat4(1.0f), -app->m_modelCenter);
        glm::mat4 back = glm::translate(glm::mat4(1.0f), app->m_modelCenter);

        // Inverted vertical rotation
        float angleX = -glm::radians(yOffset);
        float angleY = glm::radians(xOffset);

        app->m_modelMatrix = back
            * glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(1.0f, 0.0f, 0.0f))
            * glm::rotate(glm::mat4(1.0f), angleY, glm::vec3(0.0f, 1.0f, 0.0f))
            * toOrigin
            * app->m_modelMatrix;
    }
}

// Static mouse button callback
void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Application* app = getApplication(window);
    if (!app) return;

    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            app->m_rightMousePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            app->m_rightMousePressed = false;
            app->m_firstMouse = true;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            app->m_leftMousePressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            app->m_leftMousePressed = false;
            app->m_firstMouse = true;
        }
    }
}
