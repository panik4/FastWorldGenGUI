#pragma once
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include "FastWorldGenerator.h"
#include "GLFW/glfw3.h"
#include "UI/UiElements.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"

enum class InteractionType { NONE, CLICK, RCLICK };

struct ClickEvent {
  int pixel;
  InteractionType type;
};

class UIUtils {

public:
  std::map<std::string, std::string> helpTexts;
  std::map<std::string, std::string> advancedHelpTexts;
  std::map<std::string, Fwg::Gfx::Image> advancedHelpImages;
  std::map<std::string, float> advancedHelpTexturesAspectRatio;
  std::map<std::string, GLuint> advancedHelpTextures;

  std::queue<ClickEvent> clickEvents;
  bool showExtendedHelp = false;
  std::string activeKey = "";
  GLuint primaryTexture = 0;
  GLuint secondaryTexture = 0;

  bool updateTexture1;
  bool updateTexture2;
  int textureWidth;
  int textureHeight;

  std::array<Fwg::Gfx::Image, 2> activeImages;
  void resetTexture();
  void setForceUpdate() { forceUpdate = true; }
  bool getForceUpdate() {
    if (forceUpdate) {
      forceUpdate = false;
      return true;
    }
    return false;
  }
  bool getDrawMode() { return drawMode; }
  void setDrawMode(bool mode) { drawMode = mode; }
  bool processClickEventsEnabled() { return processClickEvents; }
  // This function handles the click events on an image.
  void imageClick(float scale, ImGuiIO &io);

  ClickEvent getClickedPixel();
  std::queue<ClickEvent> getClickEvents() { return clickEvents; }
  std::vector<std::pair<ClickEvent, double>>
  getAffectedPixels(const ClickEvent &clickedPixel);
  std::vector<std::pair<ClickEvent, double>> getLatestAffectedPixels();
  void brushSettingsHeader();
  void setClickOffsets(int width, int brushSize);
  void resetTexture(int id);
  template <typename T>
  bool simpleDraw(const std::vector<bool> &landMask,
                  std::vector<T> &modifiedData, const double multiplier) {
    // for drawing
    auto affected = getLatestAffectedPixels();
    if (affected.size() > 0) {
      for (auto &pix : affected) {
        if (landMask[pix.first.pixel]) {
          if (pix.first.type == InteractionType::CLICK) {
            modifiedData[pix.first.pixel] =
                static_cast<T>(pix.second * multiplier);
          } else if (pix.first.type == InteractionType::RCLICK) {
            modifiedData[pix.first.pixel] = 0;
          }
        }
      }

      resetTexture();
      return true;
    } else {
      return false;
    }
  }

  bool tabSwitchEvent(bool processClickEvents = false);
  void updateImage(int index, const Fwg::Gfx::Image &image);

  void freeTexture(GLuint *texture);

  bool getResourceView(const Fwg::Gfx::Image &image, GLuint *out_tex,
                       int *out_width, int *out_height);

  ImGuiIO &setupImGuiContextAndStyle();
  GLFWwindow *createAndConfigureWindow(int width, int height,
                                       const char *title);
  void setupImGuiBackends(GLFWwindow *window);
  void cleanupGLResources();
  void renderImGui(const ImVec4 &clear_color, GLFWwindow *window);
  void shutdownImGui();
  void loadHelpTextsFromFile(const std::string &filePath);
  void loadHelpImagesFromPath(const std::string &filePath);
  void showHelpTextBox(const std::string &key, bool switchKey = true);
  void showAdvancedTextBox();

  // Image registry per tab
  struct ImageOption {
    std::string name;
    Fwg::Gfx::Image image;
    bool isLoaded = false;                      // Lazy load flag
    std::function<Fwg::Gfx::Image()> generator; // Optional: generate on demand
  };

  // Register available images for a tab
  void registerTabImages(const std::string &tabName,
                         const std::vector<ImageOption> &options);

  // Get current selection index for a tab
  int getCurrentImageIndex(const std::string &tabName) const;

  // Switch to a different image in the tab
  void switchTabImage(const std::string &tabName, int index);

  // UI widget to show image selector
  bool renderImageSelector(const std::string &tabName);

private:
  bool forceUpdate = false;
  bool processClickEvents = false;
  bool drawMode = false;
  int brushSize = 1;
  float brushStrength = 1.0f;
  float brushHardness = 1.0f;
  std::vector<int> clickOffsets;
  // Storage: tabName -> available images
  std::map<std::string, std::vector<ImageOption>> tabImageRegistry;

  // Storage: tabName -> currently selected index
  std::map<std::string, int> tabImageSelection;
  // std::vector<double> clickStrengths;
};

namespace Fwg::UI::Utils::Masks {
std::vector<std::vector<int>>
getLandmaskEvaluationAreas(std::vector<bool> &mask);
}

namespace Fwg::UI::Elements {
void borderChild(const std::string &label,
                 const std::function<void()> &guiCalls);

} // namespace Fwg::UI::Elements