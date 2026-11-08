#include "UI/ClimateUI.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <utility>

namespace Fwg::UI::Climate {
namespace {
struct ExperimentalWindUiState {
  Fwg::Climate::Winds::ExperimentalWindSettings settings;
  int previewMode = 0; // 0: direction, 1: speed
  int installedMode = -1;
  bool generationPending = false;
  bool showPreview = false;
};

ExperimentalWindUiState &experimentalWindUiState() {
  static ExperimentalWindUiState state;
  return state;
}

bool windFieldMatches(const Fwg::Cfg &config,
                      const Fwg::FastWorldGenerator &generator,
                      const ExperimentalWindUiState &state) {
  return generator.experimentalAnnualWindField.matches(
      config.width, config.height, config.latitudeSouthDegrees,
      config.latitudeNorthDegrees, state.settings);
}

void installExperimentalWindPreview(Fwg::FastWorldGenerator &generator,
                                    UIContext &uiContext,
                                    ExperimentalWindUiState &state) {
  const auto &field = generator.experimentalAnnualWindField;
  auto image = state.previewMode == 0
                   ? Fwg::Gfx::Climate::displayWindDirection(field)
                   : Fwg::Gfx::Climate::displayWindSpeed(field);
  uiContext.imageContext.updateImage(0, std::move(image));
  state.installedMode = state.previewMode;
  const auto *fieldPointer = &field;
  uiContext.imageContext.pixelDescription = [fieldPointer](int x, int y) {
    if (!fieldPointer || x < 0 || y < 0 || x >= fieldPointer->width ||
        y >= fieldPointer->height)
      return std::string{};
    const auto pixel = static_cast<std::size_t>(y) * fieldPointer->width + x;
    const auto east = fieldPointer->eastwardMetersPerSecond[pixel];
    const auto north = fieldPointer->northwardMetersPerSecond[pixel];
    const auto speed = std::hypot(east, north);
    char description[192];
    if (speed < 0.01f) {
      std::snprintf(description, sizeof(description),
                    "Wind: calm (%.2f m/s)\nGrid: %d, %d", speed, x, y);
    } else {
      auto bearing = std::atan2(east, north) * 180.0 / std::acos(-1.0);
      if (bearing < 0.0)
        bearing += 360.0;
      std::snprintf(description, sizeof(description),
                    "Wind: %.2f m/s, bearing %.0f deg\nEast: %.2f m/s\nNorth: "
                    "%.2f m/s\nGrid: %d, %d",
                    speed, bearing, east, north, x, y);
    }
    return std::string(description);
  };
}

void showWindDirectionLegend() {
  struct Direction {
    const char *label;
    float east;
    float north;
  };
  static constexpr std::array<Direction, 8> directions{{{"E", 1.0f, 0.0f},
                                                        {"NE", 1.0f, 1.0f},
                                                        {"N", 0.0f, 1.0f},
                                                        {"NW", -1.0f, 1.0f},
                                                        {"W", -1.0f, 0.0f},
                                                        {"SW", -1.0f, -1.0f},
                                                        {"S", 0.0f, -1.0f},
                                                        {"SE", 1.0f, -1.0f}}};
  for (std::size_t i = 0; i < directions.size(); ++i) {
    const auto &direction = directions[i];
    const auto colour =
        Fwg::Gfx::Climate::windDirectionColour(direction.east, direction.north);
    ImGui::ColorButton(
        (std::string("##windDirection") + direction.label).c_str(),
        ImVec4(colour.getRed() / 255.0f, colour.getGreen() / 255.0f,
               colour.getBlue() / 255.0f, 1.0f),
        ImGuiColorEditFlags_NoTooltip, ImVec2(16.0f, 16.0f));
    ImGui::SameLine();
    ImGui::TextUnformatted(direction.label);
    if ((i + 1) % 4 != 0)
      ImGui::SameLine();
  }
  ImGui::Text("Direction hue: E red, N yellow-green, W cyan, S violet");
}

bool hasCurrentVisualizationDimensions(const Fwg::Cfg &config,
                                       const char *visualizationName) {
  if (Fwg::Gfx::isCurrentVisualizationDimensions(config.width, config.height,
                                                 config.processingArea))
    return true;
  Fwg::Utils::Logging::logLine("Skipping ", visualizationName,
                               " visualization: map dimensions are stale");
  return false;
}

void installHumidityPreview(const Fwg::Cfg &config,
                            const Fwg::FastWorldGenerator &generator,
                            UIContext &uiContext) {
  uiContext.imageContext.updateImage(
      0, hasCurrentVisualizationDimensions(config, "humidity")
             ? Fwg::Gfx::Climate::displayHumidity(
                   generator.climateData, config.climateGenerationContext())
             : Fwg::Gfx::Image());
}
} // namespace

namespace Detail {
// Helper: Render operation-specific parameters
void renderOperationParameters(Fwg::Climate::ClimateOperation &operation) {
  using namespace Fwg::Climate;

  switch (operation.type) {
  case ClimateOperationType::APPLY_ALTITUDE: {
    ImGui::TextWrapped(
        "Modifies climate based on altitude. Higher values exaggerate "
        "altitude effects on temperature.");
    float altitudeFactor =
        getParameter<float>(operation, "altitudeFactor", 0.5f);
    ImGui::Text("Altitude Factor");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##altitudeFactor", &altitudeFactor, 0.0f, 2.0f,
                           "%.2f")) {
      setParameter(operation, "altitudeFactor", altitudeFactor);
    }
    ImGui::Text("Rain Shadow");
    bool enableRainShadow =
        getParameter<bool>(operation, "enableRainShadow", true);
    if (ImGui::Checkbox("Enable Rain Shadow", &enableRainShadow)) {
      setParameter(operation, "enableRainShadow", enableRainShadow);
    }
    ImGui::Text("Latitude South");
    float latitudeSouth =
        getParameter<float>(operation, "latitudeSouth", -80.0f);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##latitudeSouth", &latitudeSouth, -90.0f, 0.0f,
                           "%.1f")) {
      setParameter(operation, "latitudeSouth", latitudeSouth);
    }
    ImGui::Text("Latitude North");
    float latitudeNorth =
        getParameter<float>(operation, "latitudeNorth", 80.0f);
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##latitudeNorth", &latitudeNorth, 0.0f, 90.0f,
                           "%.1f")) {
      setParameter(operation, "latitudeNorth", latitudeNorth);
    }

    break;
  }

  case ClimateOperationType::RANDOMIZE_TEMPERATURES: {
    ImGui::TextWrapped(
        "Adds temperature variation in Celsius. The factor sets the maximum "
        "offset as a fraction of the polar-to-equatorial reference range.");

    float frequency = getParameter<float>(operation, "frequency", 1.0f);
    ImGui::Text("Frequency");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##tempFrequency", &frequency, 0.1f, 5.0f, "%.2f")) {
      setParameter(operation, "frequency", frequency);
    }

    float randomizationFactor =
        getParameter<float>(operation, "randomizationFactor", 1.0f);
    ImGui::Text("Randomisation Factor");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##tempRandomisation", &randomizationFactor, 0.0f,
                           2.0f, "%.2f")) {
      setParameter(operation, "randomizationFactor", randomizationFactor);
    }
    break;
  }

  case ClimateOperationType::RANDOMIZE_HUMIDITIES: {
    ImGui::TextWrapped(
        "Randomizes humidity values to create more natural variation. "
        "Higher values produce more dramatic changes.");

    float frequency = getParameter<float>(operation, "frequency", 1.0f);
    ImGui::Text("Frequency");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##humFrequency", &frequency, 0.1f, 5.0f, "%.2f")) {
      setParameter(operation, "frequency", frequency);
    }

    float randomizationFactor =
        getParameter<float>(operation, "randomizationFactor", 1.0f);
    ImGui::Text("Randomisation Factor");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##humRandomisation", &randomizationFactor, 0.0f,
                           2.0f, "%.2f")) {
      setParameter(operation, "randomizationFactor", randomizationFactor);
    }
    break;
  }

  case ClimateOperationType::GAUSSIAN_BLUR: {
    ImGui::TextWrapped(
        "Smooths climate data using Gaussian blur. Larger radius and sigma "
        "values create more smoothing.");

    float radius = getParameter<float>(operation, "radius", 5.0f);
    ImGui::Text("Blur Radius");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##climateRadius", &radius, 1.0f, 20.0f, "%.1f")) {
      setParameter(operation, "radius", radius);
    }

    float sigma = getParameter<float>(operation, "sigma", 3.0f);
    ImGui::Text("Sigma (blur strength)");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##climateSigma", &sigma, 0.1f, 10.0f, "%.2f")) {
      setParameter(operation, "sigma", sigma);
    }
    break;
  }

  case ClimateOperationType::NORMALIZE: {
    ImGui::TextWrapped("Normalizes climate values to a specified range.");

    float minVal = getParameter<float>(operation, "min", 0.0f);
    ImGui::Text("Min Value");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputFloat("##climateMin", &minVal, 0.01f, 0.1f, "%.2f")) {
      setParameter(operation, "min", minVal);
    }

    float maxVal = getParameter<float>(operation, "max", 1.0f);
    ImGui::Text("Max Value");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputFloat("##climateMax", &maxVal, 0.01f, 0.1f, "%.2f")) {
      setParameter(operation, "max", maxVal);
    }
    break;
  }

  default:
    ImGui::TextWrapped("No parameters available for this operation type.");
    break;
  }
}

