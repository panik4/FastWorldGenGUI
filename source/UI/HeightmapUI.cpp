#include "UI/HeightmapUI.h"

namespace Fwg::UI {
void HeightmapUI::configureLandElevationFactors(Fwg::Cfg &cfg,
                                                Fwg::FastWorldGenerator &fwg) {
  if (cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM) {
    ImGui::SeparatorText("Land Elevation Factors");
    const float labelWidth = 10.0f;
    const float inputWidth = 100.0f;
    const int itemsPerRow = 3;
    int counter = 0;

    auto addFactor = [&](const std::string &label, float &value) {
      ImGui::BeginGroup();
      ImGui::Text("%-*s", (int)labelWidth, label);
      ImGui::SameLine();
      ImGui::SetNextItemWidth(inputWidth);
      ImGui::InputFloat((std::string("##") + label).c_str(), &value, 0.01f,
                        0.1f);
      ImGui::EndGroup();

      counter++;
      if (counter % itemsPerRow != 0)
        ImGui::SameLine();
    };

    // Iterate through landforms in display order
    for (auto &landFormDefinition : cfg.terrainConfig.landformDefinitions) {
      addFactor(landFormDefinition.name, landFormDefinition.landformFactor);
    }
  }
}

// Helper: Render operation-specific parameters
void HeightmapUI::renderOperationParameters(
    Fwg::Terrain::HeightmapOperation &operation) {
  using namespace Fwg::Terrain;

  switch (operation.type) {
  case HeightmapOperationType::APPLY_BASE_ALTITUDE: {
    ImGui::TextWrapped(
        "Modifies terrain based on input landform base altitude. Higher values "
        "exaggerate differences.");
    // Use helper functions with float type
    float baseAltitudeEffects =
        getParameter<float>(operation, "baseAltitudeEffects", 0.5f);
    ImGui::Text("Base Altitude Effect Strength");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##baseAltitudeEffects", &baseAltitudeEffects, 0.0f,
                           2.0f, "%.2f")) {
      setParameter(operation, "baseAltitudeEffects", baseAltitudeEffects);
    }
    float blurFactor = getParameter<float>(operation, "blurFactor", 1.0f);
    ImGui::Text("Blur Factor");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##blurFactor", &blurFactor, 0.0f, 5.0f, "%.1f")) {
      setParameter(operation, "blurFactor", blurFactor);
    }
    break;
  }
  case HeightmapOperationType::APPLY_LAND_LAYERS: {
    ImGui::TextWrapped(
        "Applies land and sea detail layers based on distance weights.");

    // Use helper functions with bool type
    bool useWeights = getParameter<bool>(operation, "useWeights", true);
    if (ImGui::Checkbox("Use Distance Weights", &useWeights)) {
      setParameter(operation, "useWeights", useWeights);
    }
    break;
  }
  case HeightmapOperationType::RANDOMIZE_WEIGHTS: {
    ImGui::TextWrapped(
        "Randomizes noise weights to create more natural variation. Higher "
        "values produce more dramatic changes.");
    // Use helper functions with float type
    float randomisationFactor =
        getParameter<float>(operation, "randomisationFactor", 0.5f);
    ImGui::Text("Randomisation Factor");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##randomisationFactor", &randomisationFactor, 0.0f,
                           1.0f, "%.2f")) {
      setParameter(operation, "randomisationFactor", randomisationFactor);
    }
    break;
  }

  case HeightmapOperationType::GAUSSIAN_BLUR_WEIGHTS: {
    // Use helper functions with float type
    float radius = getParameter<float>(operation, "radius", 2.0f);
    ImGui::Text("Blur Radius");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##radius", &radius, 1.0f, 10.0f, "%.1f")) {
      setParameter(operation, "radius", radius);
    }

    float sigma = getParameter<float>(operation, "sigma", 1.0f);
    ImGui::Text("Sigma (blur strength)");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##sigma", &sigma, 0.1f, 5.0f, "%.2f")) {
      setParameter(operation, "sigma", sigma);
    }
    break;
  }
  case HeightmapOperationType::GAUSSIAN_BLUR: {
    // Use helper functions with float type
    float radius = getParameter<float>(operation, "radius", 2.0f);
    ImGui::Text("Blur Radius");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##radius", &radius, 1.0f, 10.0f, "%.1f")) {
      setParameter(operation, "radius", radius);
    }

    float sigma = getParameter<float>(operation, "sigma", 1.0f);
    ImGui::Text("Sigma (blur strength)");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##sigma", &sigma, 0.1f, 5.0f, "%.2f")) {
      setParameter(operation, "sigma", sigma);
    }
    break;
  }

  case HeightmapOperationType::GULLY_EROSION: {
    // Use int type for iterations
    int iterations = getParameter<int>(operation, "iterations", 1);
    ImGui::Text("Iterations");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##iterations", &iterations, 1, 10)) {
      setParameter(operation, "iterations", iterations);
    }

    // Use float for intensity
    float intensity = getParameter<float>(operation, "intensity", 0.5f);
    ImGui::Text("Intensity");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##intensity", &intensity, 0.0f, 1.0f, "%.2f")) {
      setParameter(operation, "intensity", intensity);
    }
    break;
  }

  case HeightmapOperationType::NORMALIZE: {
    // Use float for min/max
    float minVal = getParameter<float>(operation, "min", 0.0f);
    ImGui::Text("Min Value");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputFloat("##min", &minVal, 1.0f, 10.0f, "%.1f")) {
      setParameter(operation, "min", minVal);
    }

    float maxVal = getParameter<float>(operation, "max", 255.0f);
    ImGui::Text("Max Value");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputFloat("##max", &maxVal, 1.0f, 10.0f, "%.1f")) {
      setParameter(operation, "max", maxVal);
    }
    break;
  }

  case HeightmapOperationType::CLAMP_VALUES: {
    // Use float for min/max
    float minVal = getParameter<float>(operation, "min", 0.0f);
    ImGui::Text("Clamp Min");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputFloat("##clampmin", &minVal, 1.0f, 10.0f, "%.1f")) {
      setParameter(operation, "min", minVal);
    }

    float maxVal = getParameter<float>(operation, "max", 255.0f);
    ImGui::Text("Clamp Max");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputFloat("##clampmax", &maxVal, 1.0f, 10.0f, "%.1f")) {
      setParameter(operation, "max", maxVal);
    }
    break;
  }
  case Fwg::Terrain::HeightmapOperationType::CRATER_GENERATION: {
    ImGui::TextWrapped("Adds impact craters of varying sizes to the terrain.");

    int count = Fwg::Terrain::getParameter<int>(operation, "count", 50);
    ImGui::Text("Crater Count");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##count", &count, 1, 500)) {
      Fwg::Terrain::setParameter(operation, "count", count);
    }

    float minRadius =
        Fwg::Terrain::getParameter<float>(operation, "minRadius", 5.0f);
    ImGui::Text("Min Radius (pixels)");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##minRadius", &minRadius, 2.0f, 50.0f, "%.1f")) {
      Fwg::Terrain::setParameter(operation, "minRadius", minRadius);
    }

    float maxRadius =
        Fwg::Terrain::getParameter<float>(operation, "maxRadius", 30.0f);
    ImGui::Text("Max Radius (pixels)");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##maxRadius", &maxRadius, 5.0f, 100.0f, "%.1f")) {
      Fwg::Terrain::setParameter(operation, "maxRadius", maxRadius);
    }

    float depthFactor =
        Fwg::Terrain::getParameter<float>(operation, "depthFactor", 0.5f);
    ImGui::Text("Depth Factor");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##depthFactor", &depthFactor, 0.1f, 2.0f, "%.2f")) {
      Fwg::Terrain::setParameter(operation, "depthFactor", depthFactor);
    }

    float rimFactor =
        Fwg::Terrain::getParameter<float>(operation, "rimFactor", 0.3f);
    ImGui::Text("Rim Height Factor");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##rimFactor", &rimFactor, 0.0f, 1.0f, "%.2f")) {
      Fwg::Terrain::setParameter(operation, "rimFactor", rimFactor);
    }

    int seed = Fwg::Terrain::getParameter<int>(operation, "seed", 0);
    if (ImGui::Button("Randomize Seed")) {
      seed = RandNum::getRandom<int>();
      Fwg::Terrain::setParameter(operation, "seed", seed);
    }
    ImGui::SameLine();
    ImGui::Text("Seed: %d", seed);

    break;
  }
  case Fwg::Terrain::HeightmapOperationType::TERRACE_HEIGHTS: {
    ImGui::TextWrapped(
        "Creates stepped plateau terrain with configurable bands.");

    int steps = getParameter<int>(operation, "steps", 5);
    ImGui::Text("Terrace Steps");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt("##steps", &steps, 2, 20)) {
      setParameter(operation, "steps", steps);
    }

    float smoothing = getParameter<float>(operation, "smoothing", 0.5f);
    ImGui::Text("Smoothing");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##smoothing", &smoothing, 0.0f, 2.0f, "%.2f")) {
      setParameter(operation, "smoothing", smoothing);
    }

    bool respectMask = getParameter<bool>(operation, "respectMask", true);
    if (ImGui::Checkbox("Only Apply to Land", &respectMask)) {
      setParameter(operation, "respectMask", respectMask);
    }
    break;
  }

  case Fwg::Terrain::HeightmapOperationType::RIDGE_GENERATION: {
    ImGui::TextWrapped("Adds mountain ridges and watershed lines to terrain.");

    float strength = getParameter<float>(operation, "strength", 20.0f);
    ImGui::Text("Ridge Strength");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##strength", &strength, 0.0f, 100.0f, "%.1f")) {
      setParameter(operation, "strength", strength);
    }

    float frequency = getParameter<float>(operation, "frequency", 2.0f);
    ImGui::Text("Ridge Frequency");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##frequency", &frequency, 0.5f, 10.0f, "%.2f")) {
      setParameter(operation, "frequency", frequency);
    }

    float sharpness = getParameter<float>(operation, "sharpness", 3.0f);
    ImGui::Text("Ridge Sharpness");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##sharpness", &sharpness, 1.0f, 10.0f, "%.1f")) {
      setParameter(operation, "sharpness", sharpness);
    }

    int seed = getParameter<int>(operation, "seed", 0);
    if (ImGui::Button("Randomize Seed##ridge")) {
      seed = RandNum::getRandom<int>();
      setParameter(operation, "seed", seed);
    }
    ImGui::SameLine();
    ImGui::Text("Seed: %d", seed);
    break;
  }

  default:
    ImGui::TextWrapped("No parameters available for this operation type.");
    break;
  }
}


