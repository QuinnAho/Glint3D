#include "application.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <filesystem>

#include "imgui_layer.h"
#include "app_state.h"
#include "app_commands.h"

#include "pbr_material.h"
#include "texture_cache.h"
#include "mesh_loader.h"
#include "material.h"
#include <unordered_set>
#ifndef __EMSCRIPTEN__
#ifdef OIDN_ENABLED
#include <OpenImageDenoise/oidn.hpp>
#endif
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION  
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
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

// Approximate a Blinn-Phong material from simple PBR factors for the CPU raytracer
static Material PhongFromPBR(const glm::vec4& baseColor, float metallic, float roughness)
{
    Material m;
    glm::vec3 albedo = glm::vec3(baseColor);
    m.diffuse = albedo;
    // Ambient as small fraction of albedo
    m.ambient = albedo * 0.03f;
    // Approximate specular color: F0 between 0.04 and albedo
    glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, glm::clamp(metallic, 0.0f, 1.0f));
    m.specular = F0;
    // Map roughness [0,1] -> shininess [4,256]
    float r = glm::clamp(roughness, 0.04f, 1.0f);
    float gloss = (1.0f - r);
    m.shininess = 4.0f + gloss * gloss * 252.0f; // bias + square curve
    m.roughness = r;
    m.metallic = glm::clamp(metallic, 0.0f, 1.0f);
    return m;
}

Application::~Application()
{
    cleanup();
}

bool Application::init(const std::string& windowTitle, int width, int height, bool headless)
{
    m_windowWidth = width;
    m_windowHeight = height;
    m_headless = headless;

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

    initImGui();

    // Setup our OpenGL resources (compile shaders, load model, etc.)
    setupOpenGL();

    // Initial console messages (welcome + controls)
#ifndef WEB_USE_HTML_UI
    if (!m_bootMessageShown) {
        m_chatScrollback.push_back("Welcome — ask me anything. Type in the Console below.");
        m_chatScrollback.push_back("Tip: Hold Right Mouse Button to move (W/A/S/D, Space/Ctrl, E/Q up/down).");
        m_chatScrollback.push_back("Gizmo: Shift+Q/W/E = Move/Rotate/Scale, X/Y/Z = axis.");
        m_chatScrollback.push_back("Edit gizmo via Right-Click menu. Lighting: select a light to edit.");
        m_bootMessageShown = true;
    }
#endif

    return true;
}

bool Application::initGLFW(const std::string& windowTitle, int width, int height)
{
    if (!glfwInit())
        return false;

    // Nicer default visuals
    glfwWindowHint(GLFW_SAMPLES, 4);            // MSAA 4x
#ifndef __EMSCRIPTEN__
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    if (m_headless) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    m_window = glfwCreateWindow(width, height, windowTitle.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync

    // Store a pointer to 'this' in the GLFW window user pointer,
    // so we can retrieve it in static callbacks
    glfwSetWindowUserPointer(m_window, this);

    // Register mouse callbacks
    glfwSetCursorPosCallback(m_window, mouseCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetFramebufferSizeCallback(m_window, Application::framebufferSizeCallback);

    return true;
}

bool Application::initGLAD()
{
#ifdef __EMSCRIPTEN__
    // WebGL2 doesn't use GLAD; context is ready when created
    return true;
#else
    return gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
#endif
}

void Application::initImGui() {
#ifndef WEB_USE_HTML_UI
    if (m_headless) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef IMGUI_HAS_DOCKING
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
    // Modern dark theme tweaks
    ImGuiStyle& style = ImGui::GetStyle();
#ifdef IMGUI_HAS_DOCKING
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 6.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif
    style.FrameRounding = 6.0f;
    style.GrabRounding  = 6.0f;
    style.TabRounding   = 6.0f;
    auto& c = style.Colors;
    c[ImGuiCol_WindowBg]        = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
    c[ImGuiCol_Header]          = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    c[ImGuiCol_HeaderHovered]   = ImVec4(0.28f, 0.32f, 0.36f, 1.00f);
    c[ImGuiCol_HeaderActive]    = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    c[ImGuiCol_TitleBg]         = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
    c[ImGuiCol_TitleBgActive]   = ImVec4(0.12f, 0.13f, 0.14f, 1.00f);
    c[ImGuiCol_Button]          = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
    c[ImGuiCol_ButtonHovered]   = ImVec4(0.25f, 0.28f, 0.31f, 1.00f);
    c[ImGuiCol_ButtonActive]    = ImVec4(0.22f, 0.25f, 0.28f, 1.00f);
    c[ImGuiCol_Tab]             = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
    c[ImGuiCol_TabHovered]      = ImVec4(0.28f, 0.32f, 0.36f, 1.00f);
    c[ImGuiCol_TabActive]       = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    c[ImGuiCol_FrameBg]         = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
    c[ImGuiCol_FrameBgHovered]  = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    c[ImGuiCol_FrameBgActive]   = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
#endif
}

void Application::setupOpenGL()
{
#ifdef __EMSCRIPTEN__
    if (!m_cowTexture.loadFromFile("assets/cow-tex-fin.jpg"))
        std::cerr << "Failed to load cow texture.\n";
#else
    if (!m_cowTexture.loadFromFile("cow-tex-fin.jpg"))
        std::cerr << "Failed to load cow texture.\n";
#endif

    createScreenQuad();  // allocates rayTexID (RGB32F)

    // Clear the ray-texture once so the first frame isnâ€™t pure black
    {
        std::vector<float> grey(m_windowWidth * m_windowHeight * 3, 0.1f);
        glBindTexture(GL_TEXTURE_2D, rayTexID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_windowWidth, m_windowHeight, GL_RGB, GL_FLOAT, grey.data());
        m_traceDone = true;
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

    // Load PBR shader
    m_pbrShader = new Shader();
    if (!m_pbrShader->load("shaders/pbr.vert", "shaders/pbr.frag"))
        std::cerr << "Failed to load PBR shader.\n";

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
#ifndef __EMSCRIPTEN__
    glEnable(GL_MULTISAMPLE);
    if (m_framebufferSRGBEnabled) glEnable(GL_FRAMEBUFFER_SRGB); else glDisable(GL_FRAMEBUFFER_SRGB);
#endif
    m_axisRenderer.init();
    m_gizmo.init();

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
#ifdef __EMSCRIPTEN__
    // WebGL2: no CLAMP_TO_BORDER; use EDGE and avoid border color
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
#endif

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowDepthTexture, 0);
#ifdef __EMSCRIPTEN__
    // FIX: WebGL2—do not call glDrawBuffers(0, nullptr); simply leave defaults.
#else  
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Load shadow shader
    m_shadowShader = new Shader();
    if (!m_shadowShader->load("shaders/shadow_depth.vert", "shaders/shadow_depth.frag"))
        std::cerr << "Failed to load shadow shader.\n";

    // Load outline shader for selected-object highlighting
    m_outlineShader = new Shader();
    if (!m_outlineShader->load("shaders/outline.vert", "shaders/outline.frag"))
        std::cerr << "Failed to load outline shader.\n";
}

void Application::cleanup()
{
    #ifndef WEB_USE_HTML_UI
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    #endif

    // Cleanup axis
    m_axisRenderer.cleanup();
    m_gizmo.cleanup();

    // Cleanup VAO, VBO, EBO
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);

    // UI cleanup handled by ImGui shutdown above

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

    if (m_outlineShader)
    {
        delete m_outlineShader;
        m_outlineShader = nullptr;
    }

    // Ray screen quad
    if (quadVAO) { glDeleteVertexArrays(1, &quadVAO); quadVAO = 0; }
    if (quadVBO) { glDeleteBuffers(1, &quadVBO); quadVBO = 0; }
    if (quadEBO) { glDeleteBuffers(1, &quadEBO); quadEBO = 0; }
    if (rayTexID) { glDeleteTextures(1, &rayTexID); rayTexID = 0; }

    // Shadow resources
    if (m_shadowFBO) { glDeleteFramebuffers(1, &m_shadowFBO); m_shadowFBO = 0; }
    if (m_shadowDepthTexture) { glDeleteTextures(1, &m_shadowDepthTexture); m_shadowDepthTexture = 0; }

    // Shaders
    auto del = [](Shader*& s){ if (s){ delete s; s=nullptr; } };
    del(m_standardShader);
    del(m_gridShader);
    del(m_outlineShader);
    del(m_pbrShader);
    del(m_shadowShader);
    del(m_rayScreenShader);

    // User input
    if (m_userInput) { delete m_userInput; m_userInput = nullptr; }
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

void Application::frame()
{
    if (!m_window) return;
    if (glfwWindowShouldClose(m_window)) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        cleanup();
        return;
    }
    glfwPollEvents();
    processInput();
    renderScene();
}

void Application::emscriptenFrame(void* arg)
{
    Application* app = static_cast<Application*>(arg);
    if (app) app->frame();
}

void Application::processInput()
{
    // Respect UI focus: avoid camera movement while UI is capturing input (unless RMB camera-lock is active)
#ifndef WEB_USE_HTML_UI
    if (!m_headless) {
        ImGuiIO& io = ImGui::GetIO();
        // If not holding RMB to control camera, and UI wants input, skip scene input
        if (!m_rightMousePressed && (io.WantCaptureMouse || io.WantCaptureKeyboard))
            return;
    }
#endif
    float speed = m_cameraSpeed * 0.2f; // Movement speed scaling

    // Cross product for right vector
    glm::vec3 rightVec = glm::normalize(glm::cross(m_cameraFront, m_cameraUp));

    // Camera movement only while RMB pressed (UE style)
    if (m_rightMousePressed) {
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS)
            m_cameraPos += speed * m_cameraFront;
        if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS)
            m_cameraPos -= speed * m_cameraFront;
        if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS)
            m_cameraPos -= speed * rightVec;
        if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS)
            m_cameraPos += speed * rightVec;
        // Vertical: Space up, LeftCtrl down, also E up / Q down while RMB
        if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
            m_cameraPos += speed * m_cameraUp;
        if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            m_cameraPos -= speed * m_cameraUp;
        if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS)
            m_cameraPos += speed * m_cameraUp;
        if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)
            m_cameraPos -= speed * m_cameraUp;
    }

    // F11 fullscreen toggle (edge-triggered)
    if (glfwGetKey(m_window, GLFW_KEY_F11) == GLFW_PRESS) {
        if (!m_f11Held) { toggleFullscreen(); }
        m_f11Held = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_F11) == GLFW_RELEASE) {
        m_f11Held = false;
    }

    // Gizmo mode shortcuts with Shift: Shift+Q=Translate, Shift+W=Rotate, Shift+E=Scale
    bool shiftDown = (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
                     (glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    if (shiftDown) {
        if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) m_gizmoMode = GizmoMode::Translate;
        if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) m_gizmoMode = GizmoMode::Rotate;
        if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) m_gizmoMode = GizmoMode::Scale;
    }

    // Axis highlight shortcuts: X/Y/Z
    if (glfwGetKey(m_window, GLFW_KEY_X) == GLFW_PRESS) m_gizmoAxis = GizmoAxis::X;
    if (glfwGetKey(m_window, GLFW_KEY_Y) == GLFW_PRESS) m_gizmoAxis = GizmoAxis::Y;
    if (glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_PRESS) m_gizmoAxis = GizmoAxis::Z;

    // Toggle local/world with L (edge-triggered)
    if (glfwGetKey(m_window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!m_lHeld) { m_gizmoLocalSpace = !m_gizmoLocalSpace; }
        m_lHeld = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_L) == GLFW_RELEASE) m_lHeld = false;

    // Toggle snapping with N (edge-triggered)
    if (glfwGetKey(m_window, GLFW_KEY_N) == GLFW_PRESS) {
        if (!m_nHeld) { m_snapEnabled = !m_snapEnabled; }
        m_nHeld = true;
    }
    if (glfwGetKey(m_window, GLFW_KEY_N) == GLFW_RELEASE) m_nHeld = false;
}

