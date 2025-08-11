#include "UI/ClimateUI.h"

bool ClimateUI::RenderScrollableArea(std::vector<Fwg::Gfx::Colour> &imageData) {
  // Begin the scrollable area
  ImGui::BeginChild("ScrollingRegion", ImVec2(700, 300), false,
                    ImGuiWindowFlags_HorizontalScrollbar);
  static Fwg::Gfx::Colour *selectedLabel;
  static Fwg::Gfx::Colour selectedId;

  bool updated = false;

  for (auto &input : climateInputColours.getMap()) {
    ImGui::ColorEdit3("Selected Colour", (float *)&input.second.colour,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_HDR);
    ImGui::SameLine();
    ImGui::Text(("Currently labeled as: " + input.second.rgbName).c_str());
    ImGui::SameLine();

    // Select type button
    if (ImGui::Button(
            ("Select type for " + input.second.in.toString()).c_str())) {
      selectedLabel = &input.second.out;
      selectedId = input.second.in;
      ImGui::OpenPopup("ClassificationPopup");
    }

    ImGui::SameLine();

    // Highlight Apply button if previously selected
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
      updated = true;
      highlightedInputs.erase(
          input.second.in); // Clear highlight after applying
      ImGui::PopStyleColor(3);
    }

    if (highlightedInputs.contains(input.second.in)) {
      ImGui::PopStyleColor(3);
    }
  }

  // Classification popup
  if (ImGui::BeginPopup("ClassificationPopup")) {
    ImGui::SeparatorText("Classify the type of this climate input colour");
    for (auto &internalType : allowedClimateInputs.getMap()) {
      if (ImGui::Button(internalType.second.name.c_str())) {
        *selectedLabel = internalType.second.primaryColour;
        // Only here: add to highlightedInputs
        highlightedInputs.insert(selectedId);
      }
    }
    ImGui::EndPopup();
  }

  ImGui::EndChild();
  return updated;
}

bool ClimateUI::analyzeClimateMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                                  const Fwg::Gfx::Bitmap &climateInput,
                                  int &amountClassificationsNeeded) {
  climateInputColours.clear();
  amountClassificationsNeeded = 0;
  ImGui::SameLine();
  // reset count every time we press the button
  for (auto &elem : climateInputColours.getMap()) {
    elem.second.pixels.clear();
  }
  static bool classificationNeeded = false;
  auto imageIndex = 0;
  // init and count colours
  for (auto &colour : climateInput.imageData) {
    // first count classifications
    if (climateInputColours.find(colour) &&
        climateInputColours[colour].pixels.size() == 0 &&
        !allowedClimateInputs.find(colour)) {
      amountClassificationsNeeded++;
    }

    ImVec4 inputColourVisualHelp = ImVec4(
        ((float)colour.getRed()) / 255.0f, ((float)colour.getGreen()) / 255.0f,
        ((float)colour.getBlue()) / 255.0f, 1.0f);
    // check if the input is of permitted colours or first needs to be
    // classified
    if (!allowedClimateInputs.find(colour)) {
      // this input colour is not classified yet
      classificationNeeded = true;
      // if we haven't set this colour yet
      if (!climateInputColours.find(colour)) {
        // set it
        climateInputColours.setValue(
            colour, ClimateInput{colour, colour, "Unclassified",
                                 allowedClimateInputs.at(
                                     cfg.climateColours.at("continentalhot")),
                                 inputColourVisualHelp});
        climateInputColours[colour].pixels.push_back(imageIndex);
        amountClassificationsNeeded++;
      } else {
        // otherwise increment how often this colour is here
        climateInputColours[colour].pixels.push_back(imageIndex);
      }
    }
    // this is a known and permitted colour, for which we can already create a
    // detailed climateInput
    else {
      // if we haven't set this colour yet
      if (!climateInputColours.find(colour)) {
        // set it with its valid output colour
        climateInputColours.setValue(
            colour,
            ClimateInput{colour, colour, allowedClimateInputs.at(colour).name,
                         allowedClimateInputs.at(colour),
                         inputColourVisualHelp});
        climateInputColours[colour].pixels.push_back(imageIndex);
      } else {
        // otherwise increment how often this colour is here
        climateInputColours[colour].pixels.push_back(imageIndex);
      }
    }
    imageIndex++;
  }
  return !classificationNeeded;
}

bool ClimateUI::complexTerrainMapping(Fwg::Cfg &cfg,
                                      Fwg::FastWorldGenerator &fwg,
                                      bool &analyze,
                                      int &amountClassificationsNeeded) {
  bool updated = false;
  std::vector<const char *> imageColours;
  std::vector<const char *> selectableTypes;
  for (auto &elem : climateInputColours.getMap()) {
    selectableTypes.push_back(elem.second.rgbName.c_str());
  }
  for (auto &climateInColour : climateInputColours.getMap()) {
    // only show elems with a size
    if (climateInColour.second.pixels.size()) {
      climateInColour.second.rgbName =
          (allowedClimateInputs.find(climateInColour.second.out)
               ? allowedClimateInputs[climateInColour.second.out].name
               : "Unclassified");
    }
  }
  updated = RenderScrollableArea(climateInputMap.imageData);
  if (highlightedInputs.size() > 0) {
    ImGui::Text("Before next analysis, apply all types");
    if (ImGui::Button("Apply all")) {
      for (auto &input : climateInputColours.getMap()) {
        if (highlightedInputs.contains(input.second.in)) {
          for (auto &pix : input.second.pixels) {
            climateInputMap.setColourAtIndex(pix, input.second.out);
          }
          highlightedInputs.erase(input.second.in);
        }
      }
      updated = true;
    }
  } else if (ImGui::Button("Analyze Input") || analyze) {
    analyzeClimateMap(cfg, fwg, climateInputMap, amountClassificationsNeeded);
    analyze = false;
  }
  ImGui::Value("Colours needing classification: ", amountClassificationsNeeded);
  return updated;
}
