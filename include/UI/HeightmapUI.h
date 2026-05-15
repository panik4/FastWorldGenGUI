#pragma once
#include "FastWorldGenerator.h"
#include "UI/Prerequisites.h"
#include "UI/UIContext.h"
#include "UI/UiElements.h"

namespace Fwg::UI {
class HeightmapUI {
private:
  int selectedOperationIndex;
  std::vector<std::string> heightmapConfigFiles;
  void renderOperationParameters(Fwg::Terrain::HeightmapOperation &operation);
  void configureLandElevationFactors(Fwg::Cfg &cfg,
                                     Fwg::FastWorldGenerator &fwg);
  void configurePipelineEditor(Fwg::Cfg &cfg);

public:
  void loadHeightmapConfigs();
  int showHeightmapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                       UIContext &uiContext);
};
} // namespace Fwg::UI