void Application::renderScene()
{
    // ---- 1. Render shadow map first ----
    glViewport(0, 0, 1024, 1024);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow FBO incomplete: 0x" << std::hex << status << std::dec << "\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
#ifndef __EMSCRIPTEN__
    if (m_framebufferSRGBEnabled) glEnable(GL_FRAMEBUFFER_SRGB); else glDisable(GL_FRAMEBUFFER_SRGB);
#endif
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update camera matrices
    float aspect = float(m_windowWidth) / m_windowHeight;
    m_projectionMatrix = glm::perspective(glm::radians(m_fov), aspect, m_nearClip, m_farClip);
    m_viewMatrix = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);

    if (m_renderMode == 3 && m_raytracer)
    {
        // Raytrace execution
#ifdef __EMSCRIPTEN__
    if (!m_traceDone)
    {
        // Run synchronously (single-threaded)
        std::cout << "[Trace] Tracing on main thread (Web) ...\n";
        m_framebuffer.resize(size_t(m_windowWidth) * m_windowHeight);
        m_raytracer->renderImage(m_framebuffer, m_windowWidth, m_windowHeight, m_cameraPos, m_cameraFront, m_cameraUp, m_fov, m_lights);
        glBindTexture(GL_TEXTURE_2D, rayTexID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            m_windowWidth, m_windowHeight,
            GL_RGB, GL_FLOAT, m_framebuffer.data());
        m_traceDone = true;
    }
#else
        // --- Start the raytrace ONCE ---
        if (!m_traceJob.valid() && !m_traceDone)
        {
            std::cout << "[TraceJob] Starting new raytrace...\n";

            m_framebuffer.resize(size_t(m_windowWidth) * m_windowHeight);

            m_traceJob = std::async(std::launch::async,
                [&, W = m_windowWidth, H = m_windowHeight]()
                {
                    std::cout << "[TraceJob] Worker: tracing image...\n";
                    m_raytracer->renderImage(m_framebuffer, W, H, m_cameraPos, m_cameraFront, m_cameraUp, m_fov, m_lights);

                    std::cout << "[TraceJob] Worker: finished tracing!\n";
                });
        }

        // --- If tracing finished, upload result ---
        if (m_traceJob.valid() && m_traceJob.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            m_traceJob.get(); // collect result (no exceptions)
            m_traceJob = std::future<void>(); // reset

#ifndef __EMSCRIPTEN__
#ifdef OIDN_ENABLED
            if (m_denoise) {
                denoise(m_framebuffer, nullptr, nullptr);
            }
#endif
#endif
            glBindTexture(GL_TEXTURE_2D, rayTexID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                m_windowWidth, m_windowHeight,
                GL_RGB, GL_FLOAT, m_framebuffer.data());
        }
#endif

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
        m_lights.renderIndicators(m_viewMatrix, m_projectionMatrix, m_selectedLightIndex);
        renderGUI();            // GUI is centralized here
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

#ifndef __EMSCRIPTEN__
    switch (m_renderMode)
    {
    case 0: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
    case 1: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  break;
    default:glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  break;
    }
#endif

    m_grid.render(m_viewMatrix, m_projectionMatrix);

    for (auto& obj : m_sceneObjects)
    {
        if (!obj.shader) obj.shader = m_standardShader;
        obj.shader->use();

        // Pass all the scene info
        obj.shader->setMat4("model", obj.modelMatrix);
        obj.shader->setMat4("view", m_viewMatrix);
        obj.shader->setMat4("projection", m_projectionMatrix);
        obj.shader->setMat4("lightSpaceMatrix", m_lightSpaceMatrix);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_shadowDepthTexture);
        obj.shader->setInt("shadowMap", 1);

        obj.shader->setVec3("viewPos", m_cameraPos);
        // Common shadow map binding
        m_lights.applyLights(obj.shader->getID());
        obj.material.apply(obj.shader->getID(), "material");

        // Setup per-shader specifics
        if (obj.shader == m_pbrShader)
        {
            // PBR uniforms
            obj.shader->setVec4("baseColorFactor", obj.baseColorFactor);
            obj.shader->setFloat("metallicFactor", obj.metallicFactor);
            obj.shader->setFloat("roughnessFactor", obj.roughnessFactor);
            // Textures
            int unit = 0;
            if (obj.baseColorTex) { obj.baseColorTex->bind(unit); obj.shader->setInt("baseColorTex", unit++); obj.shader->setBool("hasBaseColorMap", true);} else obj.shader->setBool("hasBaseColorMap", false);
            if (obj.normalTex)    { obj.normalTex->bind(unit);    obj.shader->setInt("normalTex", unit++);    obj.shader->setBool("hasNormalMap", true);} else obj.shader->setBool("hasNormalMap", false);
            if (obj.mrTex)        { obj.mrTex->bind(unit);        obj.shader->setInt("mrTex", unit++);        obj.shader->setBool("hasMRMap", true);} else obj.shader->setBool("hasMRMap", false);
        }
        else
        {
            obj.shader->setInt("shadingMode", m_shadingMode);
            obj.shader->setVec3("objectColor", obj.color);
            if (obj.texture)
            {
                obj.texture->bind(0);
                obj.shader->setBool("useTexture", true);
                obj.shader->setInt("cowTexture", 0);
            }
            else
                obj.shader->setBool("useTexture", false);
        }

        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Thin outline for selected object (if any)
    if (m_outlineShader && m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_sceneObjects.size())
    {
        const auto& obj = m_sceneObjects[m_selectedObjectIndex];
        glm::mat4 modelScaled = obj.modelMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(1.01f));
        m_outlineShader->use();
        m_outlineShader->setMat4("model", modelScaled);
        m_outlineShader->setMat4("view", m_viewMatrix);
        m_outlineShader->setMat4("projection", m_projectionMatrix);
        m_outlineShader->setVec3("outlineColor", glm::vec3(0.2f, 0.7f, 1.0f));
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glBindVertexArray(obj.VAO);
        glDrawElements(GL_TRIANGLES, obj.objLoader.getIndexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glCullFace(GL_BACK);
    }

    // Draw gizmo at selected object's center
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_sceneObjects.size())
    {
        glm::vec3 center = getSelectedObjectCenterWorld();
        float dist = glm::length(m_cameraPos - center);
        float gscale = glm::clamp(dist * 0.15f, 0.5f, 10.0f);
        // Orientation basis: local object's axes (normalized) or world
        glm::mat3 R(1.0f);
        if (m_gizmoLocalSpace) {
            const auto& obj = m_sceneObjects[m_selectedObjectIndex];
            glm::mat3 M3(obj.modelMatrix);
            R[0] = glm::normalize(glm::vec3(M3[0]));
            R[1] = glm::normalize(glm::vec3(M3[1]));
            R[2] = glm::normalize(glm::vec3(M3[2]));
        }
        m_gizmo.render(m_viewMatrix, m_projectionMatrix, center, R, gscale, m_gizmoAxis, m_gizmoMode);
    }

    renderAxisIndicator();
    renderGUI();
    m_lights.renderIndicators(m_viewMatrix, m_projectionMatrix, m_selectedLightIndex);
    glfwSwapBuffers(m_window);
}