void configurePipelineEditor(Fwg::Cfg &cfg, UIContext &uiContext) {
  // Wrap entire editor in collapsible header
  bool isOpen = ImGui::CollapsingHeader("Climate Processing Pipeline");

  // Reserve space even when collapsed, or use fixed height when open
  const float fixedEditorHeight = 200.0f;

  if (!isOpen) {
    return;
  }

  auto &pipeline = cfg.climateConfig.pipeline;

  ImGui::Spacing();
  ImGui::TextWrapped("Configure the order and parameters of climate "
                     "processing operations. Drag to reorder.");
  ImGui::Spacing();

  // Main container - two columns with fixed height
  ImGui::BeginChild("ClimatePipelineEditorContainer",
                    ImVec2(0, fixedEditorHeight), false, ImGuiWindowFlags_None);

  // ===== LEFT COLUMN: Operation List =====
  ImGui::BeginChild("ClimateOperationList",
                    ImVec2(ImGui::GetContentRegionAvail().x * 0.45f, 0), true,
                    ImGuiWindowFlags_None);

  ImGui::Text("Climate Processing Pipeline");
  ImGui::Separator();

  // Drag-drop target for reordering
  static int draggedIndex = -1;

  for (int i = 0; i < pipeline.operations.size(); i++) {
    auto &operation = pipeline.operations[i];

    ImGui::PushID(i);

    // Make the entire row draggable
    ImGui::BeginGroup();

    // Checkbox for enable/disable
    ImGui::Checkbox("##enabled", &operation.enabled);
    ImGui::SameLine();

    // Drag handle indicator
    ImGui::TextDisabled(":::");
    ImGui::SameLine();

    // Selectable operation name
    bool isSelected =
        (uiContext.generationContext.selectedClimateOperationIndex == i);
    ImVec4 bgColor =
        isSelected ? ImVec4(0.26f, 0.59f, 0.98f, 0.40f) : ImVec4(0, 0, 0, 0);

    ImGui::PushStyleColor(ImGuiCol_Header, bgColor);
    if (ImGui::Selectable(operation.name.c_str(), isSelected,
                          ImGuiSelectableFlags_SpanAllColumns)) {
      uiContext.generationContext.selectedClimateOperationIndex = i;
    }
    ImGui::PopStyleColor();

    ImGui::EndGroup();

    // ===== DRAG AND DROP =====
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
      ImGui::SetDragDropPayload("CLIMATE_PIPELINE_OPERATION", &i, sizeof(int));
      ImGui::Text("Moving: %s", operation.name.c_str());
      draggedIndex = i;
      ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload("CLIMATE_PIPELINE_OPERATION")) {
        int sourceIndex = *(int *)payload->Data;

        // Reorder operations
        if (sourceIndex != i) {
          auto temp = pipeline.operations[sourceIndex];
          pipeline.operations.erase(pipeline.operations.begin() + sourceIndex);
          pipeline.operations.insert(pipeline.operations.begin() + i, temp);

          // Update selection
          uiContext.generationContext.selectedClimateOperationIndex = i;
        }
      }
      ImGui::EndDragDropTarget();
    }

    ImGui::PopID();
  }

  ImGui::Separator();
  ImGui::Spacing();

  // Add/Remove buttons
  if (ImGui::Button("+ Add Operation", ImVec2(-1, 0))) {
    ImGui::OpenPopup("AddClimateOperationPopup");
  }

  // ===== ADD OPERATION POPUP =====
  if (ImGui::BeginPopup("AddClimateOperationPopup")) {
    ImGui::Text("Add Climate Processing Operation");
    ImGui::Separator();

    struct OpTemplate {
      Fwg::Climate::ClimateOperationType type;
      const char *name;
      const char *desc;
    };

    static const OpTemplate templates[] = {
        {Fwg::Climate::ClimateOperationType::APPLY_ALTITUDE, "Apply Altitude",
         "Modify climate based on altitude"},
        {Fwg::Climate::ClimateOperationType::RANDOMIZE_TEMPERATURES,
         "Randomize Temperatures", "Add natural temperature variation"},
        {Fwg::Climate::ClimateOperationType::RANDOMIZE_HUMIDITIES,
         "Randomize Humidities", "Add natural humidity variation"},
        {Fwg::Climate::ClimateOperationType::GAUSSIAN_BLUR, "Gaussian Blur",
         "Smooth climate data"},
        {Fwg::Climate::ClimateOperationType::NORMALIZE, "Normalize",
         "Normalize values to range"}};

    for (const auto &t : templates) {
      if (ImGui::Selectable(t.name)) {
        Fwg::Climate::ClimateOperation newOp;
        newOp.type = t.type;
        newOp.name = t.name;
        newOp.enabled = true;

        // Use setParameter helper with correct types
        switch (t.type) {
        case Fwg::Climate::ClimateOperationType::APPLY_ALTITUDE:
          Fwg::Climate::setParameter(newOp, "altitudeFactor", 0.5f);
          break;
        case Fwg::Climate::ClimateOperationType::RANDOMIZE_TEMPERATURES:
          Fwg::Climate::setParameter(newOp, "frequency", 2.0f);
          Fwg::Climate::setParameter(newOp, "randomizationFactor", 0.15f);
          break;
        case Fwg::Climate::ClimateOperationType::RANDOMIZE_HUMIDITIES:
          Fwg::Climate::setParameter(newOp, "frequency", 2.0f);
          Fwg::Climate::setParameter(newOp, "randomizationFactor", 0.15f);
          break;
        case Fwg::Climate::ClimateOperationType::GAUSSIAN_BLUR:
          Fwg::Climate::setParameter(newOp, "radius", 5.0f);
          Fwg::Climate::setParameter(newOp, "sigma", 3.0f);
          break;
        case Fwg::Climate::ClimateOperationType::NORMALIZE:
          Fwg::Climate::setParameter(newOp, "min", 0.0f);
          Fwg::Climate::setParameter(newOp, "max", 1.0f);
          break;
        }

        pipeline.operations.push_back(newOp);
        uiContext.generationContext.selectedClimateOperationIndex =
            pipeline.operations.size() - 1;
        ImGui::CloseCurrentPopup();
      }

      if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", t.desc);
        ImGui::EndTooltip();
      }
    }

    ImGui::EndPopup();
  }
  // ===== END ADD OPERATION POPUP =====

  if (uiContext.generationContext.selectedClimateOperationIndex >= 0 &&
      uiContext.generationContext.selectedClimateOperationIndex <
          pipeline.operations.size()) {
    if (ImGui::Button("- Remove Selected", ImVec2(-1, 0))) {
      pipeline.operations.erase(
          pipeline.operations.begin() +
          uiContext.generationContext.selectedClimateOperationIndex);
      uiContext.generationContext.selectedClimateOperationIndex = std::max(
          0, uiContext.generationContext.selectedClimateOperationIndex - 1);
    }
  }

  ImGui::EndChild(); // ClimateOperationList

  ImGui::SameLine();

  // ===== RIGHT COLUMN: Parameter Editor =====
  ImGui::BeginChild("ClimateParameterEditor", ImVec2(0, 0), true,
                    ImGuiWindowFlags_None);

  if (uiContext.generationContext.selectedClimateOperationIndex >= 0 &&
      uiContext.generationContext.selectedClimateOperationIndex <
          pipeline.operations.size()) {

    auto &operation = pipeline.operations[uiContext.generationContext
                                              .selectedClimateOperationIndex];

    ImGui::Text("Operation: %s", operation.name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Editable name
    char nameBuf[128];
    strncpy(nameBuf, operation.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';

    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##climateopname", nameBuf, sizeof(nameBuf))) {
      operation.name = nameBuf;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Type-specific parameters
    renderOperationParameters(operation);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

  } else {
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                       "Select an operation to edit its parameters");
  }

  ImGui::EndChild(); // ClimateParameterEditor

  ImGui::EndChild(); // ClimatePipelineEditorContainer
}

} // namespace Detail

