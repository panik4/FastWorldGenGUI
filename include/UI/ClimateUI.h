#pragma once
#include "FastWorldGenerator.h"
#include "UI/Prerequisites.h"
#include "UI/UIContext.h"
#include "UI/UiElements.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "imgui.h"
#include <string>
#include <vector>

namespace Fwg::UI::Climate {

namespace Input {
bool RenderScrollableClimateInput(std::vector<Fwg::Gfx::Colour> &imageData,
                                  UIContext &uiContext);
bool analyzeClimateMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                       const Fwg::Gfx::Image &climateInput,
                       UIContext &uiContext);
bool complexTerrainMapping(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                           UIContext &uiContext);

} // namespace Input

int showTemperatureMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                       UIContext &uiContext);
int showHumidityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext);
int showRiverTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                 UIContext &uiContext);
int showClimateTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                   UIContext &uiContext);
int showTreeTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                UIContext &uiContext);
int showWastelandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext);

} // namespace Fwg::UI::Climate