#include "UI/landUI.h"

namespace Fwg {
LandUI::LandUI() {}
LandUI::LandUI(std::shared_ptr<UIUtils> uiUtils) { this->uiUtils = uiUtils; }

void LandUI::RenderScrollableLandInput(
    std::vector<Fwg::Gfx::Colour> &imageData) {
  // Begin the scrollable area
  ImGui::BeginChild("ScrollingRegion",
                    ImVec2(ImGui::GetContentRegionAvail().x,
                           ImGui::GetContentRegionAvail().y * 0.8),
                    false, ImGuiWindowFlags_HorizontalScrollbar);
  static Fwg::Gfx::Colour *selectedLabel;
  static Fwg::Gfx::Colour selectedId;

  for (auto &input : landInputColours.getMap()) {
    ImGui::ColorEdit3("Selected Colour", (float *)&input.second.colour,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_HDR);
    ImGui::SameLine();
    ImGui::Text(("Currently labeled as: " + input.second.rgbName).c_str());
    ImGui::SameLine();

    // Select button
    if (ImGui::Button(
            ("Select type for " + input.second.in.toString()).c_str())) {
      selectedLabel = &input.second.out;
      selectedId = input.second.in;
      ImGui::OpenPopup("ClassificationPopup");
    }

    ImGui::SameLine();

    // Apply button - highlight if it was previously selected
    if (highlightedInputs.contains(input.second.in)) {
      ImGui::PushStyleColor(ImGuiCol_Button,
                            ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); // Red
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                            ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                            ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
    }

    if (ImGui::Button(
            ("Apply type for " + input.second.in.toString()).c_str())) {
      for (auto &pix : input.second.pixels) {
        imageData[pix] = input.second.out;
      }
      uiUtils->resetTexture();
      highlightedInputs.erase(
          input.second.in); // Remove highlight after applying
      ImGui::PopStyleColor(3);
    }

    if (highlightedInputs.contains(input.second.in)) {
      ImGui::PopStyleColor(3);
    }
  }

  if (ImGui::BeginPopup("ClassificationPopup")) {
    ImGui::SeparatorText("Classify the type of this land input colour");
    for (auto &internalType : allowedLandInputs.getMap()) {
      if (ImGui::Button(internalType.second.name.c_str())) {
        *selectedLabel = internalType.second.colour;
        highlightedInputs.insert(selectedId);
      }
    }
    ImGui::EndPopup();
  }
  // End the scrollable area
  ImGui::EndChild();
}

bool LandUI::analyzeLandMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                            const Fwg::Gfx::Bitmap &landInput,
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
    if (landInputColours.find(colour) &&
        landInputColours[colour].pixels.size() == 0 &&
        !allowedLandInputs.find(colour)) {
      amountClassificationsNeeded++;
    }

    ImVec4 inputColourVisualHelp = ImVec4(
        ((float)colour.getRed()) / 255.0f, ((float)colour.getGreen()) / 255.0f,
        ((float)colour.getBlue()) / 255.0f, 1.0f);
    // check if the input is of permitted colours or first needs to be
    // classified
    if (!allowedLandInputs.find(colour)) {
      // this input colour is not classified yet
      classificationNeeded = true;
      // if we haven't set this colour yet
      if (!landInputColours.find(colour)) {
        // set it
        landInputColours.setValue(
            colour, ElevationInput{colour, colour, "Unclassified",
                                   fwg.terrainData.elevationTypes[0],
                                   inputColourVisualHelp});
        landInputColours[colour].pixels.push_back(imageIndex);
        amountClassificationsNeeded++;
      } else {
        // otherwise increment how often this colour is here
        landInputColours[colour].pixels.push_back(imageIndex);
      }
    } else {
      // if we haven't set this colour yet
      if (!landInputColours.find(colour)) {
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
                                bool &analyze,
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
          (allowedLandInputs.find(landInColour.second.out)
               ? allowedLandInputs[landInColour.second.out].name
               : "Unclassified");
    }
  }
  RenderScrollableLandInput(landInput.imageData);
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
  } else if (ImGui::Button("Analyze Input") || analyze) {
    // always reload the classified map from disk
    Fwg::Gfx::Bmp::save(landInput, cfg.mapsPath + "//classifiedLandInput.bmp");
    analyzeLandMap(cfg, fwg, landInput, amountClassificationsNeeded);
    analyze = false;
  }
}

void LandUI::triggeredLandInput(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                                const std::string &draggedFile,
                                bool classifyInput) {
  // if (fwg.heightMap.initialised() && !classifyInput) {
  //   ImGui::OpenPopup("Drag info");
  // } else
  if (classifyInput) {
    fwg.resetData();
    fwg.configure(cfg);
    // don't immediately generate from the input, instead allow to manually
    // classify all present colours
    landInput = Fwg::IO::Reader::readGenericImage(draggedFile, cfg, false);
    // if (!cfg.validateResolution(landInput.width(), landInput.height())) {
    //   Utils::Logging::logLine("Invalid resolution for land input image");
    //   landInput.clear();
    // }
    //  save the landmap to classifiedLandInput.bmp
    Fwg::Gfx::Bmp::save(landInput, cfg.mapsPath + "//classifiedLandInput.bmp");
  } else {
    fwg.resetData();
    fwg.genHeightFromInput(cfg, draggedFile);
    fwg.genLand();
    // buffer this path in case we want to regen heightmap from this
    loadedTerrainFile = draggedFile;
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
  if (cfg.complexLandInput) {
    ImGui::PushItemWidth(100.0f);
    ImGui::SeparatorText("Land Elevation Factors");
    ImGui::InputFloat("Plains Factor", &cfg.plainsFactor, 0.01f, 0.1f);
    ImGui::InputFloat("Low Hills Factor", &cfg.lowhillsFactor, 0.01f, 1.0f);
    ImGui::InputFloat("Hills Factor", &cfg.hillsFactor, 0.02f, 1.0f);
    ImGui::InputFloat("Mountains Factor", &cfg.mountainsFactor, 0.05f, 1.0f);
    ImGui::InputFloat("Peaks Factor", &cfg.peaksFactor, 0.05f, 1.0f);
    ImGui::InputFloat("Steep Peaks Factor", &cfg.steepPeaksFactor, 0.1f, 1.0f);
    ImGui::InputFloat("Cliffs Factor", &cfg.cliffsFactor, 0.05f, 1.0f);
    ImGui::InputFloat("Valley Factor", &cfg.valleyFactor, 0.01f, 1.0f);
    ImGui::InputFloat("Highlands Factor", &cfg.highlandsFactor, 0.01f, 1.0f);
    ImGui::InputFloat("Ocean Factor", &cfg.oceanFactor, 0.1f, 1.0f);
    ImGui::InputFloat("Deep Ocean Factor", &cfg.deepOceanFactor, 0.1f, 1.0f);
    ImGui::InputFloat("Lake Factor", &cfg.lakeFactor, 0.1f, 1.0f);
    ImGui::PopItemWidth();
  }
}

} // namespace Fwg