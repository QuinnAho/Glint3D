#include "application.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION  
#include "stb_image_write.h"

Application::Application()
    : m_window(nullptr)
    , m_windowWidth(800)
    , m_windowHeight(600)
    , m_VAO(0)
    , m_VBO(0)
    , m_EBO(0)
    , m_renderMode(2)
    , m_shadingMode(1)
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
    if (!m_cowTexture.loadFromFile("cow-tex-fin.jpg"))
        std::cerr << "Failed to load cow texture.\n";

    createScreenQuad();  // allocates rayTexID (RGB32F)

    // Clear the ray-texture once so the first frame isn’t pure black
    {
        std::vector<float> grey(m_windowWidth * m_windowHeight * 3, 0.1f);
        glBindTexture(GL_TEXTURE_2D, rayTexID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_windowWidth, m_windowHeight, GL_RGB, GL_FLOAT, grey.data());
    }

    // Load shaders
    m_standardShader = new Shader();
    if (!m_standardShader->load("shaders/standard.vert", "shaders/standard.frag"))
        std::cerr << "Failed to load standard shader.\n";

    m_gridShader = new Shader();
    if (!m_gridShader->load("shaders/grid.vert", "shaders/grid.frag"))
        std::cerr << "Failed to load grid shader.\n";

    if (!m_grid.init(m_gridShader, 200, 5.0f))
        std::cerr << "Failed to initialise grid.\n";

    m_rayScreenShader = new Shader();
    if (!m_rayScreenShader->load("shaders/rayscreen.vert", "shaders/rayscreen.frag"))
    {
        std::cerr << "[RayScreen] shader failed to load\n";
        std::terminate();
    }
    m_rayScreenShader->use();
    m_rayScreenShader->setInt("rayTex", 0);

    // ---- Add scene objects ----
    addObject("Cow Left", "cow.obj", glm::vec3(-6.0f, 2.0f, 5.0f), "cow-tex-fin.jpg", glm::vec3(1.0f), false, glm::vec3(1.0f));
    addObject("Cow Right", "cow.obj", glm::vec3(6.0f, 2.0f, 5.0f), "cow-tex-fin.jpg", glm::vec3(1.0f), false, glm::vec3(1.0f));

    addObject("Wall1", "cube.obj", glm::vec3(0.0f, 2.0f, -3.0f), "", glm::vec3(16.0f, 6.0f, 0.5f), true, glm::vec3(1.0f, 0.5f, 0.5f));
    addObject("Wall2", "cube.obj", glm::vec3(0.0f, -4.0f, 5.0f), "", glm::vec3(16.0f, 0.5f, 8.0f), true, glm::vec3(0.5f, 1.0f, 0.5f));
    addObject("Wall3", "cube.obj", glm::vec3(-16.0f, 2.0f, 5.0f), "", glm::vec3(0.5f, 6.0f, 8.0f), true, glm::vec3(0.5f, 0.5f, 1.0f));
    addObject("Wall4", "cube.obj", glm::vec3(16.0f, 2.0f, 5.0f), "", glm::vec3(0.5f, 6.0f, 8.0f), true, glm::vec3(1.0f, 1.0f, 0.5f));

    // ---- Add lights ----
    m_lights.addLight(glm::vec3(-6.0f, 7.0f, 8.0f), glm::vec3(1.0f, 0.5f, 0.5f), 1.2f);
    m_lights.addLight(glm::vec3(6.0f, 7.0f, 8.0f), glm::vec3(0.5f, 0.5f, 1.0f), 1.2f);

    // ---- Setup raytracer now that sceneObjects exist ----
    m_raytracer = std::make_unique<Raytracer>();
    for (auto& obj : m_sceneObjects)
    {
        bool isWall = obj.name.rfind("Wall", 0) == 0;
        float reflectivity = isWall ? 0.4f : 0.05f;

        // Setup materials for raytracer use
        if (isWall)
        {
            obj.material.specular = glm::vec3(0.2f);
            obj.material.ambient = obj.color * 0.4f;
            obj.material.shininess = 8.0f;
            obj.material.roughness = 0.8f;
            obj.material.metallic = 0.0f;
        }
        else
        {
            obj.material.specular = glm::vec3(0.3f);
            obj.material.ambient = obj.color * 0.5f;
            obj.material.shininess = 16.0f;
            obj.material.roughness = 0.6f;
            obj.material.metallic = 0.0f;
        }

        m_raytracer->loadModel(obj.objLoader, obj.modelMatrix, reflectivity, obj.material);
    }

    glEnable(GL_DEPTH_TEST);
    m_axisRenderer.init();

    m_lights.initIndicator();
    if (!m_lights.initIndicatorShader())
        std::cerr << "Failed to initialise light-indicator shader.\n";

    // ---- Setup shadow framebuffer ----
    glGenFramebuffers(1, &m_shadowFBO);
    glGenTextures(1, &m_shadowDepthTexture);
    glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowDepthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Load shadow shader
    m_shadowShader = new Shader();
    if (!m_shadowShader->load("shaders/shadow_depth.vert", "shaders/shadow_depth.frag"))
        std::cerr << "Failed to load shadow shader.\n";
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
    // ---- 1. Render shadow map first ----
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    glm::vec3 lightDir = glm::normalize(glm::vec3(-6.0f, 7.0f, 8.0f));
    glm::mat4 lightView = glm::lookAt(lightDir * 20.0f, glm::vec3(0.0f), glm::vec3(0, 1, 0));
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
    m_lightSpaceMatrix = lightProjection * lightView;

    m_shadowShader->use();
    m_shadowShader->setMat4("lightSpaceMatrix", m_lightSpaceMatrix);

    for (auto& obj : m_sceneObjects)
    {
        m_shadowShader->setMat4("model", obj.modelMatrix);
        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ---- 2. Render normal scene ----
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update camera matrices
    float aspect = float(m_windowWidth) / m_windowHeight;
    m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspect, m_nearClip, m_farClip);
    m_viewMatrix = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);

    if (m_renderMode == 3 && m_raytracer)
    {
        // --- Start the raytrace ONCE ---
        if (!m_traceJob.valid() && !m_traceDone)  // <=== ADD THIS !!
        {
            std::cout << "[TraceJob] Starting new raytrace...\n";

            m_framebuffer.resize(size_t(m_windowWidth) * m_windowHeight);

            m_traceJob = std::async(std::launch::async,
                [&, W = m_windowWidth, H = m_windowHeight]()
                {
                    std::cout << "[TraceJob] Worker: tracing image...\n";
                    m_raytracer->renderImage(m_framebuffer, W, H, m_cameraPos, m_cameraFront, m_cameraUp, m_fov, m_lights);

                    std::cout << "[TraceJob] Worker: finished tracing!\n";
                    m_traceDone = true;
                });
        }

        // --- If tracing finished, upload result ---
        if (m_traceJob.valid() && m_traceJob.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            m_traceJob.get(); // collect result (no exceptions)
            m_traceJob = std::future<void>(); // reset

            glBindTexture(GL_TEXTURE_2D, rayTexID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                m_windowWidth, m_windowHeight,
                GL_RGB, GL_FLOAT, m_framebuffer.data());
        }

        // --- Always draw the last available raytrace result ---
        m_rayScreenShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rayTexID);

        glBindVertexArray(quadVAO);
        glDisable(GL_DEPTH_TEST);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(0);

        renderAxisIndicator();
        m_lights.renderIndicators(m_viewMatrix, m_projectionMatrix);
        renderGUI();
        glfwSwapBuffers(m_window);
        return;
    }
    else
    {
        if (m_traceJob.valid())
            m_traceJob.wait();
        m_traceJob = std::future<void>();
        m_traceDone = false;
    }

    switch (m_renderMode)
    {
    case 0: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
    case 1: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  break;
    default:glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  break;
    }

    m_grid.render(m_viewMatrix, m_projectionMatrix);

    for (auto& obj : m_sceneObjects)
    {
        if (!obj.shader) continue;
        obj.shader->use();

        // Pass all the scene info
        obj.shader->setMat4("model", obj.modelMatrix);
        obj.shader->setMat4("view", m_viewMatrix);
        obj.shader->setMat4("projection", m_projectionMatrix);
        obj.shader->setMat4("lightSpaceMatrix", m_lightSpaceMatrix);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
        obj.shader->setInt("shadowMap", 1);

        obj.shader->setInt("shadingMode", m_shadingMode);
        obj.shader->setVec3("viewPos", m_cameraPos);
        obj.shader->setVec3("objectColor", obj.color);
        m_lights.applyLights(obj.shader->getID());
        obj.material.apply(obj.shader->getID(), "material");

        if (obj.texture)
        {
            obj.texture->bind(0);
            obj.shader->setBool("useTexture", true);
            obj.shader->setInt("cowTexture", 0);
        }
        else
            obj.shader->setBool("useTexture", false);

        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    renderAxisIndicator();
    renderGUI();
    m_lights.renderIndicators(m_viewMatrix, m_projectionMatrix);
    glfwSwapBuffers(m_window);
}



