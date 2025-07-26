#pragma once
#include "ClimateUI.h"
#include "FastWorldGenerator.h"
#include "landUI.h"
#include <atomic>
#include <d3d11.h>
#include <functional>
#include <future>
#include <string>
#include <tchar.h>
#include <thread>
#include <vector>

namespace Fwg {

class FwgUI {

protected:
  // Function wrapper to run any function asynchronously
  template <typename Func, typename... Args>
  auto runAsync(Func func, Args &...args) {
    computationStarted = true;
    computationRunning = true;
    return std::async(std::launch::async, func, std::ref(args)...);
  }
  template <typename Func, typename... Args>
  auto runAsyncInitialDisable(Func func, Args &...args) {
    computationRunning = true;
    worldGenerationView = true;
    return std::async(std::launch::async, func, std::ref(args)...);
  }
  std::atomic<bool> computationRunning;
  std::atomic<bool> computationStarted;
  std::future<void> computationFuture;
  std::future<bool> computationFutureBool;
  bool worldGenerationView = false;
  std::string originalLandInput = "";
  std::shared_ptr<UIUtils> uiUtils;
  LandUI landUI;
  ClimateUI climateUI;
  bool update = false;
  std::string draggedFile = "";
  bool triggeredDrag = false;
  bool loadedConfigs = false;
  std::string activeConfig;
  std::shared_ptr<std::stringstream> log;
  float frequency = 0.0;
  float zoom = 1.0f;
  static int seed;

  bool redoTerrain = false;
  bool redoHumidity = false;
  bool redoClimate = false;
  bool redoAreas = false;
  bool redoTrees = false;
  bool redoProvinces = false;
  bool redoRegions = false;

  void writeCurrentlyDisplayedImage(Fwg::Cfg &cfg) {
    if (uiUtils->activeImages[0].size() && uiUtils->activeImages[0].size()) {
      std::string path = cfg.mapsPath + "//";
      path += std::to_string(time(NULL));
      Fwg::Gfx::Bmp::save(uiUtils->activeImages[0], path + ".bmp");
      Fwg::Gfx::Png::save(uiUtils->activeImages[0], path + ".png");
    }
  }

  void disableBlock(const Fwg::Gfx::Bitmap &bitmap);
  void reenableBlock(const Fwg::Gfx::Bitmap &bitmap);
  void initAllowedInput(Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
                        std::vector<Terrain::ElevationType> &elevationTypes);

  void clearColours(Fwg::Gfx::Bitmap &image);
  virtual int showGeneric(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showElevationTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showFwgConfigure(Fwg::Cfg &cfg);
  int showCutCfg(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showHeightmapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showLandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);

  int showNormalMapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showClimateInputTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showClimateOverview(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showTemperatureMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showHumidityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showRiverTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showClimateTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showTreeTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showWastelandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showAreasTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showDensityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showSuperSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);

  int showProvincesTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showRegionTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showContinentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showCivilizationTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showDevelopmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showPopulationTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showUrbanisationTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showAgricultureTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showLocationTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void showNavmeshTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  // Data
  static ID3D11Device *g_pd3dDevice;
  static ID3D11DeviceContext *g_pd3dDeviceContext;
  static IDXGISwapChain *g_pSwapChain;
  static ID3D11RenderTargetView *g_mainRenderTargetView;

  // UTILS:
  bool CreateDeviceD3D(HWND hWnd);
  void CleanupDeviceD3D();
  void CreateRenderTarget();
  void CleanupRenderTarget();

  template <typename S> static bool longCircuitLogicalOr(const S first) {
    return first;
  }

  template <typename S, typename... Args>
  static bool longCircuitLogicalOr(const S first, const Args... args) {
    return first || longCircuitLogicalOr(args...);
  }

public:
  FwgUI();
  WNDCLASSEXW initializeWindowClass();
  bool initializeGraphics(HWND hwnd);
  void initializeImGui(HWND hwnd);
  void genericWrapper();
  void logWrapper();
  void imageWrapper(ImGuiIO &io);
  int init(Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void initDraggingPoll(bool &done);
  void defaultTabs(Fwg::Cfg &cfg, FastWorldGenerator &fwg);
  void computationRunningCheck();
  HWND createMainWindow(const WNDCLASSEXW &wc);
  void cleanup(HWND hwnd, const WNDCLASSEXW &wc);
  int shiny(Fwg::FastWorldGenerator &fwg);
};
} // namespace Fwg
