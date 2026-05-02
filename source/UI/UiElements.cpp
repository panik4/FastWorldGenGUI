#include "UI/UiElements.h"
namespace Fwg::UI {
namespace Elements {
bool BeginMainTabBar(const std::string &label) {
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

void EndMainTabBar() { ImGui::EndTabBar(); }

bool BeginSubTabBar(const std::string &label, float height) {
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

void EndSubTabBar() { ImGui::EndTabBar(); }

bool BeginMainTabItem(const std::string &label, const bool highlight) {
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

bool BeginSubTabItem(const std::string &label, const bool highlight) {
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

GridLayout::GridLayout(int columns, float labelWidth, float itemSpacing)
    : columns(columns), currentColumn(0), labelWidth(labelWidth),
      itemSpacing(itemSpacing), inputWidth(150.0f) {

  startPos = ImGui::GetCursorPos();

  // Auto-calculate label width if not provided
  if (labelWidth == 0.0f) {
    CalculateLabelWidth();
  }

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                      ImVec2(itemSpacing, itemSpacing));
}

GridLayout::~GridLayout() { ImGui::PopStyleVar(); }

void GridLayout::CalculateLabelWidth() {
  // Default to reasonable size
  labelWidth = 100.0f;
}

void GridLayout::BeginCell() {
  if (currentColumn > 0) {
    ImGui::SameLine();
  }
}

void GridLayout::EndCell() {
  // Draw cell background and border after the group
  ImVec2 rectMin = ImGui::GetItemRectMin();
  ImVec2 rectMax = ImGui::GetItemRectMax();

  // Add padding around the content
  const float padding = 4.0f;
  rectMin.x -= padding;
  rectMin.y -= padding;
  rectMax.x += padding;
  rectMax.y += padding;

  // Get base background color and brighten it
  ImVec4 bgColor = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
  bgColor.x = std::min(bgColor.x + 0.18f, 1.0f);
  bgColor.y = std::min(bgColor.y + 0.18f, 1.0f);
  bgColor.z = std::min(bgColor.z + 0.18f, 1.0f);
  bgColor.w = 0.2f; // Semi-transparent

  // Border color (slightly brighter than background)
  ImVec4 borderColor = bgColor;
  borderColor.x = std::min(borderColor.x + 0.15f, 1.0f);
  borderColor.y = std::min(borderColor.y + 0.15f, 1.0f);
  borderColor.z = std::min(borderColor.z + 0.15f, 1.0f);
  borderColor.w = 0.8f;

  // Draw background
  ImGui::GetWindowDrawList()->AddRectFilled(
      rectMin, rectMax, ImGui::ColorConvertFloat4ToU32(bgColor),
      3.0f // Rounded corners
  );

  // Draw border
  ImGui::GetWindowDrawList()->AddRect(
      rectMin, rectMax, ImGui::ColorConvertFloat4ToU32(borderColor),
      3.0f, // Rounded corners
      0,    // Flags
      1.0f  // Border thickness
  );

  currentColumn++;
  if (currentColumn >= columns) {
    currentColumn = 0;
  }
}

void GridLayout::NextRow() {
  if (currentColumn != 0) {
    // Force new row
    currentColumn = 0;
  }
}

void GridLayout::AddText(const char *label, const char *format, ...) {
  BeginCell();

  ImGui::BeginGroup();
  ImGui::PushItemWidth(labelWidth);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%-*s", (int)labelWidth / 7, label);
  ImGui::PopItemWidth();

  ImGui::SameLine();

  // Format the value text
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  // Reserve space by using an invisible button or dummy
  ImGui::PushItemWidth(inputWidth);
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(buffer);

  // Add dummy to ensure minimum width (invisible spacer)
  float textWidth = ImGui::CalcTextSize(buffer).x;
  if (textWidth < inputWidth) {
    ImGui::SameLine(0, 0);
    ImGui::Dummy(ImVec2(inputWidth - textWidth, 0));
  }
  ImGui::PopItemWidth();

  ImGui::EndGroup();
  EndCell();
}

bool GridLayout::AddInputInt(const char *label, int *value, int min, int max) {
  BeginCell();

  ImGui::BeginGroup();
  ImGui::PushItemWidth(labelWidth);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%-*s", (int)labelWidth / 7, label);
  ImGui::PopItemWidth();

  ImGui::SameLine();
  ImGui::PushItemWidth(inputWidth);

  std::string id = "##" + std::string(label);
  bool changed = ImGui::InputInt(id.c_str(), value);

  if (changed) {
    *value = std::clamp(*value, min, max);
  }

  ImGui::PopItemWidth();
  ImGui::EndGroup();

  EndCell();
  return changed;
}

bool GridLayout::AddInputFloat(const char *label, float *value, float min,
                               float max) {
  BeginCell();

  ImGui::BeginGroup();
  ImGui::PushItemWidth(labelWidth);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%-*s", (int)labelWidth / 7, label);
  ImGui::PopItemWidth();

  ImGui::SameLine();
  ImGui::PushItemWidth(inputWidth);

  std::string id = "##" + std::string(label);
  bool changed = ImGui::InputFloat(id.c_str(), value, 0.1f, 1.0f);

  if (changed) {
    *value = std::clamp(*value, min, max);
  }

  ImGui::PopItemWidth();
  ImGui::EndGroup();

  EndCell();
  return changed;
}

bool GridLayout::AddInputDouble(const char *label, double *value, double min,
                                double max) {
  BeginCell();

  ImGui::BeginGroup();
  ImGui::PushItemWidth(labelWidth);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%-*s", (int)labelWidth / 7, label);
  ImGui::PopItemWidth();

  ImGui::SameLine();
  ImGui::PushItemWidth(inputWidth);

  std::string id = "##" + std::string(label);
  bool changed = ImGui::InputDouble(id.c_str(), value, 0.01, 0.1);

  if (changed) {
    *value = std::clamp(*value, min, max);
  }

  ImGui::PopItemWidth();
  ImGui::EndGroup();

  EndCell();
  return changed;
}

bool GridLayout::AddSliderFloat(const char *label, float *value, float min,
                                float max) {
  BeginCell();

  ImGui::BeginGroup();
  ImGui::PushItemWidth(labelWidth);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%-*s", (int)labelWidth / 7, label);
  ImGui::PopItemWidth();

  ImGui::SameLine();
  ImGui::PushItemWidth(inputWidth);

  std::string id = "##" + std::string(label);
  bool changed = ImGui::SliderFloat(id.c_str(), value, min, max);

  ImGui::PopItemWidth();
  ImGui::EndGroup();

  EndCell();
  return changed;
}

bool GridLayout::AddSliderInt(const char *label, int *value, int min, int max) {
  BeginCell();

  ImGui::BeginGroup();
  ImGui::PushItemWidth(labelWidth);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%-*s", (int)labelWidth / 7, label);
  ImGui::PopItemWidth();

  ImGui::SameLine();
  ImGui::PushItemWidth(inputWidth);

  std::string id = "##" + std::string(label);
  bool changed = ImGui::SliderInt(id.c_str(), value, min, max);

  ImGui::PopItemWidth();
  ImGui::EndGroup();

  EndCell();
  return changed;
}
bool GridLayout::AddListBox(const char *label, int *current_item,
                            const char *const items[], int items_count,
                            int height_in_items, float listBoxWidth) {
  BeginCell();
  ImGui::PushID(label);
  bool changed = false;

  // Label (left-aligned)
  ImGui::AlignTextToFramePadding();
  ImGui::TextUnformatted(label);
  ImGui::SameLine(0, itemSpacing);

  // ListBox
  float boxWidth = listBoxWidth > 0.0f ? listBoxWidth : inputWidth * 2.0f;
  ImGui::SetNextItemWidth(boxWidth);
  changed = ImGui::ListBox("##listbox", current_item, items, items_count,
                           height_in_items);

  ImGui::PopID();
  EndCell();
  return changed;
}

} // namespace Elements
} // namespace Fwg::UI
