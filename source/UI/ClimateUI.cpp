#include "UI/ClimateUI.h"

bool ClimateUI::RenderScrollableClimateInput(
    std::vector<Fwg::Gfx::Colour> &imageData) {
  ImGui::BeginChild("ScrollingRegion",
                    ImVec2(ImGui::GetContentRegionAvail().x,
                           ImGui::GetContentRegionAvail().y * 0.8f),
                    false, ImGuiWindowFlags_HorizontalScrollbar);

  // --- Selection state ---
  static bool singularEdit = false;
  static std::unordered_set<Fwg::Gfx::Colour> selectedInputs;
  static std::optional<Fwg::Gfx::Colour> lastClickedInput;

  bool updated = false;

  // --- Build sorted order for deterministic UX ---
  std::vector<Fwg::Gfx::Colour> colourOrder;
  for (auto &entry : climateInputColours.getMap()) {
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
        if (climateInputColours.getMap().contains(selId)) {
          auto &entry = climateInputColours.getMap().at(selId);
          for (auto &pix : entry.pixels) {
            imageData[pix] = entry.out;
          }
        }
      }
      highlightedInputs.clear();
      updated = true;
      selectedInputs.clear();
    }
  }

  // --- Iterate through sorted entries ---
  for (int i = 0; i < colourOrder.size(); ++i) {
    auto &id = colourOrder[i];
    auto &entry = climateInputColours.getMap().at(id);

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
    bool isHighlighted = highlightedInputs.contains(entry.in);
    if (isHighlighted) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0.2f, 0.2f, 1));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 0.4f, 0.4f, 1));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 0.3f, 0.3f, 1));
    }

    // --- Apply single ---
    if (ImGui::Button(("Apply type for " + entry.in.toString()).c_str())) {
      for (auto &pix : entry.pixels)
        imageData[pix] = entry.out;

      highlightedInputs.erase(entry.in);
      updated = true;
    }

    if (isHighlighted)
      ImGui::PopStyleColor(3);
  }

  // --- Classification Popup ---
  if (ImGui::BeginPopup("ClimateClassificationPopup")) {
    ImGui::SeparatorText("Classify selected climate input colours");

    for (auto &internalType : allowedClimateInputs.getMap()) {
      if (ImGui::Button(internalType.second.name.c_str())) {
        for (const auto &selId : selectedInputs) {
          auto &entry = climateInputColours.getMap().at(selId);
          entry.out = internalType.second.primaryColour;
          highlightedInputs.insert(selId);
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

bool ClimateUI::analyzeClimateMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                                  const Fwg::Gfx::Image &climateInput,
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
    if (climateInputColours.contains(colour) &&
        climateInputColours[colour].pixels.size() == 0 &&
        !allowedClimateInputs.contains(colour)) {
      amountClassificationsNeeded++;
    }

    ImVec4 inputColourVisualHelp = ImVec4(
        ((float)colour.getRed()) / 255.0f, ((float)colour.getGreen()) / 255.0f,
        ((float)colour.getBlue()) / 255.0f, 1.0f);
    // check if the input is of permitted colours or first needs to be
    // classified
    if (!allowedClimateInputs.contains(colour)) {
      // this input colour is not classified yet
      classificationNeeded = true;
      // if we haven't set this colour yet
      if (!climateInputColours.contains(colour)) {
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
      if (!climateInputColours.contains(colour)) {
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

  // Update names of entries based on classification
  for (auto &cl : climateInputColours.getMap()) {
    if (cl.second.pixels.size()) {
      cl.second.rgbName = (allowedClimateInputs.contains(cl.second.out)
                               ? allowedClimateInputs[cl.second.out].name
                               : "Unclassified");
    }
  }

  updated |= RenderScrollableClimateInput(climateInputMap.imageData);

  // "Apply all" option
  if (!highlightedInputs.empty()) {
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
  }
  // Re-analyze
  else if (ImGui::Button("Analyze Input") || analyze) {
    analyzeClimateMap(cfg, fwg, climateInputMap, amountClassificationsNeeded);
    analyze = false;
  }

  ImGui::Value("Colours needing classification: ", amountClassificationsNeeded);
  return updated;
}
