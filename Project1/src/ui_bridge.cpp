#include "ui_bridge.h"
#include "scene_manager.h"
#include "render_system.h"
#include "camera_controller.h"
#include "light.h"
#include <iostream>

UIBridge::UIBridge(SceneManager& scene, RenderSystem& renderer, 
                   CameraController& camera, Light& lights)
    : m_scene(scene)
    , m_renderer(renderer)
    , m_camera(camera)
    , m_lights(lights)
{
}

bool UIBridge::initUI(int windowWidth, int windowHeight)
{
    if (!m_ui) {
        return true; // No UI layer to initialize
    }
    
    // Set up command callback
    m_ui->onCommand = [this](const UICommandData& cmd) {
        handleUICommand(cmd);
    };
    
    return m_ui->init(windowWidth, windowHeight);
}

void UIBridge::shutdownUI()
{
    if (m_ui) {
        m_ui->shutdown();
    }
}

void UIBridge::renderUI()
{
    if (!m_ui) {
        return;
    }
    
    UIState state = buildUIState();
    m_ui->render(state);
}

void UIBridge::handleResize(int width, int height)
{
    if (m_ui) {
        m_ui->handleResize(width, height);
    }
}

UIState UIBridge::buildUIState() const
{
    UIState state;
    
    // Camera state
    state.camera = m_camera.getCameraState();
    state.cameraSpeed = m_camera.getSpeed();
    state.sensitivity = m_camera.getSensitivity();
    
    // Render state
    state.renderMode = m_renderer.getRenderMode();
    state.shadingMode = m_renderer.getShadingMode();
    state.framebufferSRGBEnabled = m_renderer.isFramebufferSRGBEnabled();
    state.denoiseEnabled = m_renderer.isDenoiseEnabled();
    state.showGrid = m_renderer.isShowGrid();
    state.showAxes = m_renderer.isShowAxes();
    
    // Scene state
    state.selectedObjectIndex = m_scene.getSelectedObjectIndex();
    state.selectedObjectName = m_scene.getSelectedObjectName();
    state.objectCount = (int)m_scene.getObjects().size();
    
    // Light state  
    state.lightCount = (int)m_lights.getLightCount();
    
    // Statistics
    state.renderStats = m_renderer.getLastFrameStats();
    
    // Console log
    state.consoleLog = m_consoleLog;
    
    return state;
}

void UIBridge::handleUICommand(const UICommandData& command)
{
    switch (command.command) {
        case UICommand::LoadObject:
            handleLoadObject(command);
            break;
            
        case UICommand::SetRenderMode:
            handleRenderMode(command);
            break;
            
        case UICommand::SetCameraSpeed:
        case UICommand::SetMouseSensitivity:
            handleCameraSettings(command);
            break;
            
        case UICommand::SetGizmoMode:
        case UICommand::ToggleGizmoSpace:
        case UICommand::ToggleSnap:
            handleGizmoSettings(command);
            break;
            
        case UICommand::ExecuteConsoleCommand:
            handleConsoleCommand(command);
            break;
            
        case UICommand::ApplyJsonOps:
            {
                std::string error;
                applyJsonOps(command.stringParam, error);
                if (!error.empty()) {
                    addConsoleMessage("JSON Ops error: " + error);
                }
            }
            break;
            
        case UICommand::RenderToPNG:
            {
                bool success = m_renderer.renderToPNG(m_scene, m_lights, command.stringParam, 
                                                     command.intParam, (int)command.floatParam);
                if (success) {
                    addConsoleMessage("Rendered to: " + command.stringParam);
                } else {
                    addConsoleMessage("Render to PNG failed");
                }
            }
            break;
            
        default:
            addConsoleMessage("Unknown UI command");
            break;
    }
}

void UIBridge::handleLoadObject(const UICommandData& cmd)
{
    bool success = m_scene.loadObject(cmd.stringParam, cmd.stringParam, cmd.vec3Param);
    if (success) {
        addConsoleMessage("Loaded object: " + cmd.stringParam);
    } else {
        addConsoleMessage("Failed to load object: " + cmd.stringParam);
    }
}

void UIBridge::handleRenderMode(const UICommandData& cmd)
{
    m_renderer.setRenderMode(static_cast<RenderMode>(cmd.intParam));
    
    const char* modeNames[] = {"Points", "Wireframe", "Solid", "Raytrace"};
    if (cmd.intParam >= 0 && cmd.intParam < 4) {
        addConsoleMessage("Render mode: " + std::string(modeNames[cmd.intParam]));
    }
}

void UIBridge::handleCameraSettings(const UICommandData& cmd)
{
    switch (cmd.command) {
        case UICommand::SetCameraSpeed:
            m_camera.setSpeed(cmd.floatParam);
            break;
        case UICommand::SetMouseSensitivity:
            m_camera.setSensitivity(cmd.floatParam);
            break;
        default:
            break;
    }
}

void UIBridge::handleGizmoSettings(const UICommandData& cmd)
{
    // Gizmo functionality would be implemented here
    // For now, just log the command
    addConsoleMessage("Gizmo command received");
}

void UIBridge::handleConsoleCommand(const UICommandData& cmd)
{
    // Simple command parsing - could be expanded
    std::string command = cmd.stringParam;
    
    if (command == "help") {
        addConsoleMessage("Available commands: help, clear, render, load");
    }
    else if (command == "clear") {
        clearConsoleLog();
    }
    else if (command.substr(0, 5) == "load ") {
        std::string path = command.substr(5);
        UICommandData loadCmd;
        loadCmd.command = UICommand::LoadObject;
        loadCmd.stringParam = path;
        loadCmd.vec3Param = glm::vec3(0.0f, 0.0f, -2.0f);
        handleLoadObject(loadCmd);
    }
    else {
        addConsoleMessage("Unknown command: " + command + " (type 'help' for commands)");
    }
}

void UIBridge::addConsoleMessage(const std::string& message)
{
    m_consoleLog.push_back(message);
    
    // Limit console log size
    const size_t maxLogSize = 1000;
    if (m_consoleLog.size() > maxLogSize) {
        m_consoleLog.erase(m_consoleLog.begin(), m_consoleLog.begin() + (m_consoleLog.size() - maxLogSize));
    }
}

void UIBridge::clearConsoleLog()
{
    m_consoleLog.clear();
    addConsoleMessage("Console cleared");
}

bool UIBridge::applyJsonOps(const std::string& json, std::string& error)
{
    // This would integrate with the existing JSON Ops system
    // For now, return a placeholder implementation
    error = "JSON Ops integration not implemented in UI bridge yet";
    return false;
}

std::string UIBridge::buildShareLink() const
{
    // This would build a shareable URL with scene state
    return "http://example.com/viewer?state=placeholder";
}

std::string UIBridge::sceneToJson() const
{
    return m_scene.toJson();
}