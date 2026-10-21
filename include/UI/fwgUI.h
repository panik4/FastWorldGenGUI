#pragma once
#include "FastWorldGenerator.h"
#include "UI/AreaUI.h"
#include "UI/ClimateUI.h"
#include "UI/DrawUtils.h"
#include "UI/HeightmapUI.h"
#include "UI/LandUI.h"
#include "UI/PreRequisites.h"
#include "UI/UIContext.h"
#include "UI/UIElements.h"
#include <functional>
#include <future>
#include <string>
#include <thread>
#include <variant>
#include <vector>

namespace Fwg::UI {

class FwgUI {

protected:
  GLFWwindow *window = nullptr;
  Fwg::FastWorldGenerator *standaloneGenerator = nullptr;
  Fwg::UI::UIContext uiContext;
  Fwg::UI::LandUI landUI;
  float uiScaleMultiplier = 1.0f;
  bool closingAfterTask = false;

  void writeCurrentlyDisplayedImage(Fwg::Cfg &cfg) {
    if (uiContext.imageContext.activeImages[0].size() &&
        uiContext.imageContext.activeImages[0].size()) {
      std::string path = cfg.mapsPath;
      path += std::to_string(time(NULL));
      Fwg::Gfx::Png::save(uiContext.imageContext.activeImages[0],
                          path + ".png");
    }
  }

  void initAllowedInput(
      Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
      std::vector<Terrain::LandformDefinition> &landformDefinitions);

  virtual int showGeneric(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  virtual Fwg::FastWorldGenerator &currentGenerator();
  virtual std::string windowTitle() const;
  virtual std::string frameTitle() const;
  virtual void onWindowReady();
  virtual void renderTopBar();
  virtual void renderSettingsTabs(Fwg::Cfg &cfg);
  void renderGenericControls(Fwg::Cfg &cfg);
  virtual void renderGenericContents(Fwg::Cfg &cfg);
  int showElevationTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  int showFwgConfigure(Fwg::Cfg &cfg);

  int showClimateTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);

  int showAreasTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);

protected:
  void logWrapper();
  void imageWrapper(ImGuiIO &io);
  void applyAutoUIScale();
  void init(Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void defaultTabs(Fwg::Cfg &cfg, FastWorldGenerator &fwg);
  void computationRunningCheck();
  int runApplication();
  // Standalone frontends may override this, keeping task polling and dispatch
  // on the UI thread.
  virtual int runLoop(ImGuiIO &io);

  // Helper functions for scaled UI elements
  inline float scaled(float value) const { return uiContext.scaled(value); }

  inline ImVec2 scaled(const ImVec2 &vec) const {
    return uiContext.scaled(vec);
  }

  inline void PushItemWidthScaled(float width) {
    ImGui::PushItemWidth(scaled(width));
  }

public:
  FwgUI();
  virtual ~FwgUI() = default;
  virtual int shiny(Fwg::FastWorldGenerator &fwg);
};
} // namespace Fwg::UI
