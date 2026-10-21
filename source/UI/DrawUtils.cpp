#include "UI/DrawUtils.h"
#include <algorithm>

namespace Fwg::UI::Drawing {

void imageClick(ImGuiIO &, UIContext &context) {
  if (context.asyncContext.busy() || !context.drawContext.processClickEvents ||
      !context.drawContext.clickEvents.empty() || !ImGui::IsItemHovered() ||
      (!ImGui::IsMouseReleased(0) && !ImGui::IsMouseReleased(1)))
    return;

  const auto &image = context.imageContext.activeImages[0];
  const auto itemSize = ImGui::GetItemRectSize();
  if (!image.size() || image.width() <= 0 || image.height() <= 0 ||
      itemSize.x <= 0.0f || itemSize.y <= 0.0f)
    return;

  const auto mouse = ImGui::GetMousePos();
  const auto origin = ImGui::GetItemRectMin();
  const float x = (mouse.x - origin.x) / itemSize.x;
  const float y = (mouse.y - origin.y) / itemSize.y;
  if (x < 0.0f || x >= 1.0f || y < 0.0f || y >= 1.0f)
    return;

  const int pixelX =
      std::clamp(static_cast<int>(x * image.width()), 0, image.width() - 1);
  const int pixelY =
      image.height() - 1 -
      std::clamp(static_cast<int>(y * image.height()), 0, image.height() - 1);
  const auto interaction = ImGui::IsMouseReleased(0) ? InteractionType::CLICK
                                                     : InteractionType::RCLICK;
  context.drawContext.clickEvents.push(
      {pixelY * image.width() + pixelX, interaction});
}

} // namespace Fwg::UI::Drawing
