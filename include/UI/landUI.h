#pragma once
#include "FastWorldGenerator.h"
#include "UIUtils.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#include <string>
#include <tchar.h>
#include <vector>
namespace Fwg {
struct ElevationInput {
  Fwg::Gfx::Colour in;
  Fwg::Gfx::Colour out;
  std::string rgbName;
  Fwg::Terrain::ElevationType type;
  ImVec4 colour;
  std::vector<int> pixels;
};

class LandUI {
private:
  std::shared_ptr<UIUtils> uiUtils;
  std::unordered_set<std::string> highlightedInputs;
  void RenderScrollableLandInput(std::vector<Fwg::Gfx::Colour> &imageData);
  bool analyzeLandMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                      const Fwg::Gfx::Bitmap &landInput,
                      int &amountClassificationsNeeded);

public:
  LandUI();
  LandUI(std::shared_ptr<UIUtils> uiUtils);
  Fwg::Gfx::Bitmap landInput;
  std::string loadedTerrainFile;
  bool classificationNeeded = true;
  void complexLandMapping(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                          bool &analyze, int &amountClassificationsNeeded);
  Fwg::Utils::ColourTMap<ElevationInput> landInputColours;
  Fwg::Utils::ColourTMap<Fwg::Terrain::ElevationType> allowedLandInputs;
  void triggeredLandInput(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                          const std::string &draggedFile, bool classifyInput);
  void draw(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
  void configureLandElevationFactors(Fwg::Cfg &cfg,
                                     Fwg::FastWorldGenerator &fwg);
};
} // namespace Fwg