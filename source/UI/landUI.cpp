#include "UI/LandUI.h"

namespace Fwg {
LandUI::LandUI() {}
LandUI::LandUI(std::shared_ptr<UIUtils> uiUtils) { this->uiUtils = uiUtils; }

void LandUI::RenderScrollableLandInput(
    std::vector<Fwg::Gfx::Colour> &imageData,
    const std::vector<Fwg::Terrain::LandformDefinition> &landformDefinitions) {
  // Begin the scrollable area
  ImGui::BeginChild("ScrollingRegion",
                    ImVec2(ImGui::GetContentRegionAvail().x,
                           ImGui::GetContentRegionAvail().y * 0.8),
                    false, ImGuiWindowFlags_HorizontalScrollbar);

  // --- Selection state ---
  static bool singularEdit = false;
  static std::unordered_set<Fwg::Gfx::Colour> selectedInputs;
  static std::optional<Fwg::Gfx::Colour> lastClickedInput;
  static Fwg::Gfx::Colour *selectedLabel = nullptr;
  static Fwg::Gfx::Colour selectedId;

  // --- Begin main loop over inputs ---
  int index = 0;
  std::vector<Fwg::Gfx::Colour>
      colourOrder; // To map shift-range indices
                   // --- Global apply-to-selected button ---
  if (!selectedInputs.empty() && !singularEdit) {
    ImGui::Separator();
    ImGui::Text("Selected items: %zu", selectedInputs.size());
    ImGui::SameLine();

    if (ImGui::Button("Classify selected")) {
      ImGui::OpenPopup("ClassificationPopup");
    }

    ImGui::SameLine();

    if (ImGui::Button("Apply type to all selected")) {
      for (const auto &selId : selectedInputs) {
        if (landInputColours.getMap().contains(selId)) {
          auto &entry = landInputColours.getMap().at(selId);
          for (auto &pix : entry.pixels)
            imageData[pix] = entry.out;
        }
      }
      uiUtils->resetTexture();
      selectedInputs.clear();
    }
  }
  for (auto &input : landInputColours.getMap()) {
    colourOrder.push_back(input.second.in);
    ++index;
  }
  std::sort(colourOrder.begin(), colourOrder.end(), Fwg::Gfx::colourSort);

  // --- Handle click events for direct selection from the map ---
  auto &clickEvents = uiUtils->clickEvents;
  if (!clickEvents.empty()) {
    auto event = clickEvents.front();
    clickEvents.pop();

    if (event.pixel >= 0 && event.pixel < imageData.size()) {
      Fwg::Gfx::Colour clickedColour = imageData[event.pixel];

      // Ensure clicked colour is known
      if (landInputColours.getMap().contains(clickedColour)) {

        ImGuiIO &io = ImGui::GetIO();

        if (io.KeyCtrl) {
          // CTRL toggle selection
          singularEdit = false;

          if (selectedInputs.contains(clickedColour))
            selectedInputs.erase(clickedColour);
          else
            selectedInputs.insert(clickedColour);

          // Update last clicked, but do NOT override singular selection
          lastClickedInput = clickedColour;
        } else {
          // Regular click  single select + popup
          singularEdit = true;
          selectedInputs.clear();
          selectedInputs.insert(clickedColour);
          lastClickedInput = clickedColour;

          ImGui::OpenPopup("ClassificationPopup");
        }
      }
    }
  }

  // --- Iterate with index tracking ---
  for (int i = 0; i < colourOrder.size(); ++i) {
    auto &id = colourOrder[i];
    auto &entry = landInputColours.getMap().at(id);
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
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(0.3f, 0.6f, 1.0f, 1.0f)); // Blue tint
    }

    std::string buttonLabel = "Select type for " + entry.in.toString();
    if (ImGui::Button(buttonLabel.c_str())) {
      ImGuiIO &io = ImGui::GetIO();

      if (io.KeyShift && lastClickedInput.has_value()) {
        singularEdit = false;
        // --- SHIFT: Select range ---
        auto it1 = std::find(colourOrder.begin(), colourOrder.end(),
                             *lastClickedInput);
        auto it2 = std::find(colourOrder.begin(), colourOrder.end(), entry.in);
        if (it1 != colourOrder.end() && it2 != colourOrder.end()) {
          if (it1 > it2)
            std::swap(it1, it2);
          for (auto it = it1; it <= it2; ++it)
            selectedInputs.insert(*it);
        }
      } else if (io.KeyCtrl) {
        singularEdit = false;
        // --- CTRL: Toggle single ---
        if (selectedInputs.contains(entry.in))
          selectedInputs.erase(entry.in);
        else
          selectedInputs.insert(entry.in);
        lastClickedInput = entry.in;
        // if we have fewer than 2 selected, go back to singular
        if (selectedInputs.size() <= 1)
          singularEdit = true;
      } else {
        // --- Regular click: single select ---
        singularEdit = true;
        selectedInputs.clear();
        selectedInputs.insert(entry.in);
        lastClickedInput = entry.in;
        ImGui::OpenPopup("ClassificationPopup");
      }
    }