void Application::renderGUI() {
#ifndef WEB_USE_HTML_UI
    if (m_headless) return;

    // New ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#ifdef IMGUI_HAS_DOCKING
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

    // ==== Main menu bar ====
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load Cube")) {
                loadObjInFrontOfCamera("Cube", "assets/models/cube.obj", 2.0f, glm::vec3(1.0f));
            }
            if (ImGui::MenuItem("Load Plane")) {
                loadObjInFrontOfCamera("Plane", "assets/samples/models/plane.obj", 2.0f, glm::vec3(1.0f));
            }
            if (ImGui::MenuItem("Copy Share Link")) {
                std::string link = buildShareLink();
                ImGui::SetClipboardText(link.c_str());
                m_chatScrollback.push_back("Share link copied to clipboard.");
            }
            bool toggle = m_showSettingsPanel;
            if (ImGui::MenuItem("Toggle Settings Panel", nullptr, toggle)) {
                m_showSettingsPanel = !m_showSettingsPanel;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            bool selected = (m_renderMode == 0); if (ImGui::MenuItem("Point",    nullptr, selected)) m_renderMode = 0;
            selected = (m_renderMode == 1);      if (ImGui::MenuItem("Wire",     nullptr, selected)) m_renderMode = 1;
            selected = (m_renderMode == 2);      if (ImGui::MenuItem("Solid",    nullptr, selected)) m_renderMode = 2;
            selected = (m_renderMode == 3);      if (ImGui::MenuItem("Raytrace", nullptr, selected)) m_renderMode = 3;
            ImGui::Separator();
            if (ImGui::MenuItem("Fullscreen")) toggleFullscreen();
            bool perfHud = m_showPerfHUD; if (ImGui::MenuItem("Perf HUD", nullptr, perfHud)) m_showPerfHUD = !m_showPerfHUD;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // ==== Settings & Diagnostics panel ====
    if (m_showSettingsPanel) {
        ImGuiIO& io = ImGui::GetIO();
        float consoleH = io.DisplaySize.y * 0.25f;
        float rightW   = 420.0f;
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - rightW - 10.0f, 10.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(rightW, io.DisplaySize.y - consoleH - 20.0f), ImGuiCond_Always);
        if (ImGui::Begin("Settings & Diagnostics")) {
            // Usage tips moved to Console on startup

            ImGui::SliderFloat("Camera Speed",       &m_cameraSpeed,   0.01f, 1.0f);
            ImGui::SliderFloat("Mouse Sensitivity",  &m_sensitivity,   0.01f, 1.0f);
            ImGui::SliderFloat("Field of View",      &m_fov,          30.0f, 120.0f);
            ImGui::SliderFloat("Near Clip",          &m_nearClip,      0.01f, 5.0f);
            ImGui::SliderFloat("Far Clip",           &m_farClip,       5.0f, 500.0f);

            // Render Mode Buttons
            if (ImGui::Button("Point Cloud Mode")) { m_renderMode = 0; }
            ImGui::SameLine();
            if (ImGui::Button("Wireframe Mode"))   { m_renderMode = 1; }
            ImGui::SameLine();
            if (ImGui::Button("Solid Mode"))       { m_renderMode = 2; }
            ImGui::SameLine();
            if (ImGui::Button("Raytrace"))         { m_renderMode = 3; } // NEW

            if (m_renderMode == 3) {
                if (m_traceJob.valid())
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Raytracing... please wait");
                else if (m_traceDone)
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Raytracer Done!");
            }

            // Diagnostics
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Diagnostics", ImGuiTreeNodeFlags_DefaultOpen)) {
                int  targetIdx  = m_selectedObjectIndex;
                if (targetIdx < 0 && !m_sceneObjects.empty()) targetIdx = 0;
                bool hasTarget  = (targetIdx >= 0 && targetIdx < (int)m_sceneObjects.size());

                bool anyPBR = false;
                for (auto& o : m_sceneObjects) { if (o.shader == m_pbrShader) { anyPBR = true; break; } }
                bool noLights = (m_lights.m_lights.empty());
                if (!noLights) {
                    bool allZero = true; for (auto& L : m_lights.m_lights) { if (L.enabled && L.intensity > 0.0f) { allZero = false; break; } }
                    noLights = allZero;
                }

                bool srgbMismatch = anyPBR && m_framebufferSRGBEnabled; // PBR does gamma out; sRGB FB doubles it

                ImGui::Text("Selected Object: %s", hasTarget ? m_sceneObjects[targetIdx].name.c_str() : "<none>");
                if (hasTarget) {
                    auto& obj = m_sceneObjects[targetIdx];
                    bool missingNormals = !obj.objLoader.hadNormalsFromSource();
                    bool backface       = objectMostlyBackfacing(obj);

                    if (missingNormals) {
                        ImGui::TextColored(ImVec4(1,1,0,1), "Missing normals: will use flat/poor shading."); ImGui::SameLine();
                        if (ImGui::SmallButton("Recompute (angle-weighted)")) {
                            recomputeAngleWeightedNormalsForObject(targetIdx);
                        }
                    } else {
                        ImGui::Text("Normals: OK (from source)");
                    }

                    if (backface) {
                        ImGui::TextColored(ImVec4(1,0.6f,0,1), "Bad winding: object faces away (backfaces)."); ImGui::SameLine();
                        if (ImGui::SmallButton("Flip Winding")) {
                            obj.objLoader.flipWindingAndNormals();
                            refreshIndexBuffer(obj);
                            refreshNormalBuffer(obj);
                        }
                    } else {
                        ImGui::Text("Winding: OK (mostly front-facing)");
                    }
                }

                if (noLights) {
                    ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "No effective lights -> scene is black."); ImGui::SameLine();
                    if (ImGui::SmallButton("Add Neutral Key Light")) {
                        addPointLightAt(getCameraPosition() + glm::normalize(getCameraFront()) * 2.0f, glm::vec3(1.0f), 1.0f);
                    }
                } else {
                    ImGui::Text("Lights: %d active", (int)m_lights.m_lights.size());
                }

                ImGui::Separator();
                ImGui::Text("Framebuffer sRGB: %s", m_framebufferSRGBEnabled ? "ON" : "OFF"); ImGui::SameLine();
                if (ImGui::SmallButton(m_framebufferSRGBEnabled ? "Disable" : "Enable")) { m_framebufferSRGBEnabled = !m_framebufferSRGBEnabled; }
                if (srgbMismatch) {
                    ImGui::TextColored(ImVec4(1,1,0,1), "sRGB mismatch: PBR already gamma-corrects; sRGB FB doubles it."); ImGui::SameLine();
                    if (ImGui::SmallButton("Fix (disable FB sRGB)")) { m_framebufferSRGBEnabled = false; }
                }
            }

            // Shading Mode
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Shading Mode:");
            const char* shadingModes[] = { "Flat", "Gouraud" };
            if (ImGui::Combo("##shadingMode", &m_shadingMode, shadingModes, IM_ARRAYSIZE(shadingModes))) {
                std::cout << "Shading Mode Changed: " << m_shadingMode << std::endl;
            }

            // Lighting (show selected only)
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::ColorEdit4("Global Ambient", glm::value_ptr(m_lights.m_globalAmbient));
                int sel = m_selectedLightIndex;
                if (sel >= 0 && sel < (int)m_lights.m_lights.size()) {
                    auto& light = m_lights.m_lights[(size_t)sel];
                    ImGui::Checkbox("Enabled", &light.enabled);
                    ImGui::ColorEdit3("Color", glm::value_ptr(light.color));
                    ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f);
                } else {
                    ImGui::TextDisabled("No light selected.");
                }
            }

            // Gizmo settings
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Gizmo", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool local = m_gizmoLocalSpace;
                if (ImGui::Checkbox("Local space", &local)) m_gizmoLocalSpace = local;
                ImGui::SameLine();
                bool snap = m_snapEnabled;
                if (ImGui::Checkbox("Snap", &snap)) m_snapEnabled = snap;
                ImGui::DragFloat("Translate Snap",     &m_snapTranslate, 0.05f, 0.01f, 10.0f, "%.2f");
                ImGui::DragFloat("Rotate Snap (deg)",  &m_snapRotateDeg, 1.0f,  1.0f,  90.0f, "%.0f");
                ImGui::DragFloat("Scale Snap",         &m_snapScale,     0.01f, 0.01f, 10.0f, "%.2f");
            }

            // Materials (selected object only)
            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Material (Selected)", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_sceneObjects.size()) {
                    SceneObject& obj = m_sceneObjects[(size_t)m_selectedObjectIndex];
                    std::string displayName = obj.name.empty() ? std::string("Selected Object") : obj.name;
                    ImGui::TextUnformatted(displayName.c_str());

                    // Shader selection per object
                    const char* shaderChoices[] = { "Standard", "PBR" };
                    int shaderIdx = (obj.shader == m_pbrShader) ? 1 : 0;
                    if (ImGui::Combo("Shader", &shaderIdx, shaderChoices, IM_ARRAYSIZE(shaderChoices))) {
                        obj.shader = (shaderIdx == 1 && m_pbrShader) ? m_pbrShader : m_standardShader;
                    }

                    glm::vec4 specColor(obj.material.specular, 1.0f);
                    if (ImGui::ColorEdit4("Specular", glm::value_ptr(specColor))) {
                        obj.material.specular = glm::vec3(specColor);
                    }

                    ImGui::ColorEdit3("Diffuse", glm::value_ptr(obj.material.diffuse));
                    ImGui::ColorEdit3("Ambient", glm::value_ptr(obj.material.ambient));
                    ImGui::SliderFloat("Shininess", &obj.material.shininess, 1.0f, 128.0f);
                    ImGui::SliderFloat("Roughness", &obj.material.roughness, 0.0f, 1.0f);
                    ImGui::SliderFloat("Metallic",  &obj.material.metallic,  0.0f, 1.0f);

                    if (obj.shader == m_pbrShader) {
                        ImGui::Separator();
                        ImGui::Text("PBR Factors");
                        ImGui::ColorEdit4("BaseColor",      glm::value_ptr(obj.baseColorFactor));
                        ImGui::SliderFloat("Metallic (PBR)",  &obj.metallicFactor,  0.0f, 1.0f);
                        ImGui::SliderFloat("Roughness (PBR)", &obj.roughnessFactor, 0.04f, 1.0f);
                        ImGui::Text("Maps: Base=%s Normal=%s MR=%s",
                                    obj.baseColorTex ? "yes" : "no",
                                    obj.normalTex    ? "yes" : "no",
                                    obj.mrTex        ? "yes" : "no");
                    }
                } else {
                    ImGui::TextDisabled("No object selected.");
                }
            }
        }
        ImGui::End();
    }

    // ==== Perf HUD overlay ====
    if (m_showPerfHUD) {
        PerfStats stats{}; computePerfStats(stats);
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("Perf HUD", &m_showPerfHUD, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Draw calls: %d", stats.drawCalls);
            ImGui::Text("Triangles: %zu", stats.totalTriangles);
            ImGui::Text("Materials: %d", stats.uniqueMaterialKeys);
            ImGui::Text("Textures: %zu (%.2f MB)", stats.uniqueTextures, stats.texturesMB);
            ImGui::Text("Geometry: %.2f MB", stats.geometryMB);
            ImGui::Separator();
            ImGui::Text("VRAM est: %.2f MB", stats.vramMB);
            if (stats.topSharedCount >= 2) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.8f,1.0f,0.2f,1.0f), "%d meshes share material: %s", stats.topSharedCount, stats.topSharedKey.c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton("Instancing candidate")) {
                    m_chatScrollback.push_back("Perf coach: Consider instancing/batching meshes sharing material: " + stats.topSharedKey);
                }
            } else if (stats.drawCalls > 50) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f,0.8f,0.2f,1.0f), "High draw calls (%d): merge static meshes or instance.", stats.drawCalls);
            } else if (stats.totalTriangles > 50000) {
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f,0.8f,0.2f,1.0f), "High triangle count (%.0fK): consider LOD or decimation.", stats.totalTriangles / 1000.0);
            }
            if (m_useAI) {
                if (ImGui::SmallButton("Ask AI for perf tips")) {
                    std::ostringstream summary;
                    summary << "Scene perf: drawCalls=" << stats.drawCalls
                            << ", tris=" << stats.totalTriangles
                            << ", materials=" << stats.uniqueMaterialKeys
                            << ", texMB=" << stats.texturesMB
                            << ", geoMB=" << stats.geometryMB
                            << ". TopShare='" << stats.topSharedKey << "' x" << stats.topSharedCount << ".";
                    std::string plan, err;
                    std::string scene = sceneToJson();
                    std::string prompt = std::string("Give one actionable performance suggestion for this scene (instancing, merging, texture atlases, LOD). Keep it to 1 sentence. ") + summary.str();
                    if (!m_ai.plan(prompt, scene, plan, err))
                        m_chatScrollback.push_back(std::string("AI perf tip error: ") + err);
                    else
                        m_chatScrollback.push_back(std::string("AI perf tip: ") + plan);
                }
            }
        }
        ImGui::End();
    }

    // ==== Scene context menu (right-click) ====
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        ImGui::OpenPopup("SceneContext");
    if (ImGui::BeginPopup("SceneContext")) {
        ImGui::TextUnformatted("Gizmo");
        ImGui::Separator();
        bool sel = (m_gizmoMode == GizmoMode::Translate); if (ImGui::MenuItem("Translate", nullptr, sel)) m_gizmoMode = GizmoMode::Translate;
        sel = (m_gizmoMode == GizmoMode::Rotate);         if (ImGui::MenuItem("Rotate",    nullptr, sel)) m_gizmoMode = GizmoMode::Rotate;
        sel = (m_gizmoMode == GizmoMode::Scale);          if (ImGui::MenuItem("Scale",     nullptr, sel)) m_gizmoMode = GizmoMode::Scale;
        ImGui::Separator();
        bool local = m_gizmoLocalSpace; if (ImGui::MenuItem("Local Space", nullptr, local)) m_gizmoLocalSpace = !m_gizmoLocalSpace;
        bool snap  = m_snapEnabled;     if (ImGui::MenuItem("Snap",        nullptr, snap))  m_snapEnabled = !m_snapEnabled;
        ImGui::Separator();
        if (ImGui::MenuItem("Axis X", nullptr, m_gizmoAxis==GizmoAxis::X)) m_gizmoAxis = GizmoAxis::X;
        if (ImGui::MenuItem("Axis Y", nullptr, m_gizmoAxis==GizmoAxis::Y)) m_gizmoAxis = GizmoAxis::Y;
        if (ImGui::MenuItem("Axis Z", nullptr, m_gizmoAxis==GizmoAxis::Z)) m_gizmoAxis = GizmoAxis::Z;
        ImGui::Separator();
        // Selection ops
        bool hasObj   = (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_sceneObjects.size());
        bool hasLight = (m_selectedLightIndex >= 0 && m_selectedLightIndex < (int)m_lights.m_lights.size());
        if (ImGui::MenuItem("Delete Selected", nullptr, false, hasObj || hasLight)) {
            if (hasObj) {
                std::string name = m_sceneObjects[(size_t)m_selectedObjectIndex].name;
                if (removeObjectByName(name)) m_chatScrollback.push_back(std::string("Deleted object: ") + name);
            } else if (hasLight) {
                if (removeLightAtIndex(m_selectedLightIndex)) m_chatScrollback.push_back("Deleted selected light.");
            }
        }
        if (ImGui::MenuItem("Duplicate Selected", nullptr, false, hasObj || hasLight)) {
            if (hasObj) {
                const auto& src = m_sceneObjects[(size_t)m_selectedObjectIndex];
                std::string newName = src.name + std::string("_copy");
                glm::vec3 dpos(0.2f, 0.0f, 0.0f);
                if (duplicateObject(src.name, newName, &dpos, nullptr, nullptr))
                    m_chatScrollback.push_back(std::string("Duplicated object as: ") + newName);
            } else if (hasLight) {
                auto L = m_lights.m_lights[(size_t)m_selectedLightIndex];
                addPointLightAt(L.position + glm::vec3(0.2f, 0, 0), L.color, L.intensity);
                m_chatScrollback.push_back("Duplicated selected light.");
            }
        }
        ImGui::EndPopup();
    }

    // ==== Bottom console window ====
    {
        ImGuiIO& io = ImGui::GetIO();
        float H = io.DisplaySize.y * 0.25f;
        ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - H), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, H), ImGuiCond_Always);
        if (ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
            if (ImGui::BeginChild("##console_scrollback", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar)) {
                for (const auto& line : m_chatScrollback) {
                    ImGui::TextUnformatted(line.c_str());
                }
            }
            ImGui::EndChild();

            static char quick[256] = {0};
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##console_input", quick, IM_ARRAYSIZE(quick), ImGuiInputTextFlags_EnterReturnsTrue)) {
                std::string cmd = quick; quick[0] = '\0';
                // Handle yes/y suggestion to open diagnostics
                bool handled = false;
                if (m_aiSuggestOpenDiag && !cmd.empty()) {
                    std::string s = cmd; for (auto& c : s) c = (char)std::tolower((unsigned char)c);
                    if (s == "yes" || s == "y") {
                        m_showSettingsPanel = true; m_aiSuggestOpenDiag = false; handled = true;
                        m_chatScrollback.push_back("Opening settings + diagnostics panel.");
                    }
                }
                if (!handled && !cmd.empty()) {
                    if (m_useAI) {
                        std::string plan, err;
                        std::string scene = sceneToJson();
                        if (!m_ai.plan(cmd, scene, plan, err)) {
                            m_chatScrollback.push_back(std::string("AI error: ") + err);
                        } else {
                            m_chatScrollback.push_back(std::string("AI plan:\n") + plan);
                            std::istringstream is(plan);
                            std::string line;
                            while (std::getline(is, line)) {
                                if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
                                std::vector<std::string> logs;
                                bool ok = m_nl.execute(line, logs);
                                for (auto& l : logs) m_chatScrollback.push_back(l);
                                if (!ok) m_chatScrollback.push_back("(no changes)");
                            }
                        }
                    } else {
                        std::vector<std::string> logs;
                        bool ok = m_nl.execute(cmd, logs);
                        for (auto& l : logs) m_chatScrollback.push_back(l);
                        if (!ok) m_chatScrollback.push_back("(no changes)");
                    }
                }
            }
        }
        ImGui::End(); // Console window
    }

    // Finalize ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#ifdef IMGUI_HAS_DOCKING
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup);
        }
    }
