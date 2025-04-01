#include "application.h"
#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

Application::Application()
    : m_window(nullptr)
    , m_windowWidth(800)
    , m_windowHeight(600)
    , m_VAO(0)
    , m_VBO(0)
    , m_EBO(0)
    , m_renderMode(2)
    , m_modelMatrix(1.0f)
    , m_viewMatrix(1.0f)
    , m_projectionMatrix(1.0f)
    , m_cameraPos(0.0f, 0.0f, 10.0f)
    , m_cameraFront(0.0f, 0.0f, -1.0f)
    , m_cameraUp(0.0f, 1.0f, 0.0f)
    , m_fov(45.0f)
    , m_nearClip(0.1f)
    , m_farClip(100.0f)
    , m_cameraSpeed(0.5f)
    , m_sensitivity(0.1f)
    , m_rightMousePressed(false)
    , m_leftMousePressed(false)
    , m_pitch(0.0f)
    , m_yaw(-90.0f)
    , m_modelCenter(0.0f)
{
    // Initialize the user input manager.
    m_userInput = new UserInput(this);
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

    // Setup screen quad
    createScreenQuad();

    // Load and compile your standard shader once
    m_standardShader = new Shader();
    if (!m_standardShader->load("shaders/standard.vert", "shaders/standard.frag")) {
        std::cerr << "Failed to load standard shader.\n";
    }

    // Load and compile the grid shader
    m_gridShader = new Shader();
    if (!m_gridShader->load("shaders/grid.vert", "shaders/grid.frag")) {
        std::cerr << "Failed to load grid shader.\n";
    }

    if (!m_grid.init(m_gridShader, 200, 5.0f)) {
        std::cerr << "Failed to initialize grid\n";
    }

    // Add model here
    addObject("cow.obj", glm::vec3(6.0f, 2.0f, 0.0f), "cow-tex-fin.jpg");
    addObject("cow.obj", glm::vec3(-6.0f, 2.0f, 0.0f), "cow-tex-fin.jpg");

    //Walls
    addObject("cube.obj", glm::vec3(0.0f, 2.0f, -5.0f), "", glm::vec3(14.0f, 6.0f, 0.5f), true, Colors::White);
    addObject("cube.obj", glm::vec3(0.0f, -4.0f, -0.0f), "", glm::vec3(14.0f, 0.5f, 6.0f), true, Colors::White);

    addObject("cube.obj", glm::vec3(-14, 2.0f, 0.0f), "", glm::vec3(0.5f, 6.0f, 6.0f), true, Colors::White);
    addObject("cube.obj", glm::vec3(14, 2.0f, 0.0f), "", glm::vec3(0.5f, 6.0f, 6.0f), true);


    glm::vec3 minBound = m_objLoader.getMinBounds();
    glm::vec3 maxBound = m_objLoader.getMaxBounds();
    m_modelCenter = (minBound + maxBound) * 0.5f;

    // Add lights here
    m_lights.addLight(glm::vec3(6.0f, 7.0f, 3.0f), Colors::Red, 1.2f);
    m_lights.addLight(glm::vec3(-6.0f, 7.0f, 3.0f), Colors::Blue, 1.2f);


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

    // Initializae light indicator
    m_lights.initIndicator();
    if (!m_lights.initIndicatorShader())
    {
        std::cerr << "Failed to initialize light indicator shader." << std::endl;
    }

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

    // Delete shader
    if (m_standardShader)
    {
        delete m_standardShader;
        m_standardShader = nullptr;
    }

    m_grid.cleanup();

    if (m_gridShader)
    {
        delete m_gridShader;
        m_gridShader = nullptr;
    }
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
    float speed = m_cameraSpeed * 0.2f; // Movement speed scaling

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
    // NORMAL RENDERING MODE (Rasterization)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set shading mode and apply lights
    float aspectRatio = (float)m_windowWidth / (float)m_windowHeight;
    m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspectRatio, m_nearClip, m_farClip);
    m_viewMatrix = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);

    switch (m_renderMode)
    {
    case 0: // Point Cloud Mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
       // glUniform1i(m_useTextureLocation, GL_FALSE);
        break;
    case 1: // Wireframe Mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        //glUniform1i(m_useTextureLocation, GL_FALSE);
        break;
    default: // Solid Mode (Default)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    }

    // Render infinite grid
    m_grid.render(m_viewMatrix, m_projectionMatrix);

    for (auto& obj : m_sceneObjects)
    {
        if (!obj.shader) continue;

        obj.shader->use();

        // Pass transformation matrices
        obj.shader->setMat4("model", obj.modelMatrix);
        obj.shader->setMat4("view", m_viewMatrix);
        obj.shader->setMat4("projection", m_projectionMatrix);

        // Pass shading mode and lighting
        obj.shader->setInt("shadingMode", m_shadingMode);
        obj.shader->setVec3("objectColor", obj.color);
        m_lights.applyLights(obj.shader->getID());

        // Texture binding and uniform
        if (obj.texture)
        {
            obj.texture->bind(0);
            obj.shader->setBool("useTexture", true);
            obj.shader->setInt("cowTexture", 0); // Ensure the sampler2D uniform is set to texture unit 0
        }
        else
        {
            obj.shader->setBool("useTexture", false);
        }

        // Draw the object
        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }


    renderAxisIndicator();
    renderGUI();

    // Render light indicators BEFORE swapping buffers
    m_lights.renderIndicators(m_viewMatrix, m_projectionMatrix);

    glfwSwapBuffers(m_window);
}

