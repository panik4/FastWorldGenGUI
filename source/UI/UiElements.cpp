#include "UI/UiElements.h"
namespace Fwg::UI {
bool Elements::BeginMainTabBar(const std::string &label) {
  // Emphasized color theme for main navigation
  ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.35f, 0.37f, 0.55f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.45f, 0.47f, 0.65f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.55f, 0.57f, 0.75f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 6));

  bool opened = ImGui::BeginTabBar(label.c_str(), ImGuiTabBarFlags_None);

  ImGui::PopStyleVar(1); // pop vars immediately so tab contents are normal
  ImGui::PopStyleColor(3);
  return opened;
}

void Elements::EndMainTabBar() {
  ImGui::EndTabBar();
}

bool Elements::BeginSubTabBar(const std::string &label, float height) {
  // Optional background for visual grouping
  if (height > 0.0f)
    ImGui::BeginChild((label + "_child").c_str(), ImVec2(0, height), true,
                      ImGuiWindowFlags_None);

  // Slightly different but still emphasized look
  ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.25f, 0.45f, 0.75f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.35f, 0.55f, 0.85f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.45f, 0.65f, 0.95f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 8));

  bool opened = ImGui::BeginTabBar(label.c_str(), ImGuiTabBarFlags_None);

  ImGui::PopStyleVar(1); // pop vars immediately so tab contents are normal
  ImGui::PopStyleColor(3);
  return opened;
}

void Elements::EndSubTabBar() { ImGui::EndTabBar(); }

bool Elements::BeginMainTabItem(const std::string &label,
                                const bool highlight) {
  // Colors for main navigation tabs
  // 61, 66, 99
  if (!highlight) {
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.25f, 0.27f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,
                          ImVec4(0.35f, 0.37f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabActive,
                          ImVec4(0.45f, 0.47f, 0.65f, 1.0f));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1.0f, 0.27f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,
                          ImVec4(1.0f, 0.37f, 0.55f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1.0f, 0.47f, 0.65f, 1.0f));
  }

  // Slightly larger padding to make them stand out
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 6));

  bool opened = ImGui::BeginTabItem(label.c_str());

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(3);

  return opened;
}

bool Elements::BeginSubTabItem(const std::string &label, const bool highlight) {
  // Colors for secondary tabs — still visible but less loud
  // 88, 38, 173
  if (!highlight) {
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.35f, 0.16f, 0.66f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,
                          ImVec4(0.4f, 0.22f, 0.76f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabActive,
                          ImVec4(0.45f, 0.28f, 0.86f, 1.0f));
  } else {
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(1.0f, 0.16f, 0.66f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabHovered,
                          ImVec4(1.0f, 0.22f, 0.76f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(1.0f, 0.28f, 0.86f, 1.0f));
  }

  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 8));

  bool opened = ImGui::BeginTabItem(label.c_str());

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(3);

  return opened;
}

} // namespace UI