#endif
#endif // !WEB_USE_HTML_UI
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

glm::vec3 Application::getSelectedObjectCenterWorld() const
{
    if (m_selectedObjectIndex < 0 || m_selectedObjectIndex >= (int)m_sceneObjects.size()) return glm::vec3(0);
    const auto& obj = m_sceneObjects[m_selectedObjectIndex];
    glm::vec3 objMin = obj.objLoader.getMinBounds();
    glm::vec3 objMax = obj.objLoader.getMaxBounds();
    glm::vec3 objCenter = (objMin + objMax) * 0.5f;
    glm::vec3 worldCenter = glm::vec3(obj.modelMatrix * glm::vec4(objCenter, 1.0f));
    return worldCenter;
}

// ---- Diagnostics helpers ----
void Application::refreshNormalBuffer(SceneObject& obj)
{
    if (!obj.VBO_normals) return;
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * sizeof(glm::vec3), obj.objLoader.getNormals(), GL_STATIC_DRAW);
}

void Application::refreshIndexBuffer(SceneObject& obj)
{
    if (!obj.EBO) return;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.objLoader.getIndexCount() * sizeof(unsigned int), obj.objLoader.getFaces(), GL_STATIC_DRAW);
}

// ---- Perf coach helpers ----
std::string Application::materialKeyFor(const SceneObject& obj, const Shader* pbrShader)
{
    std::ostringstream ss;
    bool isPBR = (obj.shader == pbrShader);
    ss << (isPBR?"PBR":"Std");
    // Use texture pointer identities as keys (shared via TextureCache)
    ss << "|BC=" << (void*)obj.baseColorTex;
    ss << "|N="  << (void*)obj.normalTex;
    ss << "|MR=" << (void*)obj.mrTex;
    if (!isPBR)
    {
        // For standard, include useTexture flag and color
        ss << "|TEX=" << (obj.texture?"1":"0");
        if (!obj.texture)
            ss << "|col=" << std::hex << ((int)(obj.color.r*255)<<16 | (int)(obj.color.g*255)<<8 | (int)(obj.color.b*255));
    }
    return ss.str();
}

