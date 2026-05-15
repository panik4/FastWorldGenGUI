#include "UI/UIUtils.h"

namespace Fwg::UI::Utils::Masks {
std::vector<std::vector<int>>
getLandmaskEvaluationAreas(std::vector<bool> &mask) {
  std::vector<std::vector<int>> evaluationAreas(2);
  for (int i = 0; auto pix : mask) {
    if (pix) {
      evaluationAreas[0].push_back(i);
    } else {
      evaluationAreas[1].push_back(i);
    }
    i++;
  }
  return evaluationAreas;
}

} // namespace Fwg::UI::Utils::Masks

namespace Fwg::UI::Utils {
void freeTexture(GLuint *texture) {
  if (texture && *texture != 0) {
    glDeleteTextures(1, texture);
    *texture = 0;
  }
}
bool getResourceView(const Fwg::Gfx::Image &image, GLuint *out_tex,
                     int *out_width, int *out_height) {
  if (!out_tex)
    return false;

  const int w = image.width();
  const int h = image.height();
  const std::vector<unsigned char> pixels = image.getFlipped32bit();

  *out_tex = 0;

  glGenTextures(1, out_tex);
  glBindTexture(GL_TEXTURE_2D, *out_tex);

  // Texture upload
  glTexImage2D(GL_TEXTURE_2D, 0,
               GL_RGBA, // internal format
               w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  // Basic sampling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);

  if (out_width)
    *out_width = w;
  if (out_height)
    *out_height = h;

  return true;
}

ImGuiIO &setupImGuiContextAndStyle() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();
  return io;
}

GLFWwindow *createAndConfigureWindow(int width, int height, const char *title) {
  if (!glfwInit())
    return nullptr;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window)
    return nullptr;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // vsync

  // Load GL functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return nullptr;

  return window;
}

void setupImGuiBackends(GLFWwindow *window) {
  // GLFW backend
  ImGui_ImplGlfw_InitForOpenGL(window, true);

  // OpenGL3 backend (GLSL 130 core is the safest cross-platform default)
  ImGui_ImplOpenGL3_Init("#version 130 core");
}

void renderImGui(const ImVec4 &clear_color, GLFWwindow *window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  const float clear_color_with_alpha[4] = {
      clear_color.x * clear_color.w, clear_color.y * clear_color.w,
      clear_color.z * clear_color.w, clear_color.w};

  glClearColor(clear_color_with_alpha[0], clear_color_with_alpha[1],
               clear_color_with_alpha[2], clear_color_with_alpha[3]);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);
}

void shutdownImGui() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

bool CreateDeviceGL(GLFWwindow *&window, const char *title, int width,
                    int height) {
  if (!glfwInit()) {
    return false;
  }

  // Configure GLFW for modern OpenGL
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWmonitor *primary = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(primary);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  // Create a windowed fullscreen (borderless) window
  window = glfwCreateWindow(mode->width, mode->height, title, nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return false;
  }

  glfwMaximizeWindow(window);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Load GL through GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return false;

  return true;
}

void CleanupDeviceGL(GLFWwindow *&window) {
  if (window) {
    glfwDestroyWindow(window);
    window = nullptr;
  }

  glfwTerminate();
}

} // namespace Fwg::UI::Utils