    if (isSelected)
      ImGui::PopStyleColor();

    ImGui::SameLine();

    // --- Highlighted entries ---
    bool isHighlighted = highlightedInputs.contains(entry.in);
    if (isHighlighted) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); // Red
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    }

    // --- Apply button (per item) ---
    if (ImGui::Button(("Apply type for " + entry.in.toString()).c_str())) {
      for (auto &pix : entry.pixels)
        imageData[pix] = entry.out;
      uiUtils->resetTexture();
      highlightedInputs.erase(entry.in);
    }

    if (isHighlighted)
      ImGui::PopStyleColor(3);
  }

  // --- Classification Popup ---
  if (ImGui::BeginPopup("ClassificationPopup")) {
    static std::vector<Fwg::Gfx::Colour> popupSelectedInputs;
    ImGui::SeparatorText("Classify selected land input colours");

    const int columns = 3;
    int count = 0;

    ImGui::BeginGroup();
    for (auto &landformDefinition : landformDefinitions) {
      // Each button has a fixed width to keep the grid tidy
      ImGui::PushID(landformDefinition.colour.toString().c_str());
      if (ImGui::Button(landformDefinition.name.c_str(), ImVec2(150, 0))) {

        // Apply chosen classification to all selected items
        for (const auto &selId : selectedInputs) {
          auto &entry = landInputColours.getMap().at(selId);
          entry.out = landformDefinition.colour;
          highlightedInputs.insert(selId);
        }
        selectedInputs.clear();
        ImGui::CloseCurrentPopup();
      }
      ImGui::PopID();

      // Grid layout: place next button on the same line until row is full
      count++;
      if (count % columns != 0)
        ImGui::SameLine();
    }
    ImGui::EndGroup();

    ImGui::EndPopup();
  }
  // End the scrollable area
  ImGui::EndChild();
}

bool LandUI::analyseLandMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                            const Fwg::Gfx::Image &landInput,
                            int &amountClassificationsNeeded) {
  landInputColours.clear();
  amountClassificationsNeeded = 0;
  ImGui::SameLine();
  // reset count every time we press the button
  for (auto &elem : landInputColours.getMap()) {
    elem.second.pixels.clear();
  }
  static bool classificationNeeded = false;
  auto imageIndex = 0;
  // init and count colours
  for (auto &colour : landInput.imageData) {
    // first count classifications
    if (landInputColours.contains(colour) &&
        landInputColours[colour].pixels.size() == 0 &&
        !allowedLandInputs.contains(colour)) {
      amountClassificationsNeeded++;
    }

    ImVec4 inputColourVisualHelp = ImVec4(
        ((float)colour.getRed()) / 255.0f, ((float)colour.getGreen()) / 255.0f,
        ((float)colour.getBlue()) / 255.0f, 1.0f);
    // check if the input is of permitted colours or first needs to be
    // classified
    if (!allowedLandInputs.contains(colour)) {
      // this input colour is not classified yet
      classificationNeeded = true;
      // if we haven't set this colour yet
      if (!landInputColours.contains(colour)) {
        // set it
        landInputColours.setValue(
            colour, ElevationInput{colour, colour, "Unclassified",
                                   cfg.terrainConfig.landformDefinitions[0],
                                   inputColourVisualHelp});
        landInputColours[colour].pixels.push_back(imageIndex);
        amountClassificationsNeeded++;
      } else {
        // otherwise increment how often this colour is here
        landInputColours[colour].pixels.push_back(imageIndex);
      }
    } else {
      // if we haven't set this colour yet
      if (!landInputColours.contains(colour)) {
        // set it with its valid output colour
        landInputColours.setValue(
            colour,
            ElevationInput{colour, colour, allowedLandInputs[colour].name,
                           allowedLandInputs[colour], inputColourVisualHelp});
        landInputColours[colour].pixels.push_back(imageIndex);
      } else {
        // otherwise increment how often this colour is here
        landInputColours[colour].pixels.push_back(imageIndex);
      }
    }
    imageIndex++;
  }
  return !classificationNeeded;
}

