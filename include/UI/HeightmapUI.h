#pragma once
#include "FastWorldGenerator.h"
#include "UI/Prerequisites.h"
#include "UI/UIContext.h"
#include "UI/UIElements.h"

namespace Fwg::UI::Heightmap {
namespace Detail {
void renderOperationParameters(Fwg::Terrain::HeightmapOperation &operation);
void configurePipelineEditor(Fwg::Cfg &cfg, UIContext &uiContext);
} // namespace Detail

void configureLandElevationFactors(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg);
void loadHeightmapConfigs(UIContext &uiContext);
int showHeightmapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext);
int showNormalMapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext);

} // namespace Fwg::UI::Heightmap