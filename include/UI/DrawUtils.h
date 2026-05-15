#pragma once
#include "UI/UIContext.h"
#include "imgui.h"
#include <queue>

namespace Fwg::UI::Drawing {

// This function handles the click events on an image.
void imageClick(ImGuiIO &io, UIContext &context);

ClickEvent getClickedPixel();
std::vector<std::pair<ClickEvent, double>>
getAffectedPixels(const ClickEvent &clickedPixel);
std::vector<std::pair<ClickEvent, double>> getLatestAffectedPixels();
void brushSettingsHeader();
void setClickOffsets(int width, int brushSize);
template <typename T>
bool simpleDraw(const std::vector<bool> &landMask, std::vector<T> &modifiedData,
                const double multiplier) {
  // for drawing
  auto affected = getLatestAffectedPixels();
  if (affected.size() > 0) {
    for (auto &pix : affected) {
      if (landMask[pix.first.pixel]) {
        if (pix.first.type == InteractionType::CLICK) {
          modifiedData[pix.first.pixel] =
              static_cast<T>(pix.second * multiplier);
        } else if (pix.first.type == InteractionType::RCLICK) {
          modifiedData[pix.first.pixel] = 0;
        }
      }
    }
    return true;
  } else {
    return false;
  }
}

} // namespace Fwg::UI::Drawing