void LandUI::complexLandMapping(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                                bool &analyse,
                                int &amountClassificationsNeeded) {
  ImGui::Value("Colours needing classification: ", amountClassificationsNeeded);
  std::vector<const char *> imageColours;
  std::vector<const char *> selectableTypes;
  for (auto &elem : landInputColours.getMap()) {
    selectableTypes.push_back(elem.second.rgbName.c_str());
  }
  for (auto &landInColour : landInputColours.getMap()) {
    // only show elems with a size
    if (landInColour.second.pixels.size()) {
      landInColour.second.rgbName =
          (allowedLandInputs.contains(landInColour.second.out)
               ? allowedLandInputs[landInColour.second.out].name
               : "UNCLASSIFIED");
    }
  }
  RenderScrollableLandInput(landInput.imageData,
                            cfg.terrainConfig.landformDefinitions);
  if (highlightedInputs.size() > 0) {
    ImGui::Text("Before next analysis, apply all types");
    if (ImGui::Button("Apply all")) {
      for (auto &input : landInputColours.getMap()) {
        if (highlightedInputs.contains(input.second.in)) {
          for (auto &pix : input.second.pixels) {
            landInput.setColourAtIndex(pix, input.second.out);
          }
          highlightedInputs.erase(input.second.in);
        }
      }
      uiUtils->resetTexture();
    }
  } else if (ImGui::Button("Analyse Input") || analyse) {
    // always reload the classified map from disk
    Fwg::Gfx::Png::save(landInput, cfg.mapsPath + "/classifiedLandInput.png",
                        false);
    analyseLandMap(cfg, fwg, landInput, amountClassificationsNeeded);
    if (!amountClassificationsNeeded) {
      fwg.genHeightFromInput(cfg, cfg.mapsPath + "/classifiedLandInput.png",
                             cfg.landInputMode);
    }
    uiUtils->resetTexture();

    analyse = false;
  }
}

void LandUI::triggeredLandInput(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                                const std::string &draggedFile,
                                const Fwg::Terrain::InputMode &inputMode) {
  fwg.resetData();
  fwg.configure(cfg);
  // don't immediately generate from the input, instead allow to manually
  // classify all present colours
  landInput = Fwg::IO::Reader::readGenericImage(draggedFile, cfg, false);
  std::string outputPath = "";
  switch (inputMode) {
  case Fwg::Terrain::InputMode::HEIGHTMAP:
    outputPath = cfg.mapsPath + "/heightmapInput.png";
    cfg.allowHeightmapModification = false;
    fwg.loadHeight(cfg, IO::Reader::readHeightmapImage(draggedFile, cfg));
    uiUtils->resetTexture();
    break;
  case Fwg::Terrain::InputMode::HEIGHTSKETCH:
    outputPath = cfg.mapsPath + "/heightSketchInput.png";
    break;
  case Fwg::Terrain::InputMode::TOPOGRAPHY:
    outputPath = cfg.mapsPath + "/topographyInput.png";
    break;

  case Fwg::Terrain::InputMode::LANDMASK:
    outputPath = cfg.mapsPath + "/landmaskInput.png";
    fwg.genHeightFromInput(cfg, draggedFile, inputMode);
    break;
  case Fwg::Terrain::InputMode::LANDFORM:
    outputPath = cfg.mapsPath + "/landformInput.png";
    fwg.genHeightFromInput(cfg, draggedFile, inputMode);
    break;
  default:
    break;
  }

  //  save the landmap to classifiedLandInput.png
  Fwg::Gfx::Png::save(landInput, outputPath);

  uiUtils->resetTexture();
}

