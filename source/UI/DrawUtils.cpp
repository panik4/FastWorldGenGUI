#include "UI/DrawUtils.h"

namespace Fwg::UI::Drawing {

void imageClick(ImGuiIO &io, UIContext &context) {
  // ensure we have an image to click on
  if (!context.imageContext.activeImages[0].size()) {
    return;
  }
  // Check if the mouse is clicked on the image
  if ((ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) ||
       ImGui::IsMouseReleased(1))) {
    // Determine which mouse button was pressed
    auto pressedKey =
        ImGui::IsMouseReleased(0) ? ImGuiKey_MouseLeft : ImGuiKey_MouseRight;

    // Get the mouse position relative to the image
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 imagePos = ImGui::GetItemRectMin();
    auto itemSize = ImGui::GetItemRectSize();
    ImVec2 mousePosRelative =
        ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

    // Calculate the pixel position in the texture
    int pixelX = static_cast<int>((mousePosRelative.x / itemSize.x) *
                                  context.imageContext.activeImages[0].width());
    int pixelY = context.imageContext.activeImages[0].height() -
                 static_cast<int>((mousePosRelative.y / itemSize.y) *
                         context.imageContext.activeImages[0].height());

    // Determine the type of interaction based on the mouse button pressed and
    // whether the Ctrl key is held down
    auto interactionType = pressedKey == ImGuiKey_MouseLeft
                               ? InteractionType::CLICK
                               : InteractionType::RCLICK;

    // Calculate the index of the pixel in the texture data
    int pixelIndex =
        (pixelY * context.imageContext.activeImages[0].width() + pixelX);

    // If click events are being processed, add this event to the queue
    if (context.drawContext.processClickEvents) {
      if (context.drawContext.clickEvents.size() < 1) {
        context.drawContext.clickEvents.push({pixelIndex, interactionType});
      }
    }
  }
}
ClickEvent getClickedPixel() {
  // if (clickEvents.size() > 0) {
  //   ClickEvent event = clickEvents.front();
  //   clickEvents.pop();
  //   return event;
  // }
  return {
      -1,
      InteractionType::NONE,
  };
}
std::vector<std::pair<ClickEvent, double>>
getAffectedPixels(const ClickEvent &clickEvent) {
  // if (clickEvent.pixel < 0)
  //   return {};
  // auto &cfg = Fwg::Cfg::Values();

  // std::vector<std::pair<ClickEvent, double>> affectedPixels;
  // for (int i = 0; i < clickOffsets.size(); i++) {
  //   auto pix = clickEvent.pixel + clickOffsets[i];
  //   if (pix < cfg.processingArea && pix >= 0) {
  //     auto strength = brushStrength; /* *
  //         (brushHardness + (1.0 - brushHardness) * (1.0 -
  //         clickStrengths[i]));*/
  //     affectedPixels.push_back(
  //         std::make_pair(ClickEvent{pix, clickEvent.type}, strength));
  //   }
  // }
  // return affectedPixels;
  return {};
}
std::vector<std::pair<ClickEvent, double>> getLatestAffectedPixels() {
  return getAffectedPixels(getClickedPixel());
}
void brushSettingsHeader() {
  // auto &cfg = Fwg::Cfg::Values();
  // bool update = false;
  // if (ImGui::SliderInt("<--Brushsize", &brushSize, 0, 100)) {
  //   update = true;
  // }

  // ImGui::SameLine();
  // ImGui::SliderFloat("<--Brush strength", &brushStrength, 0.0f, 1.0f);
  // ImGui::SameLine();
  //  showHelpTextBox("Drawing");
  //   ImGui::SameLine();
  //   ImGui::SliderFloat("<--Brush hardness", &brushHardness, 0.0f, 1.0f);
  // if (update) {
  //   setClickOffsets(cfg.width, brushSize);
  //  for (int i = 0; i < clickOffsets.size(); i++) {
  //    auto p1 = clickOffsets[i];
  //    auto p2 = 0;
  //    auto xmod = abs(p1 % cfg.width);
  //    const double x1 = xmod < cfg.width / 2 ? xmod : xmod - cfg.width;
  //    const double x2 = p2 % cfg.width;
  //    const double y1 = (double)p1 / (double)cfg.width;
  //    const double y2 = (double)p2 / (double)cfg.width;
  //    clickStrengths.push_back(
  //        (sqrt(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)))) /
  //        (double)brushSize);
  //  }
  //}
}

void setClickOffsets(int width, int brushSize) {
  // clickOffsets = Fwg::Utils::Math::getCircularOffsets(width, brushSize);
}

} // namespace Fwg::UI::Drawing