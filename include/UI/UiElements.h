#pragma once
#include <functional>
#include <imgui.h>
#include <string>

namespace Fwg::UI {

namespace Elements {
  // Begin a prominently styled MAIN tab bar
  bool BeginMainTabBar(const std::string &label);

  // End the MAIN tab bar
  void EndMainTabBar();

  // Begin a visually distinct SUB tab bar
  bool BeginSubTabBar(const std::string &label, float height = 0.0f);

  // End the SUB tab bar
  void EndSubTabBar();

  bool BeginMainTabItem(const std::string &label,
                               const bool highlight = false);

  bool BeginSubTabItem(const std::string &label,
                              const bool highlight = false);

  // Stylized button (e.g., red = important)
  static bool Button(const std::string &label, bool emphasize = false,
                     ImVec2 size = ImVec2(0, 0)) {
    if (emphasize) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
    }

    bool clicked = ImGui::Button(label.c_str(), size);

    if (emphasize) {
      ImGui::PopStyleColor(3);
    }

    return clicked;
  }

  // Emphasized green button for important confirmation steps
  static bool ImportantStepButton(const std::string &label,
                                  ImVec2 size = ImVec2(0, 0)) {
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.2f, 0.8f, 0.2f, 1.0f)); // Green
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.3f, 0.9f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.1f, 0.7f, 0.1f, 1.0f));

    // Slightly larger button
    ImVec2 finalSize = size.x == 0 && size.y == 0 ? ImVec2(150, 0) : size;
    bool clicked = ImGui::Button(label.c_str(), finalSize);

    ImGui::PopStyleColor(3);
    return clicked;
  }
  static bool HelpButton(const std::string &tooltip = "Extended Help") {
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.85f, 0.75f, 0.2f, 0.9f)); // Soft yellow
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.95f, 0.85f, 0.3f, 1.0f)); // Brighter yellow
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.8f, 0.7f, 0.2f, 1.0f)); // Slightly darker when pressed

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f); // Fully rounded
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

    bool pressed =
        ImGui::Button("Click here for more info on this tab", ImVec2(0, 0));

    if (ImGui::IsItemHovered() && !tooltip.empty()) {
      ImGui::SetTooltip("%s", tooltip.c_str());
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);

    return pressed;
  }
  static bool AutomationStepButton(const std::string &label,
                                   const ImVec2 &size = ImVec2(0, 0)) {
    // Push blue-ish theme colors
    ImGui::PushStyleColor(ImGuiCol_Button,
                          ImVec4(0.2f, 0.45f, 0.8f, 1.0f)); // base
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                          ImVec4(0.3f, 0.55f, 0.9f, 1.0f)); // hover
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                          ImVec4(0.15f, 0.4f, 0.75f, 1.0f)); // click

    // Make button a little larger + more padding
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f); // Rounded corners

    // Draw button
    bool pressed = ImGui::Button(label.c_str(), size);

    // Pop in reverse order
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);

    return pressed;
  }

  static bool LabeledInputInt(const std::string &label, int &value,
                              int step = 1, int stepFast = 10,
                              int min = INT_MIN, int max = INT_MAX) {
    bool changed = false;

    ImGui::PushID(label.c_str());

    // Get cursor position for drawing the frame
    ImVec2 frameStart = ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();

    // Label
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.c_str());
    ImGui::SameLine();

    // Input
    changed = ImGui::InputInt("##input", &value, step, stepFast);

    // Clamp
    if (changed) {
      if (value < min)
        value = min;
      if (value > max)
        value = max;
    }

    ImGui::EndGroup();

    // Get end of the group to compute rectangle size
    ImVec2 frameEnd = ImGui::GetItemRectMax();
    ImVec2 padding = ImGui::GetStyle().FramePadding;
    frameStart.x -= padding.x;
    frameStart.y -= padding.y;
    frameEnd.x += padding.x;
    frameEnd.y += padding.y;

    // Draw a frame rectangle around the label + input
    ImGui::GetWindowDrawList()->AddRect(
        frameStart, frameEnd,
        ImGui::GetColorU32(ImGuiCol_Border) // Rounded corners (radius = 0)
    );

    ImGui::PopID();
    return changed;
  }

  static bool LabeledInputFloat(const std::string &label, float &value,
                                float step = 0.1f, float stepFast = 1.0f,
                                float min = -FLT_MAX, float max = FLT_MAX) {
    bool changed = false;

    ImGui::PushID(label.c_str());

    // Align label to the left
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.c_str());
    ImGui::SameLine();

    changed = ImGui::InputFloat("##input", &value, step, stepFast);

    if (changed) {
      if (value < min)
        value = min;
      if (value > max)
        value = max;
    }

    ImGui::PopID();
    return changed;
  }

  static bool LabeledInputDouble(const std::string &label, double &value,
                                 double step = 0.1, double stepFast = 1.0,
                                 double min = -DBL_MAX, double max = DBL_MAX) {
    bool changed = false;

    ImGui::PushID(label.c_str());

    // Align label to the left
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label.c_str());
    ImGui::SameLine();

    changed = ImGui::InputDouble("##input", &value, step, stepFast);

    if (changed) {
      if (value < min)
        value = min;
      if (value > max)
        value = max;
    }

    ImGui::PopID();
    return changed;
  }

  // Optional tooltip display
  static void HelpMarker(const char *desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
      ImGui::BeginTooltip();
      ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
      ImGui::TextUnformatted(desc);
      ImGui::PopTextWrapPos();
      ImGui::EndTooltip();
    }
  }
};

} // namespace Fwg::UI