void Application::computePerfStats(PerfStats& out) const
{
    out.drawCalls = static_cast<int>(m_sceneObjects.size());
    // Triangles and geometry bytes
    size_t tris = 0; double geoBytes = 0.0;
    std::unordered_map<std::string, int> matCounts;
    std::unordered_set<const Texture*> texSet;
    for (const auto& obj : m_sceneObjects)
    {
        tris += static_cast<size_t>(obj.objLoader.getIndexCount() / 3);
        int v = obj.objLoader.getVertCount();
        // positions + normals always; UVs/tangents optional
        geoBytes += double(v) * (3+3) * sizeof(float);
        if (obj.objLoader.hasTexcoords()) geoBytes += double(v) * 2 * sizeof(float);
        if (obj.objLoader.hasTangents())  geoBytes += double(v) * 3 * sizeof(float);
        geoBytes += double(obj.objLoader.getIndexCount()) * sizeof(unsigned int);

        // material key
        std::string key = materialKeyFor(obj, m_pbrShader);
        matCounts[key] += 1;

        // textures used by this object
        if (obj.texture)      texSet.insert(obj.texture);
        if (obj.baseColorTex) texSet.insert(obj.baseColorTex);
        if (obj.normalTex)    texSet.insert(obj.normalTex);
        if (obj.mrTex)        texSet.insert(obj.mrTex);
    }
    out.totalTriangles = tris;
    out.geometryMB = geoBytes / (1024.0*1024.0);

    // Unique materials and top shared
    out.uniqueMaterialKeys = static_cast<int>(matCounts.size());
    for (auto& kv : matCounts) {
        if (kv.second > out.topSharedCount) { out.topSharedCount = kv.second; out.topSharedKey = kv.first; }
    }

    // Texture memory estimate from Texture objects' tracked dimensions
    double texBytes = 0.0;
    for (auto* t : texSet) {
        int w = t->width(), h = t->height(), c = t->channels();
        if (w>0 && h>0 && c>0) {
            int bpp = (c>=4)?4:3; // approximate
            double base = double(w) * double(h) * bpp;
            double withMips = base * 4.0 / 3.0; // ~1.33x for mip chain
            texBytes += withMips;
        }
    }
    out.uniqueTextures = texSet.size();
    out.texturesMB = texBytes / (1024.0*1024.0);
    out.vramMB = out.texturesMB + out.geometryMB;
}