void LandUI::draw(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  static std::vector<Fwg::Gfx::Colour> allowedLandInputColours;
  static std::vector<std::string> allowedLandInputColoursNames;
  static Fwg::Gfx::Colour selectedTypeColour;
  static int current_item = 0;
  if (!allowedLandInputColours.size()) {
    for (auto &elem : allowedLandInputs.getMap()) {
      allowedLandInputColours.push_back(elem.second.colour);
      allowedLandInputColoursNames.push_back(elem.second.name);
    }
  }
  ImGui::PushItemWidth(200.0f);
  if (ImGui::BeginCombo("Colour Selection",
                        allowedLandInputColoursNames[current_item].c_str())) {
    for (int n = 0; n < allowedLandInputColours.size(); n++) {
      bool is_selected = (current_item == n);
      if (ImGui::Selectable(allowedLandInputColoursNames[n].c_str(),
                            is_selected)) {
        current_item = n;
      }
      if (is_selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopItemWidth();
  selectedTypeColour = allowedLandInputColours[current_item];
  auto affected = uiUtils->getLatestAffectedPixels();
  if (affected.size() > 0) {
    for (auto &pix : affected) {
      landInput.setColourAtIndex(pix.first.pixel, selectedTypeColour);
    }
    uiUtils->resetTexture();
  }
}

void LandUI::configureLandElevationFactors(Fwg::Cfg &cfg,
                                           Fwg::FastWorldGenerator &fwg) {
  if (cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM) {
    ImGui::SeparatorText("Land Elevation Factors");
    const float labelWidth = 10.0f;  // reserve space for label
    const float inputWidth = 100.0f; // fixed input box width
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
  // REMOVED: Pipeline editor call - we'll call it separately
}

void LandUI::configurePipelineEditor(Fwg::Cfg &cfg) {
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
        case Fwg::Terrain::HeightmapOperationType::GAUSSIAN_BLUR_WEIGHTS:
          Fwg::Terrain::setParameter(newOp, "radius", 2.0f); // float
          Fwg::Terrain::setParameter(newOp, "sigma", 1.0f);  // float
          break;
        case Fwg::Terrain::HeightmapOperationType::APPLY_BASE_ALTITUDE:
          Fwg::Terrain::setParameter(newOp, "baseAltitudeEffects",
                                     0.5f); // float
          Fwg::Terrain::setParameter(newOp, "blurFactor",
                                     1.0f); // float
          break;
        case Fwg::Terrain::HeightmapOperationType::APPLY_LAND_LAYERS:
          Fwg::Terrain::setParameter(newOp, "useWeights", true); // bool
          break;
        case Fwg::Terrain::HeightmapOperationType::GAUSSIAN_BLUR:
          Fwg::Terrain::setParameter(newOp, "radius", 2.0f); // float
          Fwg::Terrain::setParameter(newOp, "sigma", 1.0f);  // float
          break;
        case Fwg::Terrain::HeightmapOperationType::GULLY_EROSION:
          Fwg::Terrain::setParameter(newOp, "iterations", 1);   // int
          Fwg::Terrain::setParameter(newOp, "intensity", 0.5f); // float
          break;
        case Fwg::Terrain::HeightmapOperationType::NORMALIZE:
          Fwg::Terrain::setParameter(newOp, "min", 0.0f);   // float
          Fwg::Terrain::setParameter(newOp, "max", 255.0f); // float
          break;
        case Fwg::Terrain::HeightmapOperationType::CLAMP_VALUES:
          Fwg::Terrain::setParameter(newOp, "min", 0.0f);   // float
          Fwg::Terrain::setParameter(newOp, "max", 255.0f); // float
          break;
        case Fwg::Terrain::HeightmapOperationType::CRATER_GENERATION:
          Fwg::Terrain::setParameter(newOp, "count", 50);         // int
          Fwg::Terrain::setParameter(newOp, "minRadius", 5.0f);   // float
          Fwg::Terrain::setParameter(newOp, "maxRadius", 30.0f);  // float
          Fwg::Terrain::setParameter(newOp, "depthFactor", 0.5f); // float
          Fwg::Terrain::setParameter(newOp, "rimFactor", 0.3f);   // float
          Fwg::Terrain::setParameter(newOp, "seed", 0);           // int
          break;
        case Fwg::Terrain::HeightmapOperationType::TERRACE_HEIGHTS:
          Fwg::Terrain::setParameter(newOp, "steps", 5);          // int
          Fwg::Terrain::setParameter(newOp, "smoothing", 0.5f);   // float
          Fwg::Terrain::setParameter(newOp, "respectMask", true); // bool
          break;
        case Fwg::Terrain::HeightmapOperationType::RIDGE_GENERATION:
          Fwg::Terrain::setParameter(newOp, "strength", 20.0f); // float
          Fwg::Terrain::setParameter(newOp, "frequency", 2.0f); // float
          Fwg::Terrain::setParameter(newOp, "sharpness", 3.0f); // float
          Fwg::Terrain::setParameter(newOp, "seed", 0);         // int
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

// Helper: Render operation-specific parameters
void LandUI::renderOperationParameters(
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

} // namespace Fwg