namespace Input {
bool RenderScrollableClimateInput(std::vector<Fwg::Gfx::Colour> &imageData,
                                  UIContext &uiContext) {
  // Use all available space, leaving room for buttons below (Apply all,
  // Analyze, and Value display)
  ImGui::BeginChild("ScrollingRegion",
                    ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 3.5f),
                    false, ImGuiWindowFlags_HorizontalScrollbar);

  // --- Selection state ---
  static bool singularEdit = false;
  static std::unordered_set<Fwg::Gfx::Colour> selectedInputs;
  static std::optional<Fwg::Gfx::Colour> lastClickedInput;

  bool updated = false;

  // --- Build sorted order for deterministic UX ---
  std::vector<Fwg::Gfx::Colour> colourOrder;
  for (auto &entry : uiContext.climateUI.climateInputColours.getMap()) {
    colourOrder.push_back(entry.second.in);
  }
  std::sort(colourOrder.begin(), colourOrder.end(), Fwg::Gfx::colourSort);

  // --- Global apply-to-selected buttons ---
  if (!selectedInputs.empty() && !singularEdit) {
    ImGui::Separator();
    ImGui::Text("Selected items: %zu", selectedInputs.size());
    ImGui::SameLine();

    if (ImGui::Button("Classify selected")) {
      ImGui::OpenPopup("ClimateClassificationPopup");
    }

    ImGui::SameLine();

    if (ImGui::Button("Apply type to all selected")) {
      for (const auto &selId : selectedInputs) {
        if (uiContext.climateUI.climateInputColours.getMap().contains(selId)) {
          auto &entry =
              uiContext.climateUI.climateInputColours.getMap().at(selId);
          for (auto &pix : entry.pixels) {
            imageData[pix] = entry.out;
          }
        }
      }
      uiContext.climateUI.highlightedInputs.clear();
      updated = true;
      selectedInputs.clear();
    }
  }