void Application::renderGUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Always);

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
    ImGui::SameLine();
    if (ImGui::Button("Raytrace"))   m_renderMode = 3;   // NEW

    if (m_renderMode == 3)
    {
        if (m_traceJob.valid())
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Raytracing... please wait");
        else if (m_traceDone)
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Raytracer Done!");
    }


    // Shading Mode
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Shading Mode:");
    const char* shadingModes[] = { "Flat", "Gouraud" };
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

    // Materials
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (size_t i = 0; i < m_sceneObjects.size(); ++i)
        {
            SceneObject& obj = m_sceneObjects[i];

            // If obj.name is empty, fall back to "Object i"
            std::string displayName = obj.name.empty()
                ? ("Object " + std::to_string(i))
                : obj.name;

            // Append an invisible unique ID (##Object_i) so ImGui tree nodes don't collide
            std::string treeNodeLabel = displayName + "##Object_" + std::to_string(i);

            if (ImGui::TreeNode(treeNodeLabel.c_str()))
            {
                glm::vec4 specColor(obj.material.specular, 1.0f);
                if (ImGui::ColorEdit4(("Specular##" + std::to_string(i)).c_str(), glm::value_ptr(specColor)))
                {
                    obj.material.specular = glm::vec3(specColor);
                }

                ImGui::ColorEdit3(("Diffuse##" + std::to_string(i)).c_str(), glm::value_ptr(obj.material.diffuse));
                ImGui::ColorEdit3(("Ambient##" + std::to_string(i)).c_str(), glm::value_ptr(obj.material.ambient));
                ImGui::SliderFloat(("Shininess##" + std::to_string(i)).c_str(),
                    &obj.material.shininess, 1.0f, 128.0f);
                ImGui::SliderFloat(("Roughness##" + std::to_string(i)).c_str(),
                    &obj.material.roughness, 0.0f, 1.0f);
                ImGui::SliderFloat(("Metallic##" + std::to_string(i)).c_str(),
                    &obj.material.metallic, 0.0f, 1.0f);

                ImGui::TreePop();
            }
        }
    }

    ImGui::End();

    // Right-side Talk panel (chat-style command input)
    renderChatPanel();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::renderChatPanel()
{
    const float panelWidth = 420.0f;
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - panelWidth - 10.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, ImGui::GetIO().DisplaySize.y - 20.0f), ImGuiCond_Always);
    ImGui::Begin("Talk", nullptr, ImGuiWindowFlags_NoCollapse);

    ImGui::Checkbox("Preview only", &m_previewOnly);
    ImGui::SameLine();
    ImGui::Checkbox("Use AI", &m_useAI);
    if (m_useAI)
    {
        char endpointBuf[256]; std::snprintf(endpointBuf, sizeof(endpointBuf), "%s", m_aiConfig.endpoint.c_str());
        char modelBuf[128];    std::snprintf(modelBuf,    sizeof(modelBuf),    "%s", m_aiConfig.model.c_str());
        if (ImGui::InputText("Endpoint", endpointBuf, IM_ARRAYSIZE(endpointBuf))) {
            m_aiConfig.endpoint = endpointBuf;
            m_ai.setConfig(m_aiConfig);
        }
        if (ImGui::InputText("Model", modelBuf, IM_ARRAYSIZE(modelBuf))) {
            m_aiConfig.model = modelBuf;
            m_ai.setConfig(m_aiConfig);
        }
    }
    ImGui::Separator();

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CtrlEnterForNewLine;
    ImGui::Text("Enter JSON-like command(s):");
    ImGui::InputTextMultiline("##chat_input", m_chatInput, IM_ARRAYSIZE(m_chatInput), ImVec2(-1.0f, 140.0f), flags);
    if (ImGui::Button("Submit"))
    {
        std::string input(m_chatInput);
        if (!input.empty()) {
            std::string jsonToRun;
            if (m_useAI) {
                std::string err;
                if (!m_ai.translate(input, jsonToRun, err)) {
                    m_chatScrollback.push_back(std::string("AI error: ") + err);
                    m_chatInput[0] = '\0';
                    // keep window open; do not early-return out of UI
                }
                m_chatScrollback.push_back(std::string("AI JSON:\n") + jsonToRun);
            } else {
                jsonToRun = input;
            }

            CommandBatch batch; std::string perr;
            if (ParseCommandBatch(jsonToRun, batch, perr)) {
                m_pendingPretty = ToJson(batch);
                m_chatScrollback.push_back(std::string("Parsed OK:\n") + m_pendingPretty);
                m_pendingBatch = batch;
                m_confirmOpen = true;
                ImGui::OpenPopup("Apply Commands?");
            } else {
                m_chatScrollback.push_back(std::string("Parse error: ") + perr);
            }
        }
        m_chatInput[0] = '\0';
    }

    ImGui::Separator();
    ImGui::Text("Output:");
    if (ImGui::BeginChild("##scrollback", ImVec2(0,0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        for (const auto& line : m_chatScrollback) {
            ImGui::TextUnformatted(line.c_str());
            ImGui::Separator();
        }
    }
    ImGui::EndChild();

    // Confirmation modal for current batch
    if (ImGui::BeginPopupModal("Apply Commands?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("About to apply the following commands:");
        ImGui::Separator();
        ImGui::BeginChild("##pending_pretty", ImVec2(400, 220), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(m_pendingPretty.c_str());
        ImGui::EndChild();
        ImGui::Separator();
        if (ImGui::Button("Apply")) {
            executeCommands(m_pendingBatch, false);
            m_confirmOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            m_confirmOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void Application::executeCommands(const CommandBatch& batch, bool previewOnly)
{
    for (const auto& cmd : batch.commands) {
        switch (cmd.op) {
        case CommandOp::LoadModel: {
            std::ostringstream os; os << "Will load_model: path='" << cmd.loadModel.path << "'";
            if (cmd.loadModel.name) os << ", name='" << *cmd.loadModel.name << "'";
            if (cmd.loadModel.transform.position) os << ", pos=(" << cmd.loadModel.transform.position->x << "," << cmd.loadModel.transform.position->y << "," << cmd.loadModel.transform.position->z << ")";
            if (cmd.loadModel.transform.scale)    os << ", scale=(" << cmd.loadModel.transform.scale->x    << "," << cmd.loadModel.transform.scale->y    << "," << cmd.loadModel.transform.scale->z    << ")";
            m_chatScrollback.push_back(os.str());
            if (!previewOnly) {
                glm::vec3 pos = cmd.loadModel.transform.position.value_or(glm::vec3(0.0f));
                glm::vec3 scl = cmd.loadModel.transform.scale.value_or(glm::vec3(1.0f));
                addObject(cmd.loadModel.name.value_or(""), cmd.loadModel.path, pos, "", scl, false, glm::vec3(1.0f));
                if (m_raytracer && !m_sceneObjects.empty()) {
                    const SceneObject& newObj = m_sceneObjects.back();
                    m_raytracer->loadModel(newObj.objLoader, newObj.modelMatrix, 0.05f, newObj.material);
                }
            }
            break; }
        case CommandOp::Duplicate: {
            std::ostringstream os; os << "Will duplicate: source='" << cmd.duplicate.source << "'";
            if (cmd.duplicate.name) os << ", name='" << *cmd.duplicate.name << "'";
            if (cmd.duplicate.transform.position) os << ", deltaPos=(" << cmd.duplicate.transform.position->x << "," << cmd.duplicate.transform.position->y << "," << cmd.duplicate.transform.position->z << ")";
            m_chatScrollback.push_back(os.str());
            if (!previewOnly) {
                auto it = std::find_if(m_sceneObjects.begin(), m_sceneObjects.end(), [&](const SceneObject& o){ return o.name == cmd.duplicate.source; });
                if (it != m_sceneObjects.end()) {
                    SceneObject copy = *it;
                    if (cmd.duplicate.name) copy.name = *cmd.duplicate.name; else copy.name += "_copy";
                    if (cmd.duplicate.transform.position) {
                        copy.modelMatrix = glm::translate(copy.modelMatrix, *cmd.duplicate.transform.position);
                    }
                    if (cmd.duplicate.transform.scale) {
                        copy.modelMatrix = copy.modelMatrix * glm::scale(glm::mat4(1.0f), *cmd.duplicate.transform.scale);
                    }
                    m_sceneObjects.push_back(copy);
                    if (m_raytracer && !m_sceneObjects.empty()) {
                        const SceneObject& newObj = m_sceneObjects.back();
                        m_raytracer->loadModel(newObj.objLoader, newObj.modelMatrix, 0.05f, newObj.material);
                    }
                } else {
                    m_chatScrollback.push_back("Duplicate error: source not found");
                }
            }
            break; }
        case CommandOp::AddLight: {
            std::ostringstream os; os << "Will add_light: type='" << cmd.addLight.type << "'";
            if (cmd.addLight.position)  os << ", pos=(" << cmd.addLight.position->x  << "," << cmd.addLight.position->y  << "," << cmd.addLight.position->z  << ")";
            if (cmd.addLight.direction) os << ", dir=(" << cmd.addLight.direction->x << "," << cmd.addLight.direction->y << "," << cmd.addLight.direction->z << ")";
            if (cmd.addLight.intensity) os << ", I=" << *cmd.addLight.intensity;
            m_chatScrollback.push_back(os.str());
            if (!previewOnly) {
                glm::vec3 c = cmd.addLight.color.value_or(glm::vec3(1.0f));
                float I = cmd.addLight.intensity.value_or(1.0f);
                glm::vec3 p = cmd.addLight.position.value_or(glm::vec3(0.0f));
                m_lights.addLight(p, c, I);
                m_lights.initIndicator();
            }
            break; }
        }
    }
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

void Application::addObject(std::string name,
    const std::string& modelPath,
    const glm::vec3& initialPosition,
    const std::string& texturePath,
    const glm::vec3& scale,
    bool isStatic,
    const glm::vec3& color
)
{
    SceneObject obj;
    obj.isStatic = isStatic;
    obj.color = color;
    obj.shader = m_standardShader;
    obj.name = name;

    obj.material.diffuse = color;
    obj.material.specular = glm::vec3(1.0f);   // white specular
    obj.material.ambient = color * 0.5f;        // slightly darker ambient
    obj.material.shininess = 32.0f;             // typical shiny surface
    obj.material.roughness = 0.2f;              // little bit glossy
    obj.material.metallic = 0.0f;               // not metallic


    // Walls: less reflective, less specular
    Material wallMaterial;
    wallMaterial.diffuse = color;
    wallMaterial.specular = glm::vec3(0.2f);  // LOWER specular strength
    wallMaterial.ambient = color * 0.4f;       // Keep ambient a bit dark
    wallMaterial.shininess = 8.0f;             // LOW shininess = large soft highlights
    wallMaterial.roughness = 0.8f;             // HIGH roughness = matte
    wallMaterial.metallic = 0.0f;              // Non-metallic

    float wallReflectivity = 0.05f;             // VERY LOW reflectivity


    // 1) Load the OBJ data (positions, faces, compute normals, etc.)
    obj.objLoader.load(modelPath.c_str());

    // 2) Compute bounding box center for pivot transforms
    glm::vec3 minBound = obj.objLoader.getMinBounds();
    glm::vec3 maxBound = obj.objLoader.getMaxBounds();
    glm::vec3 modelCenter = (minBound + maxBound) * 0.5f;

    // 3) Build the per-object model matrix (translate, scale, recenter)
    obj.modelMatrix = glm::translate(glm::mat4(1.0f), initialPosition)
        * glm::scale(glm::mat4(1.0f), scale)
        * glm::translate(glm::mat4(1.0f), -modelCenter);

    // 4) Generate VAO & the needed VBO/EBOs
    glGenVertexArrays(1, &obj.VAO);
    glBindVertexArray(obj.VAO);

    // 4a) Positions (VBO_positions)
    glGenBuffers(1, &obj.VBO_positions);
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_positions);
    glBufferData(
        GL_ARRAY_BUFFER,
        obj.objLoader.getVertCount() * sizeof(glm::vec3),
        obj.objLoader.getPositions(),
        GL_STATIC_DRAW
    );
    // aPos in location = 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // 4b) Normals (VBO_normals) - only if you have getNormals() in objLoader
    glGenBuffers(1, &obj.VBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_normals);
    glBufferData(
        GL_ARRAY_BUFFER,
        obj.objLoader.getVertCount() * sizeof(glm::vec3),
        obj.objLoader.getNormals(),
        GL_STATIC_DRAW
    );
    // aNormal in location = 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    // 4c) Element Buffer (EBO)
    glGenBuffers(1, &obj.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        obj.objLoader.getIndexCount() * sizeof(unsigned int),
        obj.objLoader.getFaces(),
        GL_STATIC_DRAW
    );

    // 5) Unbind the VAO (optional, but often good practice)
    glBindVertexArray(0);

    // 6) (Optional) Load a texture if provided
    if (!texturePath.empty()) {
        obj.texture = new Texture();
        if (!obj.texture->loadFromFile(texturePath)) {
            std::cerr << "Failed to load texture from: " << texturePath << std::endl;
            delete obj.texture;
            obj.texture = nullptr;
        }
    }

    // 7) Finally, add to our scene
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

    glGenTextures(1, &rayTexID);
    glBindTexture(GL_TEXTURE_2D, rayTexID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
        m_windowWidth, m_windowHeight,
        0, GL_RGB, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // ADD
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // ADD


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
