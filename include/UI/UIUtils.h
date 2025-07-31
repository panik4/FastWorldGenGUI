#pragma once
#include "FastWorldGenerator.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"

enum class InteractionType { NONE, CLICK, RCLICK, DRAG };

struct ClickEvent {
  int pixel;
  InteractionType type;
};

class UIUtils {
private:
  bool enteredTab = false;
  bool forceUpdate = false;
  bool processClickEvents = false;
  bool drawMode = false;
  int brushSize = 1;
  float brushStrength = 1.0f;
  float brushHardness = 1.0f;
  std::vector<int> clickOffsets;
  // std::vector<double> clickStrengths;

public:
  std::map<std::string, std::string> helpTexts;
  std::map<std::string, std::string> advancedHelpTexts;
  std::map<std::string, Fwg::Gfx::Bitmap> advancedHelpImages;
  std::map<std::string, float> advancedHelpTexturesAspectRatio;
  std::map<std::string, ID3D11ShaderResourceView *> advancedHelpTextures;

  std::queue<ClickEvent> clickEvents;
  bool showExtendedHelp = false;
  std::string activeKey = "";
  ID3D11ShaderResourceView *primaryTexture = nullptr;
  ID3D11ShaderResourceView *secondaryTexture = nullptr;
  ID3D11Device *device = nullptr;
  bool updateTexture1;
  bool updateTexture2;
  int textureWidth;
  int textureHeight;

  std::array<Fwg::Gfx::Bitmap, 2> activeImages;
  void resetTexture();
  void setForceUpdate() { forceUpdate = true; }
  bool getForceUpdate() {
    if (forceUpdate) {
      forceUpdate = false;
      return true;
    }
    return false;
  }
  void dirtyTextureUpdate();
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

  bool enterTabEvent() { return enteredTab; }
  bool tabSwitchEvent(bool processClickEvents = false);
  void updateImage(int index, const Fwg::Gfx::Bitmap &image);

  void freeTexture(ID3D11ShaderResourceView **texture);
  // texture utils
  bool getResourceView(const Fwg::Gfx::Bitmap &image,
                       ID3D11ShaderResourceView **out_srv, int *out_width,
                       int *out_height, ID3D11Device *g_pd3dDevice);
  ImGuiIO setupImGuiContextAndStyle();
  HWND createAndConfigureWindow(WNDCLASSEXW &wc, LPCWSTR className,
                                LPCWSTR windowName);
  void setupImGuiBackends(HWND hwnd, ID3D11Device *g_pd3dDevice,
                          ID3D11DeviceContext *g_pd3dDeviceContext);
  void cleanupDirect3DDevice(ID3D11Device *g_pd3dDevice);
  void renderImGui(ID3D11DeviceContext *g_pd3dDeviceContext,
                   ID3D11RenderTargetView *g_mainRenderTargetView,
                   const ImVec4 &clear_color, IDXGISwapChain *g_pSwapChain);
  void shutdownImGui();
  void loadHelpTextsFromFile(const std::string &filePath);
  void loadHelpImagesFromPath(const std::string &filePath);
  void showHelpTextBox(const std::string &key);
  void showAdvancedTextBox();
};

namespace Elements {
void borderChild(const std::string &label,
                 const std::function<void()> &guiCalls);

} // namespace Elements