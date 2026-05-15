#pragma once
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"
#include "UI/UiElements.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#include "rendering/Image.h"

namespace Fwg::UI::Utils {
void freeTexture(GLuint *texture);
bool getResourceView(const Fwg::Gfx::Image &image, GLuint *out_tex,
                     int *out_width, int *out_height);
ImGuiIO &setupImGuiContextAndStyle();
GLFWwindow *createAndConfigureWindow(int width, int height, const char *title);
void setupImGuiBackends(GLFWwindow *window);
void renderImGui(const ImVec4 &clear_color, GLFWwindow *window);
void shutdownImGui();
bool CreateDeviceGL(GLFWwindow *&window, const char *title, int width,
                    int height);
void CleanupDeviceGL(GLFWwindow *&window);
template <typename S> static bool longCircuitLogicalOr(const S first) {
  return first;
}

template <typename S, typename... Args>
static bool longCircuitLogicalOr(const S first, const Args... args) {
  return first || longCircuitLogicalOr(args...);
}

} // namespace Fwg::UI::Utils

namespace Fwg::UI::Utils::Masks {
std::vector<std::vector<int>>
getLandmaskEvaluationAreas(std::vector<bool> &mask);
}