void Application::recomputeAngleWeightedNormalsForObject(int index)
{
    if (index < 0 || index >= (int)m_sceneObjects.size()) return;
    auto& obj = m_sceneObjects[index];
    obj.objLoader.computeNormalsAngleWeighted();
    refreshNormalBuffer(obj);
}

bool Application::objectMostlyBackfacing(const SceneObject& obj) const
{
    int triCount = obj.objLoader.getIndexCount() / 3;
    if (triCount <= 0) return false;
    const unsigned int* idx = obj.objLoader.getFaces();
    const float* pos = obj.objLoader.getPositions();
    int samples = std::min(triCount, 200);
    int step = std::max(1, triCount / samples);
    int back = 0, total = 0;
    for (int t = 0; t < triCount && total < samples; t += step)
    {
        unsigned ia = idx[t*3+0], ib = idx[t*3+1], ic = idx[t*3+2];
        glm::vec3 v0(pos[ia*3+0], pos[ia*3+1], pos[ia*3+2]);
        glm::vec3 v1(pos[ib*3+0], pos[ib*3+1], pos[ib*3+2]);
        glm::vec3 v2(pos[ic*3+0], pos[ic*3+1], pos[ic*3+2]);
        glm::vec3 w0 = glm::vec3(obj.modelMatrix * glm::vec4(v0, 1.0f));
        glm::vec3 w1 = glm::vec3(obj.modelMatrix * glm::vec4(v1, 1.0f));
        glm::vec3 w2 = glm::vec3(obj.modelMatrix * glm::vec4(v2, 1.0f));
        glm::vec3 fn = glm::normalize(glm::cross(w1 - w0, w2 - w0));
        glm::vec3 center = (w0 + w1 + w2) * (1.0f/3.0f);
        glm::vec3 toCam = glm::normalize(m_cameraPos - center);
        if (glm::dot(fn, toCam) <= 0.0f) back++;
        total++;
    }
    if (total == 0) return false;
    float ratio = float(back) / float(total);
    return ratio > 0.95f;
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


    // 1) Load mesh + optional PBR via unified loader
    MeshData mesh; PBRMaterial pbr;
    std::string lerr;
    if (!LoadMeshFromFile(modelPath, mesh, &pbr, &lerr)) {
        // fallback to OBJ loader if unified loader fails unexpectedly
        if (!mesh.positions.size()) obj.objLoader.load(modelPath.c_str());
    } else {
        obj.objLoader.setFromRaw(mesh.positions, mesh.indices, mesh.normals, mesh.uvs, mesh.tangents);
        if (!pbr.baseColorTex.empty()) obj.baseColorTex = TextureCache::instance().get(pbr.baseColorTex, false);
        if (!pbr.normalTex.empty())    obj.normalTex    = TextureCache::instance().get(pbr.normalTex, false);
        if (!pbr.mrTex.empty())        obj.mrTex        = TextureCache::instance().get(pbr.mrTex, false);
        obj.baseColorFactor = pbr.baseColorFactor;
        obj.metallicFactor  = pbr.metallicFactor;
        obj.roughnessFactor = pbr.roughnessFactor;
        // Keep raytracer material in sync with PBR factors
        obj.material = PhongFromPBR(obj.baseColorFactor, obj.metallicFactor, obj.roughnessFactor);
        if (m_pbrShader && (!pbr.baseColorTex.empty() || !pbr.normalTex.empty() || !pbr.mrTex.empty()))
            obj.shader = m_pbrShader;
    }

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

    // 4b) Normals (VBO_normals)
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

    // 4c) UVs (VBO_uvs) if present
    if (obj.objLoader.hasTexcoords())
    {
        glGenBuffers(1, &obj.VBO_uvs);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_uvs);
        glBufferData(
            GL_ARRAY_BUFFER,
            obj.objLoader.getVertCount() * sizeof(glm::vec2),
            obj.objLoader.getTexcoords(),
            GL_STATIC_DRAW
        );
        // aUV in location = 2
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(2);
    }

    // 4d) Tangents (VBO_tangents) if present
    if (obj.objLoader.hasTangents())
    {
        glGenBuffers(1, &obj.VBO_tangents);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_tangents);
        glBufferData(
            GL_ARRAY_BUFFER,
            obj.objLoader.getVertCount() * sizeof(glm::vec3),
            obj.objLoader.getTangents(),
            GL_STATIC_DRAW
        );
        // aTangent in location = 3
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(3);
    }

    // 4e) Element Buffer (EBO)
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

    // 6) (Optional) Load a legacy diffuse texture if provided (via cache)
    if (!texturePath.empty()) {
        obj.texture = TextureCache::instance().get(texturePath, false);
        if (!obj.texture) {
            std::cerr << "Failed to load texture from: " << texturePath << std::endl;
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
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;
#ifndef WEB_USE_HTML_UI
    if (!app->m_headless) {
        ImGuiIO& io = ImGui::GetIO();
        // Allow mouse look when RMB is pressed even if UI wants the mouse
        if (io.WantCaptureMouse && !app->m_rightMousePressed) return;
    }
#endif
    if (app->m_userInput) {
        app->m_userInput->mouseCallback(xpos, ypos);
    }
}

// Static mouse button callback
void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;
#ifndef WEB_USE_HTML_UI
    if (!app->m_headless) {
        ImGuiIO& io = ImGui::GetIO();
        // Always forward right-button events for camera lock; otherwise respect UI capture
        if (io.WantCaptureMouse && button != GLFW_MOUSE_BUTTON_RIGHT) return;
    }
#endif
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
    #ifdef __EMSCRIPTEN__
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_windowWidth, m_windowHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    #else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_windowWidth, m_windowHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    #endif
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

// ---- Runtime scene API used by NL executor ----
bool Application::loadObjAt(const std::string& name,
                             const std::string& path,
                             const glm::vec3& position,
                             const glm::vec3& scale)
{
    namespace fs = std::filesystem;
    auto hasExt = [](const std::string& s){ return s.find('.') != std::string::npos; };
    auto toLower = [](std::string s){ std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return (char)std::tolower(c); }); return s; };
    auto stripTrailingDigits = [](std::string s){ while(!s.empty() && std::isdigit((unsigned char)s.back())) s.pop_back(); return s; };

    std::vector<std::string> baseCandidates;
    baseCandidates.push_back(path);
    if (!hasExt(path)) {
        static const char* kExts[] = { ".obj", ".gltf", ".glb", ".fbx", ".dae", ".ply" };
        for (const char* e : kExts) baseCandidates.push_back(path + e);
    }
    // Lowercase variants
    auto low = toLower(path);
    if (low != path) {
        baseCandidates.push_back(low);
        if (!hasExt(low)) {
            static const char* kExts[] = { ".obj", ".gltf", ".glb", ".fbx", ".dae", ".ply" };
            for (const char* e : kExts) baseCandidates.push_back(low + e);
        }
    }
    // Strip trailing digits (e.g., "Cow1" -> "Cow") then lowercase
    auto noDigits = stripTrailingDigits(path);
    if (noDigits != path) {
        auto ndLow = toLower(noDigits);
        baseCandidates.push_back(ndLow);
        if (!hasExt(ndLow)) {
            static const char* kExts[] = { ".obj", ".gltf", ".glb", ".fbx", ".dae", ".ply" };
            for (const char* e : kExts) baseCandidates.push_back(ndLow + e);
        }
    }

    std::vector<std::string> prefixes = { "", "assets/models/" };

    for (const auto& base : baseCandidates) {
        for (const auto& pref : prefixes) {
            std::string candidate = pref + base;
            std::error_code ec;
            if (fs::exists(candidate, ec)) {
                addObject(name, candidate, position, "", scale, false, glm::vec3(1.0f));
                if (m_raytracer && !m_sceneObjects.empty()) {
                    const SceneObject& newObj = m_sceneObjects.back();
                    m_raytracer->loadModel(newObj.objLoader, newObj.modelMatrix, 0.05f, newObj.material);
                }
                return true;
            }
        }
    }
    // Not found: log to chat scrollback for visibility
    std::ostringstream os; os << "Model not found for '" << path << "'. Tried ['', 'assets/models/'] with extensions [.obj,.gltf,.glb,.fbx,.dae,.ply] and lowercase/numberless variants.";
    m_chatScrollback.push_back(os.str());
    return false;
}