void HeightmapUI::loadHeightmapConfigs() {
  auto &cfg = Fwg::Cfg::Values();
  auto heightmapConfigFolder = cfg.workingDirectory + "configs/heightmap/";
  // gather all files in the working heightmapConfigFolder
  for (const auto &entry :
       std::filesystem::directory_iterator(heightmapConfigFolder)) {
    if (!entry.is_directory() && entry.path().string().contains(".json")) {
      heightmapConfigFiles.push_back(entry.path().string());
      Fwg::Utils::Logging::logLine("Found heightmap config: ", entry);
    }
  }
}


void HeightmapUI::configurePipelineEditor(Fwg::Cfg &cfg) {
  // Wrap entire editor in collapsible header
  if (!ImGui::CollapsingHeader("Heightmap Processing Pipeline")) {
    return;
  }

  auto &pipeline = cfg.terrainConfig.heightmapPipeline;

  ImGui::Spacing();
  ImGui::TextWrapped("Configure the order and parameters of heightmap "
                     "processing operations. Drag to reorder.");
  ImGui::Spacing();

  // Main container - two columns
  ImGui::BeginChild("PipelineEditorContainer",
                    ImVec2(0, ImGui::GetContentRegionAvail().y * 0.5f), false,
                    ImGuiWindowFlags_None);

  // ===== LEFT COLUMN: Operation List =====
  ImGui::BeginChild("OperationList",
                    ImVec2(ImGui::GetContentRegionAvail().x * 0.45f, 0), true,
                    ImGuiWindowFlags_None);

  ImGui::Text("Processing Pipeline");
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
    bool isSelected = (selectedOperationIndex == i);
    ImVec4 bgColor =
        isSelected ? ImVec4(0.26f, 0.59f, 0.98f, 0.40f) : ImVec4(0, 0, 0, 0);

    ImGui::PushStyleColor(ImGuiCol_Header, bgColor);
    if (ImGui::Selectable(operation.name.c_str(), isSelected,
                          ImGuiSelectableFlags_SpanAllColumns)) {
      selectedOperationIndex = i;
    }
    ImGui::PopStyleColor();

    ImGui::EndGroup();

    // ===== DRAG AND DROP =====
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
      ImGui::SetDragDropPayload("PIPELINE_OPERATION", &i, sizeof(int));
      ImGui::Text("Moving: %s", operation.name.c_str());
      draggedIndex = i;
      ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload("PIPELINE_OPERATION")) {
        int sourceIndex = *(int *)payload->Data;

        // Reorder operations
        if (sourceIndex != i) {
          auto temp = pipeline.operations[sourceIndex];
          pipeline.operations.erase(pipeline.operations.begin() + sourceIndex);
          pipeline.operations.insert(pipeline.operations.begin() + i, temp);

          // Update selection
          selectedOperationIndex = i;
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
    ImGui::OpenPopup("AddOperationPopup");
  }

  // ===== ADD OPERATION POPUP =====
  if (ImGui::BeginPopup("AddOperationPopup")) {
    ImGui::Text("Add Processing Operation");
    ImGui::Separator();

    struct OpTemplate {
      Fwg::Terrain::HeightmapOperationType type;
      const char *name;
      const char *desc;
    };

    static const OpTemplate templates[] = {
        {Fwg::Terrain::HeightmapOperationType::RANDOMIZE_WEIGHTS,
         "Randomize Weights", "Randomize noise weights"},
        {Fwg::Terrain::HeightmapOperationType::GAUSSIAN_BLUR_WEIGHTS,
         "Gaussian Blur of noise weights", "Smooth weights"},
        {Fwg::Terrain::HeightmapOperationType::APPLY_BASE_ALTITUDE,
         "Apply Base Altitude",
         "Modify terrain based on input landform base altitude"},
        {Fwg::Terrain::HeightmapOperationType::APPLY_LAND_LAYERS,
         "Apply Land/Sea Layers", "Detail layers based on coast distance"},
        {Fwg::Terrain::HeightmapOperationType::GAUSSIAN_BLUR, "Gaussian Blur",
         "Smooth terrain"},
        {Fwg::Terrain::HeightmapOperationType::GULLY_EROSION, "Gully Erosion",
         "Erosion simulation"},
        {Fwg::Terrain::HeightmapOperationType::NORMALIZE, "Normalize Heights",
         "Normalize to range"},
        {Fwg::Terrain::HeightmapOperationType::CLAMP_VALUES, "Clamp Values",
         "Clamp to min/max"},
        {Fwg::Terrain::HeightmapOperationType::CRATER_GENERATION,
         "Crater Generation", "Add impact craters to terrain"}};

    for (const auto &t : templates) {
      if (ImGui::Selectable(t.name)) {
        Fwg::Terrain::HeightmapOperation newOp;
        newOp.type = t.type;
        newOp.name = t.name;
        newOp.enabled = true;

        // Use setParameter helper with correct types
        switch (t.type) {
        case Fwg::Terrain::HeightmapOperationType::RANDOMIZE_WEIGHTS:
          Fwg::Terrain::setParameter(newOp, "randomisationFactor", 0.5f);
          break;
        case Fwg::Terrain::HeightmapOperationType::GAUSSIAN_BLUR_WEIGHTS:
          Fwg::Terrain::setParameter(newOp, "radius", 2.0f);
          Fwg::Terrain::setParameter(newOp, "sigma", 1.0f);
          break;
        case Fwg::Terrain::HeightmapOperationType::APPLY_BASE_ALTITUDE:
          Fwg::Terrain::setParameter(newOp, "baseAltitudeEffects", 0.5f);
          Fwg::Terrain::setParameter(newOp, "blurFactor", 1.0f);
          break;
        case Fwg::Terrain::HeightmapOperationType::APPLY_LAND_LAYERS:
          Fwg::Terrain::setParameter(newOp, "useWeights", true);
          break;
        case Fwg::Terrain::HeightmapOperationType::GAUSSIAN_BLUR:
          Fwg::Terrain::setParameter(newOp, "radius", 2.0f);
          Fwg::Terrain::setParameter(newOp, "sigma", 1.0f);
          break;
        case Fwg::Terrain::HeightmapOperationType::GULLY_EROSION:
          Fwg::Terrain::setParameter(newOp, "iterations", 1);
          Fwg::Terrain::setParameter(newOp, "intensity", 0.5f);
          break;
        case Fwg::Terrain::HeightmapOperationType::NORMALIZE:
          Fwg::Terrain::setParameter(newOp, "min", 0.0f);
          Fwg::Terrain::setParameter(newOp, "max", 255.0f);
          break;
        case Fwg::Terrain::HeightmapOperationType::CLAMP_VALUES:
          Fwg::Terrain::setParameter(newOp, "min", 0.0f);
          Fwg::Terrain::setParameter(newOp, "max", 255.0f);
          break;
        case Fwg::Terrain::HeightmapOperationType::CRATER_GENERATION:
          Fwg::Terrain::setParameter(newOp, "count", 50);
          Fwg::Terrain::setParameter(newOp, "minRadius", 5.0f);
          Fwg::Terrain::setParameter(newOp, "maxRadius", 30.0f);
          Fwg::Terrain::setParameter(newOp, "depthFactor", 0.5f);
          Fwg::Terrain::setParameter(newOp, "rimFactor", 0.3f);
          Fwg::Terrain::setParameter(newOp, "seed", 0);
          break;
        case Fwg::Terrain::HeightmapOperationType::TERRACE_HEIGHTS:
          Fwg::Terrain::setParameter(newOp, "steps", 5);
          Fwg::Terrain::setParameter(newOp, "smoothing", 0.5f);
          Fwg::Terrain::setParameter(newOp, "respectMask", true);
          break;
        case Fwg::Terrain::HeightmapOperationType::RIDGE_GENERATION:
          Fwg::Terrain::setParameter(newOp, "strength", 20.0f);
          Fwg::Terrain::setParameter(newOp, "frequency", 2.0f);
          Fwg::Terrain::setParameter(newOp, "sharpness", 3.0f);
          Fwg::Terrain::setParameter(newOp, "seed", 0);
          break;
        }

        pipeline.operations.push_back(newOp);
        selectedOperationIndex = pipeline.operations.size() - 1;
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

  if (selectedOperationIndex >= 0 &&
      selectedOperationIndex < pipeline.operations.size()) {
    if (ImGui::Button("- Remove Selected", ImVec2(-1, 0))) {
      pipeline.operations.erase(pipeline.operations.begin() +
                                selectedOperationIndex);
      selectedOperationIndex = std::max(0, selectedOperationIndex - 1);
    }
  }

  ImGui::EndChild(); // OperationList

  ImGui::SameLine();

  // ===== RIGHT COLUMN: Parameter Editor =====
  ImGui::BeginChild("ParameterEditor", ImVec2(0, 0), true,
                    ImGuiWindowFlags_None);

  if (selectedOperationIndex >= 0 &&
      selectedOperationIndex < pipeline.operations.size()) {

    auto &operation = pipeline.operations[selectedOperationIndex];

    ImGui::Text("Operation: %s", operation.name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Editable name
    char nameBuf[128];
    strncpy(nameBuf, operation.name.c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';

    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##opname", nameBuf, sizeof(nameBuf))) {
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

  ImGui::EndChild(); // ParameterEditor

  ImGui::EndChild(); // PipelineEditorContainer
}


int HeightmapUI::showHeightmapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                                  UIContext &uiContext) {
  static bool updateLayer = false;
  static int selectedLayer = 0;
  static bool rerandomiseSeed = false;
  static int layerTypeSelection = 0; // 0=Shape, 1=Land, 2=Sea
  static int previousLayerTypeSelection = 0;

  if (UI::Elements::BeginSubTabItem("Heightmap")) {
    if (uiContext.tabSwitchEvent()) {

      if (fwg.terrainData.detailedHeightMap.size()) {
        auto heightmap =
            Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap);
        uiContext.imageContext.updateImage(0, heightmap);
        uiContext.imageContext.updateImage(1, heightmap);

        if (updateLayer) {
          auto &selectedLayers =
              (layerTypeSelection == 0)   ? fwg.terrainData.shapeLayers
              : (layerTypeSelection == 1) ? fwg.terrainData.landLayers
                                          : fwg.terrainData.seaLayers;
          if (selectedLayer < selectedLayers.size() &&
              selectedLayers[selectedLayer].size() &&
              fwg.terrainData.detailedHeightMap.size()) {
            uiContext.imageContext.updateImage(
                1, Fwg::Gfx::Image(cfg.width, cfg.height, 24,
                                   selectedLayers[selectedLayer]));
          }
          updateLayer = false;
        } else {
          if (fwg.terrainData.landMask.size()) {
            uiContext.imageContext.updateImage(
                1, Fwg::Gfx::landFormMap(fwg.terrainData));
          } else {
            uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
          }
        }
      } else {
        uiContext.imageContext.updateImage(0, Fwg::Gfx::Image());
        uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
      }
    }

    uiContext.helpContext.showHelpTextBox("Heightmap");

    ImGui::SeparatorText("Heightmap Configuration Presets");

    // Heightmap preset selection with auto-initialization
    static int activeConfigIndex = -1;
    static bool initializedDefault = false;

    // Initialize with default config on first run
    if (!initializedDefault && heightmapConfigFiles.size() > 0) {
      // Try to find "default.json" in the list
      for (int i = 0; i < heightmapConfigFiles.size(); ++i) {
        if (heightmapConfigFiles[i].find("default.json") != std::string::npos) {
          activeConfigIndex = i;
          break;
        }
      }
      // If "default.json" not found, use first config
      if (activeConfigIndex == -1) {
        activeConfigIndex = 0;
      }
      initializedDefault = true;
    }

    ImGui::Text("Select Preset:");
    ImGui::SameLine();

    ImGui::PushItemWidth(300.0f);
    if (ImGui::BeginCombo("##HeightmapPresets",
                          activeConfigIndex >= 0 &&
                                  activeConfigIndex <
                                      heightmapConfigFiles.size()
                              ? heightmapConfigFiles[activeConfigIndex].c_str()
                              : "Select a preset...")) {
      for (int i = 0; i < heightmapConfigFiles.size(); ++i) {
        bool isActive = (i == activeConfigIndex);
        if (ImGui::Selectable(heightmapConfigFiles[i].c_str(), isActive)) {
          activeConfigIndex = i;
          cfg.readHeightmapConfig(heightmapConfigFiles[i]);
        }
        if (isActive)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::SeparatorText("Basic Parameters");

    // Main parameters grid
    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      grid.AddText("Sealevel", "%d", cfg.seaLevel);

      if (cfg.landInputMode == Fwg::Terrain::InputMode::HEIGHTMAP) {
        grid.AddSliderFloat("Target Land %", &cfg.landPercentage, 0.0f, 1.0f);
        grid.AddInputInt("Height Adjustments", &cfg.heightAdjustments, 0, 100);

      } else {
        grid.AddText("Land Percentage", "%.2f%%", cfg.landPercentage * 100.0f);
      }
      grid.AddInputFloat("Coastal Distance", &cfg.layerApplicationFactor, 0.0f,
                         10.0f);
      grid.AddInputDouble("Lake Size Factor", &cfg.lakeMaxShare, 0.0, 1.0);
      if (grid.AddInputInt("Max Land Height", &cfg.maxLandHeight,
                           cfg.seaLevel + 1, 255)) {
        cfg.maxLandHeight =
            std::clamp(cfg.maxLandHeight, cfg.seaLevel + 1, 255);
      }
    }

    ImGui::Spacing();

    configureLandElevationFactors(cfg, fwg);

    ImGui::Spacing();
    ImGui::SeparatorText("Advanced Heightmap Settings");

    // Advanced settings grid
    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      grid.AddSliderFloat("Heightmap Frequency",
                          &cfg.heightmapFrequencyModifier, 0.1f, 10.0f);
      grid.AddSliderFloat("Edge Fade Width Factor",
                          &cfg.globalEdgeFadeWidthModifier, 0.0f, 10.0f);
      grid.AddSliderFloat("Edge Fade Height Factor",
                          &cfg.globalEdgeFadeHeightModifier, 0.0f, 10.0f);
    }

    ImGui::Spacing();

    // Replace checkbox with collapsing header
    if (ImGui::CollapsingHeader("Layer Editor", ImGuiTreeNodeFlags_None)) {
      ImGui::Spacing();

      // Layer type selector
      ImGui::Text("Layer Type:");
      ImGui::SameLine();
      ImGui::RadioButton("Shape", &layerTypeSelection, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Land", &layerTypeSelection, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Sea", &layerTypeSelection, 2);

      // Reset selection when switching layer types
      if (previousLayerTypeSelection != layerTypeSelection) {
        selectedLayer = 0;
        updateLayer = true;
        uiContext.imageContext.resetTexture(1);
        previousLayerTypeSelection = layerTypeSelection;
      }

      // Get current layer vector
      std::vector<LayerConfig> *currentLayers = nullptr;
      const char *layerTypeName = "";

      switch (layerTypeSelection) {
      case 0:
        currentLayers = &cfg.shapeLayers;
        layerTypeName = "Shape";
        break;
      case 1:
        currentLayers = &cfg.landLayers;
        layerTypeName = "Land";
        break;
      case 2:
        currentLayers = &cfg.seaLayers;
        layerTypeName = "Sea";
        break;
      }

      ImGui::Text("%s Layers: %zu", layerTypeName, currentLayers->size());
      ImGui::Spacing();

      // Layer selection and editing in two columns
      {
        ImGui::BeginChild("LayerSelection",
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.25f,
                                 ImGui::GetContentRegionAvail().y * 0.6f),
                          true, ImGuiWindowFlags_None);

        ImGui::TextUnformatted("Layer List");
        ImGui::Separator();

        for (int i = 0; i < currentLayers->size(); i++) {
          char label[64];
          snprintf(label, sizeof(label), "%s Layer %d", layerTypeName, i);

          ImGui::PushID(i);
          if (ImGui::Selectable(label, selectedLayer == i)) {
            selectedLayer = i;
            updateLayer = true;
            uiContext.imageContext.resetTexture(1);
          }
          ImGui::PopID();
        }

        ImGui::Separator();

        if (ImGui::Button("Add Layer", ImVec2(-1, 0))) {
          LayerConfig newLayer{};
          newLayer.noiseType = 3;
          newLayer.fractalType = 3;
          newLayer.type = static_cast<LayerType>(layerTypeSelection);
          newLayer.fractalFrequency = 0.5f;
          newLayer.fractalOctaves = 11;
          newLayer.fractalGain = 0.5f;
          newLayer.seed = RandNum::getRandom<int>();
          newLayer.edgeFadeWidth = 10.0;
          newLayer.edgeFadeHeight = 0.0;
          newLayer.weight = 1.0;
          newLayer.altitudeWeightStart = 0.0;
          newLayer.altitudeWeightEnd = 0.0;
          newLayer.minHeight = 0;
          newLayer.maxHeight = 100;
          newLayer.tanFactor = 0.0;

          currentLayers->push_back(newLayer);
          selectedLayer = currentLayers->size() - 1;
        }

        if (currentLayers->size() > 0 &&
            ImGui::Button("Remove Layer", ImVec2(-1, 0))) {
          if (selectedLayer < currentLayers->size()) {
            currentLayers->erase(currentLayers->begin() + selectedLayer);
            selectedLayer = std::max(0, selectedLayer - 1);
            updateLayer = true;
          }
        }

        ImGui::EndChild();
      }

      ImGui::SameLine();

      // Layer properties editor
      {
        ImGui::BeginChild("LayerEdit",
                          ImVec2(ImGui::GetContentRegionAvail().x * 1.0f,
                                 ImGui::GetContentRegionAvail().y * 0.6f),
                          true, ImGuiWindowFlags_None);

        if (selectedLayer < currentLayers->size()) {
          LayerConfig &layer = (*currentLayers)[selectedLayer];

          ImGui::Text("Editing %s Layer %d", layerTypeName, selectedLayer);
          ImGui::Separator();
          ImGui::Spacing();

          // Noise settings grid
          {
            UI::Elements::GridLayout grid(2, 180.0f, 8.0f);

            static const char *NoiseTypeNames[] = {
                "OpenSimplex2", "OpenSimplex2S", "Cellular",
                "Perlin",       "ValueCubic",    "Value"};

            static const char *FractalTypeNames[] = {"None",
                                                     "FBm",
                                                     "Ridged",
                                                     "PingPong",
                                                     "DomainWarpProgressive",
                                                     "DomainWarpIndependent"};

            // Clamp indices
            layer.noiseType = std::clamp(
                layer.noiseType, 0, (int)(IM_ARRAYSIZE(NoiseTypeNames)) - 1);
            layer.fractalType =
                std::clamp(layer.fractalType, 0,
                           (int)(IM_ARRAYSIZE(FractalTypeNames)) - 1);

            // Show combo boxes inline
            ImGui::PushItemWidth(180.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%-*s", 25, "Noise Type");
            ImGui::SameLine();
            ImGui::Combo("##NoiseType", &layer.noiseType, NoiseTypeNames,
                         IM_ARRAYSIZE(NoiseTypeNames));

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%-*s", 25, "Fractal Type");
            ImGui::SameLine();
            ImGui::Combo("##FractalType", &layer.fractalType, FractalTypeNames,
                         IM_ARRAYSIZE(FractalTypeNames));
            ImGui::PopItemWidth();

            grid.AddInputFloat("Frequency", &layer.fractalFrequency, 0.01f,
                               10.0f);
            grid.AddInputInt("Octaves", &layer.fractalOctaves, 1, 20);
            grid.AddInputFloat("Gain", &layer.fractalGain, 0.0f, 1.0f);
            grid.AddInputDouble("Weight", &layer.weight, 0.0, 10.0);

            int tempMinHeight = layer.minHeight;
            if (grid.AddInputInt("Min Height", &tempMinHeight, 0, 255)) {
              layer.minHeight =
                  static_cast<unsigned char>(std::clamp(tempMinHeight, 0, 255));
            }

            int tempMaxHeight = layer.maxHeight;
            if (grid.AddInputInt("Max Height", &tempMaxHeight, 0, 255)) {
              layer.maxHeight =
                  static_cast<unsigned char>(std::clamp(tempMaxHeight, 0, 255));
            }

            grid.AddInputDouble("Tan Factor", &layer.tanFactor, 0.0, 100.0);
            grid.AddInputDouble("Edge Fade Width", &layer.edgeFadeWidth, 0.0,
                                100.0);
            grid.AddInputDouble("Edge Fade Height", &layer.edgeFadeHeight, 0.0,
                                100.0);

            if (layerTypeSelection != 0) { // Land or Sea layers
              ImGui::Spacing();
              ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                                 "Altitude Weight Settings");
              grid.AddInputDouble("Weight Start", &layer.altitudeWeightStart,
                                  0.0, 1.0);
              grid.AddInputDouble("Weight End", &layer.altitudeWeightEnd, 0.0,
                                  1.0);
            }
          }

          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();

          if (ImGui::Button("Randomize Seed", ImVec2(150, 0))) {
            layer.seed = RandNum::getRandom<int>();
          }
          ImGui::SameLine();
          ImGui::Text("Current Seed: %d", layer.seed);

        } else {
          ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
                             "No layer selected or layer list is empty");
        }

        ImGui::EndChild();
      }
    } // End of CollapsingHeader
    ImGui::Spacing();

    // Pipeline Editor
    configurePipelineEditor(cfg);

    ImGui::Spacing();
    ImGui::SeparatorText("Generation Controls");

    ImGui::Checkbox("Randomize seed on each generation", &rerandomiseSeed);
    ImGui::Spacing();

    // Generation buttons based on input mode
    switch (cfg.landInputMode) {
    case Fwg::Terrain::InputMode::HEIGHTMAP: {
      if (UI::Elements::Button("Generate Random Continent Shape", false,
                               ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &uiContext, this]() {
              fwg.genHeight();
              uiContext.imageContext.resetTexture();
              updateLayer = true;
              return true;
            });
      }

      ImGui::SameLine();

      if (UI::Elements::Button("Generate Heightmap Details", false,
                               ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &uiContext, this]() {
              fwg.genLand();
              updateLayer = true;
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (UI::Elements::ImportantStepButton(
              "Generate Complete Heightmap from new seed", ImVec2(250, 0))) {
        cfg.randomSeed = true;
        cfg.reRandomize();
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &uiContext, &cfg, this]() {
              fwg.genHeight();
              fwg.genLand();
              uiContext.imageContext.resetTexture();
              updateLayer = true;
              return true;
            });
      }
      break;
    }

    case Fwg::Terrain::InputMode::HEIGHTSKETCH: {
      if (UI::Elements::Button("Generate from Sketch", false, ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        fwg.genHeightFromInput(cfg, cfg.mapsPath + "/heightSketchInput.png",
                               cfg.landInputMode);
        uiContext.imageContext.resetTexture();
      }

      ImGui::SameLine();

      if (UI::Elements::Button("Apply Detail Layers", false, ImVec2(250, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &uiContext, this]() {
              fwg.genLand();
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
      break;
    }
    case Fwg::Terrain::InputMode::TOPOGRAPHY: {
      // TODO Not supported yet

      // if (ImGui::Button("Generate land shape from topography")) {
      // if (rerandomiseSeed) {
      //   cfg.randomSeed = true;
      //   cfg.reRandomize();
      // }
      //   fwg.genHeightFromInput(cfg, cfg.mapsPath + "/topographyInput.png",
      //                          cfg.landInputMode);
      // }
      break;
    }
    case Fwg::Terrain::InputMode::LANDFORM: {
      auto classificationGuard =
          UI::PrerequisiteChecker::requireSilent({UI::Prerequisite{
              "Classification", "Complete landform classification", [&]() {
                return uiContext.generationContext
                               .amountClassificationsNeeded <= 0 &&
                       !uiContext.generationContext.analyze;
              }}});

      if (classificationGuard.ready() &&
          UI::Elements::ImportantStepButton("Generate from Landform Input",
                                            ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext, this]() {
              if (fwg.genHeightFromInput(
                      cfg, cfg.mapsPath + "/classifiedLandInput.png",
                      cfg.landInputMode)) {
                fwg.genLand();
              }
              updateLayer = true;
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
      break;
    }

    case Fwg::Terrain::InputMode::LANDMASK: {
      if (UI::Elements::ImportantStepButton("Generate from Landmask",
                                            ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext, this]() {
              fwg.genHeightFromInput(cfg, cfg.mapsPath + "/landmaskInput.png",
                                     cfg.landInputMode);
              fwg.genLand();
              updateLayer = true;
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
      break;
    }

    default:
      break;
    }

    // Drag & drop handler
    if (uiContext.triggeredDrag) {
      uiContext.triggeredDrag = false;
      cfg.allowHeightmapModification = false;
      fwg.loadHeight(
          cfg, IO::Reader::readHeightmapImage(uiContext.draggedFile, cfg));
      uiContext.imageContext.resetTexture();
    }

    ImGui::EndTabItem();
  }
  return 0;
}

} // namespace Fwg::UI