  // --- Iterate through sorted entries ---
  for (int i = 0; i < colourOrder.size(); ++i) {
    auto &id = colourOrder[i];
    auto &entry = uiContext.climateUI.climateInputColours.getMap().at(id);

    bool isSelected = selectedInputs.contains(entry.in);

    // Colour preview
    ImGui::ColorEdit3("##colourPreview", (float *)&entry.colour,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_HDR);

    ImGui::SameLine();
    ImGui::Text("%s", ("Currently labeled as: " + entry.rgbName).c_str());
    ImGui::SameLine();

    // --- Selection button ---
    if (isSelected) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
    }

    std::string btnLabel = "Select type for " + entry.in.toString();
    if (ImGui::Button(btnLabel.c_str())) {

      ImGuiIO &io = ImGui::GetIO();

      // SHIFT: range-select
      if (io.KeyShift && lastClickedInput.has_value()) {
        singularEdit = false;

        auto it1 = std::find(colourOrder.begin(), colourOrder.end(),
                             *lastClickedInput);
        auto it2 = std::find(colourOrder.begin(), colourOrder.end(), entry.in);
        if (it1 != colourOrder.end() && it2 != colourOrder.end()) {
          if (it1 > it2)
            std::swap(it1, it2);
          for (auto it = it1; it <= it2; ++it) {
            selectedInputs.insert(*it);
          }
        }
      }
      // CTRL: toggle
      else if (io.KeyCtrl) {
        singularEdit = false;

        if (selectedInputs.contains(entry.in))
          selectedInputs.erase(entry.in);
        else
          selectedInputs.insert(entry.in);

        lastClickedInput = entry.in;
        if (selectedInputs.size() <= 1)
          singularEdit = true;
      }
      // Normal click: single select
      else {
        singularEdit = true;
        selectedInputs.clear();
        selectedInputs.insert(entry.in);
        lastClickedInput = entry.in;

        ImGui::OpenPopup("ClimateClassificationPopup");
      }
    }

    if (isSelected)
      ImGui::PopStyleColor();

    ImGui::SameLine();

    // --- Highlighted entries ---
    bool isHighlighted =
        uiContext.climateUI.highlightedInputs.contains(entry.in);
    if (isHighlighted) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0.2f, 0.2f, 1));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0.4f, 0.4f, 1));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0.3f, 0.3f, 1));
    }

    // --- Apply single ---
    if (ImGui::Button(("Apply type for " + entry.in.toString()).c_str())) {
      for (auto &pix : entry.pixels)
        imageData[pix] = entry.out;

      uiContext.climateUI.highlightedInputs.erase(entry.in);
      updated = true;
    }

    if (isHighlighted)
      ImGui::PopStyleColor(3);
  }

  // --- Classification Popup ---
  if (ImGui::BeginPopup("ClimateClassificationPopup")) {
    ImGui::SeparatorText("Classify selected climate input colours");

    for (auto &internalType :
         uiContext.climateUI.allowedClimateInputs.getMap()) {
      if (ImGui::Button(internalType.second.name.c_str())) {
        for (const auto &selId : selectedInputs) {
          auto &entry =
              uiContext.climateUI.climateInputColours.getMap().at(selId);
          entry.out = internalType.second.primaryColour;
          uiContext.climateUI.highlightedInputs.insert(selId);
        }
        selectedInputs.clear();
        ImGui::CloseCurrentPopup();
      }
    }
    ImGui::EndPopup();
  }

  ImGui::EndChild();
  return updated;
}

bool analyzeClimateMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                       const Fwg::Gfx::Image &climateInput,
                       UIContext &uiContext) {
  uiContext.climateUI.climateInputColours.clear();
  uiContext.climateUI.amountClassificationsNeeded = 0;
  ImGui::SameLine();
  // reset count every time we press the button
  for (auto &elem : uiContext.climateUI.climateInputColours.getMap()) {
    elem.second.pixels.clear();
  }
  static bool classificationNeeded = false;
  auto imageIndex = 0;
  // init and count colours
  for (auto &colour : climateInput.imageData) {
    // first count classifications
    if (uiContext.climateUI.climateInputColours.contains(colour) &&
        uiContext.climateUI.climateInputColours[colour].pixels.size() == 0 &&
        !uiContext.climateUI.allowedClimateInputs.contains(colour)) {
      uiContext.climateUI.amountClassificationsNeeded++;
    }

    ImVec4 inputColourVisualHelp = ImVec4(
        ((float)colour.getRed()) / 255.0f, ((float)colour.getGreen()) / 255.0f,
        ((float)colour.getBlue()) / 255.0f, 1.0f);
    // check if the input is of permitted colours or first needs to be
    // classified
    if (!uiContext.climateUI.allowedClimateInputs.contains(colour)) {
      // this input colour is not classified yet
      classificationNeeded = true;
      // if we haven't set this colour yet
      if (!uiContext.climateUI.climateInputColours.contains(colour)) {
        // set it
        uiContext.climateUI.climateInputColours.setValue(
            colour, ClimateInput{colour, colour, "Unclassified",
                                 uiContext.climateUI.allowedClimateInputs.at(
                                     cfg.climateConfig.climateColours.at(
                                         "continentalhot")),
                                 inputColourVisualHelp});
        uiContext.climateUI.climateInputColours[colour].pixels.push_back(
            imageIndex);
        uiContext.climateUI.amountClassificationsNeeded++;
      } else {
        // otherwise increment how often this colour is here
        uiContext.climateUI.climateInputColours[colour].pixels.push_back(
            imageIndex);
      }
    }
    // this is a known and permitted colour, for which we can already create a
    // detailed climateInput
    else {
      // if we haven't set this colour yet
      if (!uiContext.climateUI.climateInputColours.contains(colour)) {
        // set it with its valid output colour
        uiContext.climateUI.climateInputColours.setValue(
            colour,
            ClimateInput{
                colour, colour,
                uiContext.climateUI.allowedClimateInputs.at(colour).name,
                uiContext.climateUI.allowedClimateInputs.at(colour),
                inputColourVisualHelp});
        uiContext.climateUI.climateInputColours[colour].pixels.push_back(
            imageIndex);
      } else {
        // otherwise increment how often this colour is here
        uiContext.climateUI.climateInputColours[colour].pixels.push_back(
            imageIndex);
      }
    }
    imageIndex++;
  }
  return !classificationNeeded;
}

