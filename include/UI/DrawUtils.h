#pragma once
#include "UI/UIContext.h"
#include "imgui.h"

namespace Fwg::UI::Drawing {

// This function handles the click events on an image.
void imageClick(ImGuiIO &io, UIContext &context);

} // namespace Fwg::UI::Drawing