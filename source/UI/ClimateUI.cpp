#include "UI/ClimateUI.h"

void ClimateUI::RenderScrollableArea(std::vector<Fwg::Gfx::Colour> &imageData) {
  // Begin the scrollable area
  ImGui::BeginChild("ScrollingRegion", ImVec2(700, 300), false,
                    ImGuiWindowFlags_HorizontalScrollbar);
  static Fwg::Gfx::Colour *selectedLabel;
  // Display some widgets within the scrollable area
  for (auto &input : climateInputColours.getMap()) {
    ImGui::ColorEdit3("Selected Colour", (float *)&input.second.colour,
                      ImGuiColorEditFlags_NoInputs |
                          ImGuiColorEditFlags_NoLabel |
                          ImGuiColorEditFlags_HDR);
    ImGui::SameLine();
    ImGui::Text(("Currently labeled as: " + input.second.rgbName).c_str());
    ImGui::SameLine();
    if (ImGui::Button(
            ("Select type for " + input.second.in.toString()).c_str())) {
      selectedLabel = &input.second.out;
      ImGui::OpenPopup("ClassificationPopup");
    }
    ImGui::SameLine();
    if (ImGui::Button(
            ("Apply type for " + input.second.in.toString()).c_str())) {
      for (auto &pix : input.second.pixels) {
        imageData[pix] = input.second.out;
      }
    }
  }
  if (ImGui::BeginPopup("ClassificationPopup")) {
    ImGui::SeparatorText("Classify the type of this climate input colour");
    for (auto &internalType : allowedClimateInputs.getMap()) {
      if (ImGui::Button(internalType.second.name.c_str())) {
        *selectedLabel = internalType.second.primaryColour;
      }
    }
    ImGui::EndPopup();
  }
  // End the scrollable area
  ImGui::EndChild();
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
            colour,
            ClimateInput{colour, colour, "Unclassified",
                         allowedClimateInputs.at(cfg.climateColours.at("continentalhot")),
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

void ClimateUI::complexTerrainMapping(Fwg::Cfg &cfg,
                                      Fwg::FastWorldGenerator &fwg,
                                      bool &analyze,
                                      int &amountClassificationsNeeded) {

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
  RenderScrollableArea(climateInputMap.imageData);
  if (ImGui::Button("Analyze Input. Don't forget to apply mappings before "
                    "clicking this") ||
      analyze) {
    analyzeClimateMap(cfg, fwg, climateInputMap,
                      amountClassificationsNeeded);
    analyze = false;
  }
  ImGui::Value("Colours needing classification: ", amountClassificationsNeeded);
}
