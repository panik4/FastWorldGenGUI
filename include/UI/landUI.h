#pragma once
#include "FastWorldGenerator.h"
#include "UI/InputUI.h"
#include "UI/UIUtils.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#include <string>
#include <vector>
namespace Fwg {
struct ElevationInput {
  Fwg::Gfx::Colour in;
  Fwg::Gfx::Colour out;
  std::string rgbName;
  Fwg::Terrain::LandformDefinition type;
  ImVec4 colour;
  std::vector<int> pixels;
};

class LandUI {
private:
  std::shared_ptr<UIUtils> uiUtils;
  std::set<Fwg::Gfx::Colour> highlightedInputs;

  void RenderScrollableLandInput(
      std::vector<Fwg::Gfx::Colour> &imageData,
      const std::vector<Fwg::Terrain::LandformDefinition> &landformDefinitions);
  bool analyseLandMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                      const Fwg::Gfx::Image &landInput,
                      int &amountClassificationsNeeded);

public:
  LandUI();
  LandUI(std::shared_ptr<UIUtils> uiUtils);
  Fwg::Gfx::Image landInput;
  std::string loadedTerrainFile;
  bool classificationNeeded = true;
  void complexLandMapping(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                          bool &analyze, int &amountClassificationsNeeded);
  Fwg::Utils::ColourTMap<ElevationInput> landInputColours;
  Fwg::Utils::ColourTMap<Fwg::Terrain::LandformDefinition> allowedLandInputs;
  void triggeredLandInput(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                          const std::string &draggedFile,
                          const Fwg::Terrain::InputMode &inputMode);
  void draw(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void configureLandElevationFactors(Fwg::Cfg &cfg,
                                     Fwg::FastWorldGenerator &fwg);
};
} // namespace Fwg