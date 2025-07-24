#pragma once
#include "FastWorldGenerator.h"
#include "imgui.h"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include <string>
#include <tchar.h>
#include <vector>
class ClimateUI {
  struct ClimateInput {
    Fwg::Gfx::Colour in;
    Fwg::Gfx::Colour out;
    std::string rgbName;
    Fwg::Climate::ClimateType climate;
    ImVec4 colour;
    std::vector<int> pixels;
  };




public:
  void RenderScrollableArea(std::vector<Fwg::Gfx::Colour> &imageData);
  bool analyzeClimateMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                         const Fwg::Gfx::Bitmap &climateInput,
                         int &amountClassificationsNeeded);
  void complexTerrainMapping(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                             bool &analyze, int &amountClassificationsNeeded);

  
  Fwg::Gfx::Bitmap climateInputMap;
  Fwg::Utils::ColourTMap<ClimateInput> climateInputColours;
  Fwg::Utils::ColourTMap<Fwg::Climate::ClimateType>
      allowedClimateInputs;



};
