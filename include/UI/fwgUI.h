#pragma once
#include "ClimateUI.h"
#include "FastWorldGenerator.h"
#include "HeightmapUI.h"
#include "LandUI.h"
#include "UI/AreaUI.h"
#include "UI/DrawUtils.h"
#include "UI/PreRequisites.h"
#include "UI/UIContext.h"
#include "UI/UiElements.h"
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
  GLFWwindow *window = nullptr;
  std::shared_ptr<std::stringstream> log;
  Fwg::UI::UIContext uiContext;
  Fwg::UI::HeightmapUI heightmapUI;
  LandUI landUI;

  void writeCurrentlyDisplayedImage(Fwg::Cfg &cfg) {
    if (uiContext.imageContext.activeImages[0].size() &&
        uiContext.imageContext.activeImages[0].size()) {
      std::string path = cfg.mapsPath + "/";
      path += std::to_string(time(NULL));
      Fwg::Gfx::Png::save(uiContext.imageContext.activeImages[0],
                          path + ".png");
    }
  }

  void initAllowedInput(
      Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
      std::vector<Terrain::LandformDefinition> &landformDefinitions);

  virtual int showGeneric(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showElevationTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showFwgConfigure(Fwg::Cfg &cfg);
  int showLandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);

  int showNormalMapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showClimateInputTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showClimateOverview(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);

  int showAreasTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);

protected:
  void genericWrapper(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void logWrapper();
  void imageWrapper(ImGuiIO &io);
  void init(Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void defaultTabs(Fwg::Cfg &cfg, FastWorldGenerator &fwg);
  void computationRunningCheck();

public:
  FwgUI();
  int shiny(Fwg::FastWorldGenerator &fwg);
};
} // namespace Fwg