bool Application::loadObjInFrontOfCamera(const std::string& name,
                                          const std::string& path,
                                          float metersForward,
                                          const glm::vec3& scale)
{
    glm::vec3 pos = m_cameraPos + glm::normalize(m_cameraFront) * metersForward;
    return loadObjAt(name, path, pos, scale);
}

bool Application::addPointLightAt(const glm::vec3& position,
                                  const glm::vec3& color,
                                  float intensity)
{
    m_lights.addLight(position, color, intensity);
    m_lights.initIndicator();
    return true;
}

bool Application::createMaterialNamed(const std::string& name, const Material& m)
{
    m_namedMaterials[name] = m;
    return true;
}

bool Application::assignMaterialToObject(const std::string& objectName, const std::string& materialName)
{
    auto itM = m_namedMaterials.find(materialName);
    if (itM == m_namedMaterials.end()) return false;
    for (auto& o : m_sceneObjects) {
        if (o.name == objectName) { o.material = itM->second; return true; }
    }
    return false;
}

void Application::toggleFullscreen()
{
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (!m_fullscreen) {
        // save windowed pos/size
        glfwGetWindowPos(m_window, &m_windowPosX, &m_windowPosY);
        glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        m_fullscreen = true;
    } else {
        glfwSetWindowMonitor(m_window, nullptr, m_windowPosX, m_windowPosY, m_windowedWidth, m_windowedHeight, 0);
        m_fullscreen = false;
    }

    // Ensure our internal size matches framebuffer size immediately
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(m_window, &fbw, &fbh);
    if (fbw > 0 && fbh > 0) {
        m_windowWidth = fbw;
        m_windowHeight = fbh;
        glViewport(0, 0, m_windowWidth, m_windowHeight);
        // Reallocate ray texture to new size
        if (rayTexID != 0) {
            glBindTexture(GL_TEXTURE_2D, rayTexID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, m_windowWidth, m_windowHeight, 0, GL_RGB, GL_FLOAT, nullptr);
        }
    }
}

// Static callback - resize viewport and dependent textures
void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    if (!window) return;
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (!app) return;
    if (width <= 0 || height <= 0) return;

    app->m_windowWidth = width;
    app->m_windowHeight = height;

    glViewport(0, 0, width, height);
    // Reallocate ray texture to match new size
    if (app->rayTexID != 0) {
        glBindTexture(GL_TEXTURE_2D, app->rayTexID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    }
}

std::string Application::getSelectedObjectName() const
{
    if (m_selectedObjectIndex >= 0 && m_selectedObjectIndex < (int)m_sceneObjects.size())
        return m_sceneObjects[m_selectedObjectIndex].name;
    return std::string();
}

// ---- JSON Ops v1 apply (minimal tolerant parser) ----
namespace {
    static inline std::string trim_copy(std::string s){
        auto ns = [](int c){ return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), ns));
        s.erase(std::find_if(s.rbegin(), s.rend(), ns).base(), s.end());
        return s;
    }
    static bool findString(const std::string& src, const std::string& key, std::string& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false;
        pos = src.find(':', pos); if (pos==std::string::npos) return false;
        pos = src.find('"', pos); if (pos==std::string::npos) return false;
        auto end = pos + 1; std::string s;
        while (end < src.size()) { char c = src[end++]; if (c=='\\') { if (end<src.size()) s.push_back(src[end++]); } else if (c=='"') break; else s.push_back(c);} out = s; return true;
    }
    static bool findNumber(const std::string& src, const std::string& key, float& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false; pos = src.find(':', pos); if (pos==std::string::npos) return false;
        auto i = pos + 1; while (i<src.size() && (std::isspace((unsigned char)src[i])||src[i]=='"')) ++i;
        std::string num; while (i<src.size() && (std::isdigit((unsigned char)src[i])||src[i]=='-'||src[i]=='+'||src[i]=='.'||src[i]=='e'||src[i]=='E')) num.push_back(src[i++]);
        if (num.empty()) return false; try { out = std::stof(num); return true; } catch (...) { return false; }
    }
    static bool findBool(const std::string& src, const std::string& key, bool& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false; pos = src.find(':', pos); if (pos==std::string::npos) return false;
        auto i = pos + 1; while (i<src.size() && std::isspace((unsigned char)src[i])) ++i;
        if (i>=src.size()) return false;
        if (src.compare(i, 4, "true") == 0) { out = true; return true; }
        if (src.compare(i, 5, "false") == 0) { out = false; return true; }
        if (src[i]=='1') { out = true; return true; }
        if (src[i]=='0') { out = false; return true; }
        return false;
    }
    static bool findVec3(const std::string& src, const std::string& key, glm::vec3& out) {
        auto pos = src.find("\"" + key + "\""); if (pos==std::string::npos) return false; pos = src.find(':', pos); if (pos==std::string::npos) return false; pos = src.find('[', pos); if (pos==std::string::npos) return false;
        auto end = src.find(']', pos); if (end==std::string::npos) return false; std::string inside = src.substr(pos+1, end-pos-1);
        std::stringstream ss(inside); float x=0,y=0,z=0; char ch; if (!(ss>>x)) return false; ss>>ch; if (!(ss>>y)) return false; ss>>ch; if (!(ss>>z)) return false; out = glm::vec3(x,y,z); return true;
    }
}

