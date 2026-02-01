#pragma once
#include "ClimateUI.h"
#include "FastWorldGenerator.h"
#include "UI/UiElements.h"
#include "LandUI.h"
#include <atomic>
#include <functional>
#include <future>
#include <string>
#include <thread>
#include <variant>
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
  bool modifiedAreas = false;

  
  std::vector<std::string> heightmapConfigFiles;
  GLFWwindow *window = nullptr;


  void writeCurrentlyDisplayedImage(Fwg::Cfg &cfg) {
    if (uiUtils->activeImages[0].size() && uiUtils->activeImages[0].size()) {
      std::string path = cfg.mapsPath + "//";
      path += std::to_string(time(NULL));
      Fwg::Gfx::Bmp::save(uiUtils->activeImages[0], path + ".bmp");
      Fwg::Gfx::Png::save(uiUtils->activeImages[0], path + ".png");
    }
  }

  void disableBlock(const Fwg::Gfx::Image &image);
  void reenableBlock(const Fwg::Gfx::Image &image);
  void initAllowedInput(Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
                        std::vector<Terrain::LandformDefinition> &landformDefinitions);

  void clearColours(Fwg::Gfx::Image &image);

  void loadHeightmapConfigs();

  virtual int showGeneric(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void areaInputSelector(Fwg::Cfg &cfg);
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

  // Data

  // UTILS:
  bool CreateDeviceGL(const char *title, int width, int height);
  void CleanupDeviceGL();

  static bool optionalInput(bool condition, std::function<bool()> fn) {
    return condition ? fn() : false;
  }

  template <typename S> static bool longCircuitLogicalOr(const S first) {
    return first;
  }

  template <typename S, typename... Args>
  static bool longCircuitLogicalOr(const S first, const Args... args) {
    return first || longCircuitLogicalOr(args...);
  }


public:
  FwgUI();
  bool initializeGraphics();
  void initializeImGui();
  void cleanup();
  void genericWrapper(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void logWrapper();
  void imageWrapper(ImGuiIO &io);
  void init(Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void defaultTabs(Fwg::Cfg &cfg, FastWorldGenerator &fwg);
  void computationRunningCheck();
  int shiny(Fwg::FastWorldGenerator &fwg);
};
} // namespace Fwg
