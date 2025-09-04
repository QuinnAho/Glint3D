#pragma once
#include <memory>
#include <string>
#include "app_state.h"
#include "app_commands.h"

// Forward-declare GLFWwindow to avoid dragging app headers here
struct GLFWwindow;

class ImGuiLayer {
public:
  ImGuiLayer(GLFWwindow* window, bool enableDocking = true);
  ~ImGuiLayer();

  void BeginFrame();
  void Render(const AppStateView& s, const AppCommands& c);
  void EndFrame();

  bool WantsMouse() const;
  bool WantsKeyboard() const;
  bool WantsAnyInput() const { return WantsMouse() || WantsKeyboard(); }


private:
  void RenderDockspace_();
  void RenderMenuBar_(const AppStateView& s, const AppCommands& c);
  void RenderSettings_(const AppStateView& s, const AppCommands& c);
  void RenderPerfHud_(const AppStateView& s);
  void RenderConsoleWithAi_(const AppStateView& s, const AppCommands& c);
  void ApplyModernTheme_(float scale=1.0f);
};