bool Application::denoise(std::vector<glm::vec3>& color,
                          const std::vector<glm::vec3>* normal,
                          const std::vector<glm::vec3>* albedo)
{
#ifndef __EMSCRIPTEN__
#ifdef OIDN_ENABLED
    if (color.empty() || m_windowWidth <= 0 || m_windowHeight <= 0) return false;
    try {
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();

        oidn::FilterRef filter = device.newFilter("RT");
        filter.setImage("color", color.data(), oidn::Format::Float3, m_windowWidth, m_windowHeight);
        std::vector<glm::vec3> out(color.size());
        filter.setImage("output", out.data(), oidn::Format::Float3, m_windowWidth, m_windowHeight);
        if (normal && !normal->empty())
            filter.setImage("normal", normal->data(), oidn::Format::Float3, m_windowWidth, m_windowHeight);
        if (albedo && !albedo->empty())
            filter.setImage("albedo", albedo->data(), oidn::Format::Float3, m_windowWidth, m_windowHeight);
        // Our buffer is LDR-ish after gamma; leave hdr=false for robustness here.
        filter.set("hdr", false);
        filter.commit();
        filter.execute();

        const char* errMsg = nullptr;
        if (device.getError(errMsg) != oidn::Error::None) {
            std::cerr << "OIDN error: " << (errMsg ? errMsg : "unknown") << "\n";
            return false;
        }

        color.swap(out);
        return true;
    } catch (...) {
        return false;
    }
#else
    (void)color; (void)normal; (void)albedo; return false;
#endif
#else
    (void)color; (void)normal; (void)albedo; return false;
#endif
}

bool Application::moveObjectByName(const std::string& name, const glm::vec3& delta)
{
    for (auto& o : m_sceneObjects) {
        if (o.name == name) {
            o.modelMatrix = glm::translate(o.modelMatrix, delta);
            return true;
        }
    }
    return false;
}

bool Application::duplicateObject(const std::string& sourceName, const std::string& newName,
                                  const glm::vec3* deltaPos,
                                  const glm::vec3* deltaScale,
                                  const glm::vec3* deltaRotDeg)
{
    // Find source
    int idx = -1;
    for (int i = 0; i < (int)m_sceneObjects.size(); ++i) {
        if (m_sceneObjects[i].name == sourceName) { idx = i; break; }
    }
    if (idx < 0) return false;
    const SceneObject& src = m_sceneObjects[idx];

    // Create copy with fresh GL buffers from CPU geometry
    SceneObject obj = src; // copy material, shader choice, textures, factors, loader, etc.
    obj.name = newName;
    obj.VAO = obj.VBO_positions = obj.VBO_normals = obj.VBO_uvs = obj.VBO_tangents = obj.EBO = 0;

    // Upload GL buffers identical to addObject() step 4
    glGenVertexArrays(1, &obj.VAO);
    glBindVertexArray(obj.VAO);

    glGenBuffers(1, &obj.VBO_positions);
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_positions);
    glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * sizeof(glm::vec3), obj.objLoader.getPositions(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &obj.VBO_normals);
    glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_normals);
    glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * sizeof(glm::vec3), obj.objLoader.getNormals(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    if (obj.objLoader.hasTexcoords()) {
        glGenBuffers(1, &obj.VBO_uvs);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_uvs);
        glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * sizeof(glm::vec2), obj.objLoader.getTexcoords(), GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(2);
    }
    if (obj.objLoader.hasTangents()) {
        glGenBuffers(1, &obj.VBO_tangents);
        glBindBuffer(GL_ARRAY_BUFFER, obj.VBO_tangents);
        glBufferData(GL_ARRAY_BUFFER, obj.objLoader.getVertCount() * sizeof(glm::vec3), obj.objLoader.getTangents(), GL_STATIC_DRAW);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(3);
    }

    glGenBuffers(1, &obj.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj.objLoader.getIndexCount() * sizeof(unsigned int), obj.objLoader.getFaces(), GL_STATIC_DRAW);
    glBindVertexArray(0);

    // Set transform equal to source, then apply deltas
    obj.modelMatrix = src.modelMatrix;
    if (deltaPos)   obj.modelMatrix = glm::translate(obj.modelMatrix, *deltaPos);
    if (deltaScale) obj.modelMatrix = obj.modelMatrix * glm::scale(glm::mat4(1.0f), *deltaScale);
    if (deltaRotDeg) {
        glm::mat4 R = glm::mat4(1.0f);
        R = glm::rotate(R, glm::radians(deltaRotDeg->x), glm::vec3(1,0,0));
        R = glm::rotate(R, glm::radians(deltaRotDeg->y), glm::vec3(0,1,0));
        R = glm::rotate(R, glm::radians(deltaRotDeg->z), glm::vec3(0,0,1));
        obj.modelMatrix = obj.modelMatrix * R;
    }

    if (m_raytracer) m_raytracer->loadModel(m_sceneObjects.back().objLoader,
                                        m_sceneObjects.back().modelMatrix,
                                        0.05f, m_sceneObjects.back().material);

    m_sceneObjects.push_back(std::move(obj));
    return true;
}

void Application::setCameraTarget(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
{
    m_cameraPos = position;
    m_cameraUp = glm::normalize(up);
    m_cameraFront = glm::normalize(target - position);
}

void Application::setCameraFrontUp(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up)
{
    m_cameraPos = position;
    m_cameraUp = glm::normalize(up);
    m_cameraFront = glm::normalize(front);
}

void Application::setCameraLens(float fovDeg, float nearZ, float farZ)
{
    if (fovDeg > 0) m_fov = fovDeg;
    if (nearZ > 0)  m_nearClip = nearZ;
    if (farZ > 0)   m_farClip = farZ;
}

bool Application::removeObjectByName(const std::string& name)
{
    for (size_t i=0;i<m_sceneObjects.size();++i){
        if (m_sceneObjects[i].name == name) {
            auto& obj = m_sceneObjects[i];
            if (obj.VAO) glDeleteVertexArrays(1, &obj.VAO);
            if (obj.VBO_positions) glDeleteBuffers(1, &obj.VBO_positions);
            if (obj.VBO_normals) glDeleteBuffers(1, &obj.VBO_normals);
            if (obj.VBO_uvs) glDeleteBuffers(1, &obj.VBO_uvs);
            if (obj.VBO_tangents) glDeleteBuffers(1, &obj.VBO_tangents);
            if (obj.EBO) glDeleteBuffers(1, &obj.EBO);
            m_sceneObjects.erase(m_sceneObjects.begin() + i);
            if (m_selectedObjectIndex == (int)i) m_selectedObjectIndex = -1;
            else if (m_selectedObjectIndex > (int)i) m_selectedObjectIndex--;
            return true;
        }
    }
    return false;
}

bool Application::removeLightAtIndex(int i)
{
    if (i < 0) return false;
    bool ok = m_lights.removeLightAt((size_t)i);
    if (ok) {
        if (m_selectedLightIndex == i) m_selectedLightIndex = -1;
        else if (m_selectedLightIndex > i) m_selectedLightIndex--;
    }
    return ok;
}

int Application::getLightCount() const { return static_cast<int>(m_lights.m_lights.size()); }
glm::vec3 Application::getLightPosition(int i) const { return m_lights.m_lights[i].position; }