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
                                   fwg.terrainData.landformDefinitions[0],
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
                            fwg.terrainData.landformDefinitions);
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
    Fwg::Gfx::Bmp::save(landInput, cfg.mapsPath + "//classifiedLandInput.png");
    analyseLandMap(cfg, fwg, landInput, amountClassificationsNeeded);
    analyse = false;
  }
}

void LandUI::triggeredLandInput(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                                const std::string &draggedFile,
                                const Fwg::Terrain::InputMode &inputMode) {
  // if (fwg.heightMap.initialised() && !classifyInput) {
  //   ImGui::OpenPopup("Drag info");
  // } else
  if (inputMode == Fwg::Terrain::InputMode::SHAPE) {
    fwg.resetData();
    fwg.genHeightFromInput(cfg, draggedFile, inputMode);
    fwg.genLand();
    // buffer this path in case we want to regen heightmap from this
    loadedTerrainFile = draggedFile;

  } else {
    fwg.resetData();
    fwg.configure(cfg);
    // don't immediately generate from the input, instead allow to manually
    // classify all present colours
    landInput = Fwg::IO::Reader::readGenericImage(draggedFile, cfg, false);
    // if (!cfg.validateResolution(landInput.width(), landInput.height())) {
    //   Utils::Logging::logLine("Invalid resolution for land input image");
    //   landInput.clear();
    // }
    std::string outputPath = "";
    switch (inputMode) {
    case Fwg::Terrain::InputMode::HEIGHTMAP:
      outputPath = cfg.mapsPath + "//heightmapInput.png";
      break;
    case Fwg::Terrain::InputMode::TOPOGRAPHY:
      outputPath = cfg.mapsPath + "//topographyInput.png";
      break;
    case Fwg::Terrain::InputMode::LANDFORM:
      outputPath = cfg.mapsPath + "//landformInput.png";
      break;
    default:
      break;
    }

    //  save the landmap to classifiedLandInput.png
    Fwg::Gfx::Png::save(landInput, outputPath);
  }

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

    auto addFactor = [&](const char *label, float &value) {
      ImGui::BeginGroup(); // group label + input as one block
      ImGui::Text("%-*s", (int)labelWidth, label); // fixed width label
      ImGui::SameLine();
      ImGui::SetNextItemWidth(inputWidth);
      ImGui::InputFloat((std::string("##") + label).c_str(), &value, 0.01f,
                        0.1f);
      ImGui::EndGroup();

      counter++;
      if (counter % itemsPerRow != 0)
        ImGui::SameLine();
    };

    // Example usage
    addFactor("Plains", cfg.plainsFactor);
    addFactor("Low Hills", cfg.lowhillsFactor);
    addFactor("Hills", cfg.hillsFactor);
    addFactor("Mountains", cfg.mountainsFactor);
    addFactor("Peaks", cfg.peaksFactor);
    addFactor("Steep Peaks", cfg.steepPeaksFactor);
    addFactor("Cliffs", cfg.cliffsFactor);
    addFactor("Valley", cfg.valleyFactor);
    addFactor("Highlands", cfg.highlandsFactor);
    addFactor("Ocean", cfg.oceanFactor);
    addFactor("Deep Ocean", cfg.deepOceanFactor);
    addFactor("Lake", cfg.lakeFactor);
  }
}

} // namespace Fwg