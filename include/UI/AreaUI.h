#pragma once
#include "FastWorldGenerator.h"
#include "UI/Prerequisites.h"
#include "UI/UIContext.h"
#include "UI/UiElements.h"

namespace Fwg::UI::Areas {

void areaInputSelector(Fwg::Cfg &cfg);
int showDensityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                   UIContext &uiContext);
void showSuperSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                         UIContext &uiContext);
void showSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext);

int showProvincesTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext);
int showRegionTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                  UIContext &uiContext);
int showContinentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext);

} // namespace Fwg::UI::Areas