bool climateMapping(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext) {
  bool updated = false;

  // Update names of entries based on classification
  for (auto &cl : uiContext.climateUI.climateInputColours.getMap()) {
    if (cl.second.pixels.size()) {
      cl.second.rgbName =
          (uiContext.climateUI.allowedClimateInputs.contains(cl.second.out)
               ? uiContext.climateUI.allowedClimateInputs[cl.second.out].name
               : "Unclassified");
    }
  }

  updated |= RenderScrollableClimateInput(
      uiContext.climateUI.climateInputMap.imageData, uiContext);

  // "Apply all" option
  if (!uiContext.climateUI.highlightedInputs.empty()) {
    ImGui::Text("Before next analysis, apply all types");
    if (ImGui::Button("Apply all")) {
      for (auto &input : uiContext.climateUI.climateInputColours.getMap()) {
        if (uiContext.climateUI.highlightedInputs.contains(input.second.in)) {
          for (auto &pix : input.second.pixels) {
            uiContext.climateUI.climateInputMap.setColourAtIndex(
                pix, input.second.out);
          }
          uiContext.climateUI.highlightedInputs.erase(input.second.in);
        }
      }
      updated = true;
    }
  }
  // Re-analyze
  else if (ImGui::Button("Analyze Input") || uiContext.climateUI.analyze) {
    analyzeClimateMap(cfg, fwg, uiContext.climateUI.climateInputMap, uiContext);
    uiContext.climateUI.analyze = false;
  }

  ImGui::Value("Colours needing classification: ",
               uiContext.climateUI.amountClassificationsNeeded);
  return updated;
}

int showClimateInputTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                        UIContext &uiContext) {

  if (UI::Elements::BeginMainTabItem("Climate Input")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(0,
                                         uiContext.climateUI.climateInputMap);
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
      if (fwg.worldMap.initialised()) {
        uiContext.imageContext.updateImage(1, fwg.worldMap);
      }
    }
    ImGui::SeparatorText("This step is OPTIONAL! You can also generate a "
                         "random climate in the next tab");
    uiContext.helpContext.showHelpTextBox("Climate Input");
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData)});

    if (guard.ready()) {
      if (uiContext.triggeredDrag) {
        uiContext.triggeredDrag = false;
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg, &uiContext]() {
              // don't immediately generate from the input, instead allow to
              // manually classify all present colours
              uiContext.climateUI.climateInputMap =
                  Fwg::IO::Reader::readGenericImage(uiContext.draggedFile,
                                                    cfg.imageReadContext());
              if (!uiContext.climateUI.climateInputMap.size())
                return false;
              // create a map from secondary colours to primary colours
              Fwg::Utils::ColourTMap<Fwg::Climate::ClimateClassDefinition>
                  secondaryToPrimary;
              for (auto &type : cfg.climateConfig.climateClassDefinitions) {
                for (auto &secondary : type.secondaryColours) {
                  secondaryToPrimary.setValue(secondary, type);
                }
              }

              // preprocess input to convert to primary colours where possible
              for (auto &col : uiContext.climateUI.climateInputMap.imageData) {
                if (secondaryToPrimary.contains(col)) {
                  col = secondaryToPrimary[col].primaryColour;
                }
              }

              if (uiContext.climateUI.climateInputMap.size() !=
                  fwg.terrainData.detailedHeightMap.size()) {
                Fwg::Utils::Logging::logLine(
                    "Climate input map size does not match height "
                    "map size. Please ensure that the input map is "
                    "the same size as the height map");
                uiContext.climateUI.climateInputMap.clear();
                return false;
              }
              return true;
            },
            [&uiContext](TaskResult::Outcome outcome) {
              if (outcome == TaskResult::Outcome::Success)
                uiContext.climateUI.analyze = true;
            });
      }
      if (uiContext.climateUI.climateInputMap.initialised()) {
        // Calculate available space: reserve space for pipeline editor if it
        // will be shown The collapsible header takes ~20px, the pipeline editor
        // takes 200px when open We need to check if we're past the
        // prerequisites to show the editor
        const float pipelineHeaderHeight = 25.0f;
        const float pipelineEditorHeight = 200.0f;

        // Reserve space for pipeline editor (will be shown below)
        float availableHeight = ImGui::GetContentRegionAvail().y -
                                pipelineHeaderHeight - pipelineEditorHeight -
                                50.0f;

        // Ensure minimum height
        if (availableHeight < 100.0f) {
          availableHeight = 100.0f;
        }

        // Wrap climate mapping in a child window with calculated height
        ImGui::BeginChild("ClimateMappingSection", ImVec2(0, availableHeight),
                          false, ImGuiWindowFlags_None);
        if (Fwg::UI::Climate::Input::climateMapping(cfg, fwg, uiContext)) {
          uiContext.imageContext.resetTexture();
        }
        ImGui::EndChild(); // ClimateMappingSection
      }
    }

    Detail::configurePipelineEditor(cfg, uiContext);

    // Classification guard
    auto classificationGuard =
        UI::PrerequisiteChecker::requireSilent({UI::Prerequisite{
            "Classification", "Complete climate classification", [&]() {
              return uiContext.climateUI.amountClassificationsNeeded <= 0 &&
                     !uiContext.climateUI.analyze;
            }}});

    if (classificationGuard.ready() &&
        uiContext.climateUI.climateInputMap.size() &&
        ImGui::Button("Generate from labeled climate")) {
      uiContext.asyncContext.runAsync(
          [&fwg, &cfg, &uiContext]() {
            Fwg::Gfx::Png::save(uiContext.climateUI.climateInputMap,
                                cfg.mapsPath + "classifiedClimateInput.png");
            return fwg.loadClimate(cfg, uiContext.climateUI.climateInputMap);
          },
          [&cfg](TaskResult::Outcome outcome) { (void)outcome; });
    }

    ImGui::EndTabItem();
  }
  return 0;
}

} // namespace Input

int showTemperatureMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                       UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Temperature")) {
    static int previewSelection = 0;
    static const char *previewItems[] = {
        "Annual",  "January",  "February", "March",  "April",
        "May",     "June",     "July",     "August", "September",
        "October", "November", "December"};
    const auto refreshPreview = [&]() {
      uiContext.imageContext.updateImage(
          0, hasCurrentVisualizationDimensions(cfg, "temperature")
                 ? Fwg::Gfx::Climate::displayTemperature(
                       fwg.climateData, cfg.climateConfig,
                       cfg.climateGenerationContext(), previewSelection - 1)
                 : Fwg::Gfx::Image());
    };
    if (uiContext.tabSwitchEvent()) {
      refreshPreview();
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
    }
    uiContext.helpContext.showHelpTextBox("Temperature");

    if (ImGui::Combo("Preview", &previewSelection, previewItems, 13))
      refreshPreview();

    bool settingsChanged = false;
    {
      UI::Elements::GridLayout temperatureSettings(2);
      settingsChanged |= temperatureSettings.AddInputDouble(
          "Maximum seasonal amplitude (°C)",
          &cfg.climateConfig.maximumSeasonalAmplitudeCelsius, 0.0, 80.0);
      settingsChanged |= temperatureSettings.AddInputDouble(
          "Elevation cooling (°C/m)",
          &cfg.climateConfig.elevationCoolingCelsiusPerMeter, 0.0, 0.1);
    }
    if (settingsChanged)
      refreshPreview();

    ImGui::SeparatorText("Temperature Map Generation");

    static bool applyAltitudeEffect = false;
    ImGui::Checkbox("Apply elevation effect when loading",
                    &applyAltitudeEffect);

    ImGui::Spacing();

    {
      auto guard = UI::PrerequisiteChecker::require(
          {UI::PrerequisiteChecker::climateOperation(
              fwg, cfg, ClimateOperation::Temperature)});
      if (!cfg.climateConfig.fantasyClimate &&
          UI::Elements::ImportantStepButton("Generate Temperature Map",
                                            ImVec2(280, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genTemperatures(cfg); },
            [refreshPreview](TaskResult::Outcome) { refreshPreview(); });
      }
      if (cfg.climateConfig.fantasyClimate &&
          UI::Elements::ImportantStepButton(
              "Generate Temperature Map (new seed)", ImVec2(280, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() {
              cfg.randomSeed = true;
              cfg.reRandomize();
              return fwg.genTemperatures(cfg);
            },
            [refreshPreview](TaskResult::Outcome) { refreshPreview(); });
      }

      if (!fwg.climateData.temperatures.annualMeanCelsius.empty() &&
          UI::Elements::ImportantStepButton("Rebuild Seasonal Metadata",
                                            ImVec2(280, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.rebuildTemperatureSeasonality(cfg); },
            [refreshPreview](TaskResult::Outcome) { refreshPreview(); });
      }

      if (uiContext.triggeredDrag) {
        fwg.importClimateImage(cfg, ClimateProduct::Temperature,
                               uiContext.draggedFile, applyAltitudeEffect);
        uiContext.triggeredDrag = false;
        refreshPreview();
        uiContext.imageContext.resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int showHumidityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext) {
  auto &windUi = experimentalWindUiState();
  if (UI::Elements::BeginSubTabItem("Humidity")) {
    if (uiContext.tabSwitchEvent()) {
      if (windUi.showPreview && windFieldMatches(cfg, fwg, windUi)) {
        installExperimentalWindPreview(fwg, uiContext, windUi);
      } else {
        installHumidityPreview(cfg, fwg, uiContext);
        windUi.installedMode = -1;
      }
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
    }
    uiContext.helpContext.showHelpTextBox("Humidity");

    ImGui::SeparatorText("Humidity Map Generation");

    static bool applyElevationEffect = false;
    ImGui::Checkbox("Apply elevation effect when loading",
                    &applyElevationEffect);

    ImGui::Spacing();

    {
      auto guard = UI::PrerequisiteChecker::require(
          {UI::PrerequisiteChecker::climateOperation(
              fwg, cfg, ClimateOperation::Humidity)});
      if (!cfg.climateConfig.fantasyClimate &&
          UI::Elements::ImportantStepButton("Generate Humidity Map",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genHumidity(cfg); });
      }
      if (cfg.climateConfig.fantasyClimate &&
          UI::Elements::ImportantStepButton("Generate Humidity Map (new seed)",
                                            ImVec2(280, 0))) {
        uiContext.asyncContext.runAsync([&fwg, &cfg]() {
          cfg.randomSeed = true;
          cfg.reRandomize();
          return fwg.genHumidity(cfg);
        });
      }

      if (uiContext.triggeredDrag) {
        fwg.importClimateImage(cfg, ClimateProduct::Humidity,
                               uiContext.draggedFile, applyElevationEffect);
        uiContext.triggeredDrag = false;
        uiContext.imageContext.resetTexture();
      }
    }

    ImGui::SeparatorText("Experimental Annual Winds");
    ImGui::TextWrapped(
        "Full-resolution prescribed wind belts for the precipitation model "
        "experiment. These winds do not affect existing generation.");
    const auto area = static_cast<std::int64_t>(cfg.width) * cfg.height;
    const auto windMiB = area > 0 ? static_cast<double>(area) * 2.0 *
                                        sizeof(float) / (1024.0 * 1024.0)
                                  : 0.0;
    const auto previewCpuMiB = area > 0 ? static_cast<double>(area) *
                                              sizeof(Fwg::Gfx::Colour) /
                                              (1024.0 * 1024.0)
                                        : 0.0;
    const auto previewGpuMiB =
        area > 0 ? static_cast<double>(area) * 4.0 / (1024.0 * 1024.0) : 0.0;
    ImGui::Text("Wind grid: %d x %d; components %.1f MiB; preview about %.1f "
                "MiB CPU + %.1f MiB GPU",
                cfg.width, cfg.height, windMiB, previewCpuMiB, previewGpuMiB);

    const double multiplierMin = 0.0;
    const double multiplierMax = 3.0;
    ImGui::SliderScalar("Zonal wind multiplier", ImGuiDataType_Double,
                        &windUi.settings.zonalMultiplier, &multiplierMin,
                        &multiplierMax, "%.2f");
    ImGui::SliderScalar("Meridional wind multiplier", ImGuiDataType_Double,
                        &windUi.settings.meridionalMultiplier, &multiplierMin,
                        &multiplierMax, "%.2f");

    const bool hasPositiveDimensions = cfg.width > 0 && cfg.height > 0;
    const bool windAreaAllowed =
        area > 0 && cfg.resolutionConstraints.maxImageArea > 0 &&
        area <= cfg.resolutionConstraints.maxImageArea &&
        area <= Fwg::Climate::Winds::maximumExperimentalWindPixels;
    if (!hasPositiveDimensions)
      ImGui::TextDisabled("Configured map dimensions are invalid.");
    else if (!windAreaAllowed)
      ImGui::TextDisabled(
          "This grid exceeds the configured wind memory limit.");

    const bool currentWind =
        !windUi.generationPending && windFieldMatches(cfg, fwg, windUi);
    ImGui::BeginDisabled(!windAreaAllowed || uiContext.asyncContext.busy());
    const bool generateWind =
        ImGui::Button("Generate Full-Resolution Annual Winds");
    ImGui::EndDisabled();
    if (generateWind) {
      if (!cfg.validateResolution(cfg.width, cfg.height)) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                           "Map resolution does not meet configured limits.");
      } else {
        const auto width = cfg.width;
        const auto height = cfg.height;
        const auto latitudeSouth = cfg.latitudeSouthDegrees;
        const auto latitudeNorth = cfg.latitudeNorthDegrees;
        const auto settings = windUi.settings;
        windUi.showPreview = false;
        windUi.installedMode = -1;
        windUi.generationPending = true;
        uiContext.imageContext.pixelDescription = {};
        uiContext.imageContext.updateImage(0, Fwg::Gfx::Image());
        const bool queued = uiContext.asyncContext.runAsync(
            [&fwg, width, height, latitudeSouth, latitudeNorth, settings]() {
              return fwg.genExperimentalAnnualWinds(
                  width, height, latitudeSouth, latitudeNorth, settings);
            },
            [&fwg, &cfg, &uiContext, &windUi](TaskResult::Outcome outcome) {
              windUi.generationPending = false;
              if (outcome == TaskResult::Outcome::Success &&
                  windFieldMatches(cfg, fwg, windUi)) {
                windUi.showPreview = true;
                installExperimentalWindPreview(fwg, uiContext, windUi);
              } else {
                windUi.showPreview = false;
                windUi.installedMode = -1;
                installHumidityPreview(cfg, fwg, uiContext);
              }
            });
        if (!queued) {
          windUi.generationPending = false;
          installHumidityPreview(cfg, fwg, uiContext);
        }
      }
    }
    if (uiContext.asyncContext.busy()) {
      ImGui::SameLine();
      ImGui::TextDisabled("Generating...");
    }

    if (windUi.showPreview && !currentWind && windUi.installedMode != -1) {
      uiContext.imageContext.updateImage(0, Fwg::Gfx::Image());
      windUi.installedMode = -1;
    }
    if (windUi.showPreview && !currentWind)
      ImGui::TextColored(
          ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
          "Wind settings or map geometry changed. Regenerate to view.");

    if (!windUi.generationPending && windFieldMatches(cfg, fwg, windUi)) {
      ImGui::SameLine();
      const auto previewButton =
          windUi.showPreview ? "Hide Wind Preview" : "Show Wind Preview";
      if (ImGui::Button(previewButton)) {
        windUi.showPreview = !windUi.showPreview;
        if (windUi.showPreview)
          installExperimentalWindPreview(fwg, uiContext, windUi);
        else {
          installHumidityPreview(cfg, fwg, uiContext);
          windUi.installedMode = -1;
        }
      }

      const char *previewModes[] = {"Direction (cyclic colour)", "Speed"};
      if (ImGui::Combo("Wind preview", &windUi.previewMode, previewModes,
                       IM_ARRAYSIZE(previewModes))) {
        if (windUi.showPreview)
          installExperimentalWindPreview(fwg, uiContext, windUi);
      }
      if (windUi.previewMode == 0)
        showWindDirectionLegend();
      else
        ImGui::Text("Speed scale: 0-12 m/s (blue to red), calm is grey");

      if (windUi.showPreview) {
        ImGui::Text(
            "Hover over the map preview for component values and bearing.");
        ImGui::BeginDisabled(cfg.mapsPath.empty());
        if (ImGui::Button("Export Selected Preview")) {
          const auto directory =
              std::filesystem::path(cfg.mapsPath) / "climate" / "experimental";
          std::filesystem::create_directories(directory);
          const auto filename = windUi.previewMode == 0
                                    ? "annualWindDirection.png"
                                    : "annualWindSpeed.png";
          const auto path = (directory / filename).string();
          auto image = std::move(uiContext.imageContext.activeImages[0]);
          Fwg::Gfx::Png::save(std::move(image), path, false, LCT_RGBA, 8, 255,
                              cfg.username);
          installExperimentalWindPreview(fwg, uiContext, windUi);
        }
        ImGui::EndDisabled();
        if (cfg.mapsPath.empty())
          ImGui::TextDisabled("Set a maps output path to export the preview.");
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int showRiverTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                 UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Rivers")) {
    if (uiContext.tabSwitchEvent()) {
      const auto states = fwg.generationStateSnapshot();
      const bool riversReady =
          states.at(static_cast<std::size_t>(ClimateProduct::Rivers)).status ==
          GenerationStateTracker::Status::Ready;
      uiContext.imageContext.updateImage(
          0, riversReady && hasCurrentVisualizationDimensions(cfg, "river")
                 ? Gfx::riverMap(fwg.terrainData.detailedHeightMap,
                                 fwg.climateData.rivers,
                                 cfg.visualizationContext())
                 : Fwg::Gfx::Image());
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
    }
    uiContext.helpContext.showHelpTextBox("Rivers");

    ImGui::SeparatorText("River Configuration");
    ImGui::Checkbox("Treat dropped image as exact-blue river mask",
                    &uiContext.blueMaskRiverImport);

    {
      UI::Elements::GridLayout grid(2, uiContext.scaled(200.0f),
                                    uiContext.scaled(12.0f));
      grid.AddText("River Count", "%d", (int)fwg.climateData.rivers.size());
    }

    ImGui::Spacing();
    {
      auto guard = UI::PrerequisiteChecker::require(
          {UI::PrerequisiteChecker::climateOperation(
              fwg, cfg, ClimateOperation::Rivers)});
      if (UI::Elements::ImportantStepButton("Generate River Map",
                                            ImVec2(200, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genRivers(cfg); });
      }
      if (UI::Elements::ImportantStepButton("Generate River Map (new seed)",
                                            ImVec2(280, 0))) {
        uiContext.asyncContext.runAsync([&fwg, &cfg]() {
          cfg.randomSeed = true;
          cfg.reRandomize();
          return fwg.genRivers(cfg);
        });
      }

      if (uiContext.triggeredDrag) {
        fwg.importClimateImage(cfg, ClimateProduct::Rivers,
                               uiContext.draggedFile, false,
                               uiContext.blueMaskRiverImport);
        uiContext.imageContext.resetTexture();
        uiContext.triggeredDrag = false;
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
int showClimateZonesTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                        UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Climate Zones")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, hasCurrentVisualizationDimensions(cfg, "climate")
                 ? Fwg::Gfx::Climate::displayClimate(
                       fwg.climateData, false, cfg.climateConfig,
                       cfg.climateGenerationContext())
                 : Fwg::Gfx::Image());
      uiContext.imageContext.updateImage(1, fwg.worldMap);
    }

    uiContext.helpContext.showHelpTextBox("Climate Gen");
    ImGui::SeparatorText("Generate climate map");

    {
      auto guard = UI::PrerequisiteChecker::require(
          {UI::PrerequisiteChecker::climateOperation(
              fwg, cfg, ClimateOperation::Classification)});
      if (ImGui::Button(
              "Generate Climate Zones from Temperature and Heightmap Data")) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genClimate(cfg); });
      } else if (cfg.climateConfig.fantasyClimate &&
                 ImGui::Button("Generate completely random fantasy climate")) {
        uiContext.asyncContext.runAsync([&fwg, &cfg]() {
          cfg.randomSeed = true;
          cfg.reRandomize();
          if (!fwg.genTemperatures(cfg) || !fwg.genHumidity(cfg))
            return false;
          return fwg.genClimate(cfg);
        });
      }

      if (uiContext.triggeredDrag) {
        Fwg::Utils::Logging::logLine(
            "Climate map loading is done in the Climate Input Tab");
        uiContext.triggeredDrag = false;
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int showTreeTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Forests")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, hasCurrentVisualizationDimensions(cfg, "climate")
                 ? Fwg::Gfx::Climate::displayClimate(
                       fwg.climateData, true, cfg.climateConfig,
                       cfg.climateGenerationContext())
                 : Fwg::Gfx::Image());
      uiContext.imageContext.updateImage(
          1, hasCurrentVisualizationDimensions(cfg, "tree density")
                 ? Fwg::Gfx::Climate::displayTreeDensity(
                       fwg.climateData, cfg.climateGenerationContext())
                 : Fwg::Gfx::Image());
    }
    uiContext.helpContext.showHelpTextBox("Forests");

    ImGui::SeparatorText("Forest Density Configuration");

    {
      UI::Elements::GridLayout grid(2, uiContext.scaled(200.0f),
                                    uiContext.scaled(12.0f));

      grid.AddInputDouble("Boreal Density", &cfg.climateConfig.borealDensity,
                          0.0, 1.0);
      grid.AddInputDouble("Temperate Needle",
                          &cfg.climateConfig.temperateNeedleDensity, 0.0, 1.0);
      grid.AddInputDouble("Temperate Mixed",
                          &cfg.climateConfig.temperateMixedDensity, 0.0, 1.0);
      grid.AddInputDouble("Sparse Density", &cfg.climateConfig.sparseDensity,
                          0.0, 1.0);
      grid.AddInputDouble("Tropical Dry", &cfg.climateConfig.tropicalDryDensity,
                          0.0, 1.0);
      grid.AddInputDouble("Tropical Moist",
                          &cfg.climateConfig.tropicalMoistDensity, 0.0, 1.0);
    }

    ImGui::Spacing();
    {
      auto guard = UI::PrerequisiteChecker::require(
          {UI::PrerequisiteChecker::climateOperation(
              fwg, cfg, ClimateOperation::Forests)});
      if (UI::Elements::ImportantStepButton("Generate Forest Map",
                                            ImVec2(200, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genForests(cfg); });
      }

      if (uiContext.triggeredDrag) {
        fwg.importClimateImage(cfg, ClimateProduct::Forests,
                               uiContext.draggedFile);
        uiContext.triggeredDrag = false;
        uiContext.imageContext.resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int showWastelandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Wasteland")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(0, fwg.worldMap);
      uiContext.imageContext.updateImage(1, fwg.worldMap);
    }
    uiContext.helpContext.showHelpTextBox("Wasteland");
    ImGui::SeparatorText("Generate wasteland map or drop it in");
    if (ImGui::Button("Generate Wasteland Map")) {
      uiContext.asyncContext.runAsync([&fwg, &cfg]() {
        // fwg.genWasteland(cfg);
        return TaskResult{TaskResult::Outcome::Failure,
                          "Wasteland generation is not implemented"};
      });
    }

    if (uiContext.triggeredDrag) {
      // fwg.loadWasteland(cfg, uiContext.draggedFile);
      uiContext.triggeredDrag = false;
      uiContext.imageContext.resetTexture();
    }

    ImGui::EndTabItem();
  }
  return 0;
}

} // namespace Fwg::UI::Climate