void Application::renderGUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always);

    ImGui::Begin("Render Settings");
    ImGui::Text("Use WASD to move, Q/E for up/down.");
    ImGui::Text("Left-click & drag to rotate model.");
    ImGui::Text("Right-click & drag to rotate camera.");

    ImGui::SliderFloat("Camera Speed", &m_cameraSpeed, 0.01f, 1.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_sensitivity, 0.01f, 1.0f);
    ImGui::SliderFloat("Field of View", &m_fov, 30.0f, 120.0f);
    ImGui::SliderFloat("Near Clip", &m_nearClip, 0.01f, 5.0f);
    ImGui::SliderFloat("Far Clip", &m_farClip, 5.0f, 500.0f);

    // Render Mode Buttons
    if (ImGui::Button("Point Cloud Mode")) { m_renderMode = 0; }
    ImGui::SameLine();
    if (ImGui::Button("Wireframe Mode")) { m_renderMode = 1; }
    ImGui::SameLine();
    if (ImGui::Button("Solid Mode")) { m_renderMode = 2; }

    // Shading Mode
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Shading Mode:");
    const char* shadingModes[] = { "Flat", "Gouraud", "Phong" };
    if (ImGui::Combo("##shadingMode", &m_shadingMode, shadingModes, IM_ARRAYSIZE(shadingModes))) {
        std::cout << "Shading Mode Changed: " << m_shadingMode << std::endl;
    }

    // Lighting
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Global ambient light
        ImGui::ColorEdit4("Global Ambient", glm::value_ptr(m_lights.m_globalAmbient));

        // Loop through lights
        for (size_t i = 0; i < m_lights.m_lights.size(); ++i)
        {
            auto& light = m_lights.m_lights[i];
            std::string label = "Light " + std::to_string(i);
            if (ImGui::TreeNode(label.c_str()))
            {
                ImGui::Checkbox(("Enabled##" + label).c_str(), &light.enabled);
                ImGui::ColorEdit3(("Color##" + label).c_str(), glm::value_ptr(light.color));
                ImGui::SliderFloat(("Intensity##" + label).c_str(), &light.intensity, 0.0f, 5.0f);
                ImGui::TreePop();
            }
        }
    }

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

void Application::addObject(const std::string& modelPath,
    const glm::vec3& initialPosition,
    const std::string& texturePath,
    const glm::vec3& scale,
    bool isStatic,
    const glm::vec3& color)
{
    SceneObject obj;
    obj.isStatic = isStatic;
    obj.color = color;
    obj.shader = m_standardShader;

    // Load the model
    obj.objLoader.load(modelPath.c_str());

    // Compute the object's bounding box and center
    glm::vec3 minBound = obj.objLoader.getMinBounds();
    glm::vec3 maxBound = obj.objLoader.getMaxBounds();
    glm::vec3 modelCenter = (minBound + maxBound) * 0.5f;

    // Build the model matrix
    obj.modelMatrix = glm::translate(glm::mat4(1.0f), initialPosition)
        * glm::scale(glm::mat4(1.0f), scale)
        * glm::translate(glm::mat4(1.0f), -modelCenter);

    // Generate and bind VAO, VBO, and EBO
    glGenVertexArrays(1, &obj.VAO);
    glGenBuffers(1, &obj.VBO);
    glGenBuffers(1, &obj.EBO);

    glBindVertexArray(obj.VAO);

    // Setup vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO);
    glBufferData(GL_ARRAY_BUFFER,
        obj.objLoader.getVertCount() * sizeof(glm::vec3),
        obj.objLoader.getPositions(),
        GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    // Setup element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        obj.objLoader.getIndexCount() * sizeof(unsigned int),
        obj.objLoader.getFaces(),
        GL_STATIC_DRAW);

    glBindVertexArray(0);

    // Load the texture if provided
    if (!texturePath.empty()) {
        obj.texture = new Texture();
        if (!obj.texture->loadFromFile(texturePath)) {
            std::cerr << "Failed to load texture from: " << texturePath << std::endl;
            delete obj.texture;
            obj.texture = nullptr;
        }
    }

    // Add to scene
    m_sceneObjects.push_back(obj);
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
    if (app->m_userInput) {
        app->m_userInput->mouseCallback(xpos, ypos);
    }
}

// Static mouse button callback
void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Application* app = getApplication(window);
    if (!app) return;
    if (app->m_userInput) {
        app->m_userInput->mouseButtonCallback(button, action, mods);
    }
}

void Application::setCameraAngles(float yaw, float pitch)
{
    m_yaw = yaw;
    m_pitch = pitch;
    // Recalculate the camera front vector from yaw and pitch
    glm::vec3 direction;
    direction.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    direction.y = sin(glm::radians(m_pitch));
    direction.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_cameraFront = glm::normalize(direction);
}

// Screen Quad for raytracing
void Application::createScreenQuad()
{
    float quadVertices[] = {
        // Positions    // Texture Coords
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };

    unsigned int quadIndices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    glBindVertexArray(quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool Application::rayIntersectsAABB(const Ray& ray, const glm::vec3& aabbMin, const glm::vec3& aabbMax, float& t)
{
    float tmin = (aabbMin.x - ray.origin.x) / ray.direction.x;
    float tmax = (aabbMax.x - ray.origin.x) / ray.direction.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (aabbMin.y - ray.origin.y) / ray.direction.y;
    float tymax = (aabbMax.y - ray.origin.y) / ray.direction.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;
    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (aabbMin.z - ray.origin.z) / ray.direction.z;
    float tzmax = (aabbMax.z - ray.origin.z) / ray.direction.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;
    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    t = tmin;
    if (t < 0) {
        t = tmax;
        if (t < 0)
            return false;
    }
    return true;
}