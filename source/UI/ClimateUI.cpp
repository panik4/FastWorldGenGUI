#include "UI/ClimateUI.h"

namespace Fwg::UI::Climate {
namespace Input {
bool RenderScrollableClimateInput(std::vector<Fwg::Gfx::Colour> &imageData,
                                  UIContext &uiContext) {
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
                                     cfg.climateColours.at("continentalhot")),
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

bool complexTerrainMapping(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
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
} // namespace Input

int showTemperatureMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                       UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Temperature")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, Fwg::Gfx::Climate::displayTemperature(fwg.climateData));
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
    }
    uiContext.helpContext.showHelpTextBox("Temperature");

    ImGui::SeparatorText("Temperature Map Generation");

    static bool applyAltitudeEffect = false;
    ImGui::Checkbox("Apply elevation effect when loading",
                    &applyAltitudeEffect);

    ImGui::Spacing();

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData)});

    if (guard.ready()) {
      if (UI::Elements::ImportantStepButton("Generate Temperature Map",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              fwg.genTemperatures(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        fwg.loadTemperatures(cfg, uiContext.draggedFile, applyAltitudeEffect);
        uiContext.triggeredDrag = false;
        uiContext.imageContext.resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int showHumidityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Humidity")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, Fwg::Gfx::Climate::displayHumidity(fwg.climateData));
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
    }
    uiContext.helpContext.showHelpTextBox("Humidity");

    ImGui::SeparatorText("Humidity Map Generation");

    static bool applyElevationEffect = false;
    ImGui::Checkbox("Apply elevation effect when loading",
                    &applyElevationEffect);

    ImGui::Spacing();

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData)});

    if (guard.ready()) {
      if (UI::Elements::ImportantStepButton("Generate Humidity Map",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              fwg.genHumidity(cfg);
              uiContext.imageContext.resetTexture(0);
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        fwg.loadHumidity(
            cfg, Fwg::IO::Reader::readGenericImage(uiContext.draggedFile, cfg),
            applyElevationEffect);
        uiContext.generationContext.redoHumidity = false;
        uiContext.triggeredDrag = false;
        uiContext.imageContext.resetTexture();
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
      uiContext.imageContext.updateImage(
          0, Gfx::riverMap(fwg.terrainData.detailedHeightMap,
                           fwg.climateData.rivers));
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
    }
    uiContext.helpContext.showHelpTextBox("Rivers");

    ImGui::SeparatorText("River Configuration");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);
      grid.AddInputDouble("River Multiplier", &cfg.riverFactor, 0.0, 10.0);
      grid.AddText("River Count", "%d", (int)fwg.climateData.rivers.size());
    }

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData),
         UI::PrerequisiteChecker::humidity(fwg.climateData)});

    if (guard.ready()) {
      if (UI::Elements::ImportantStepButton("Generate River Map",
                                            ImVec2(200, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              fwg.genRivers(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        fwg.loadRivers(
            cfg, Fwg::IO::Reader::readGenericImage(uiContext.draggedFile, cfg));
        uiContext.imageContext.resetTexture();
        uiContext.triggeredDrag = false;
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
int showClimateTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                   UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Climate")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, Fwg::Gfx::Climate::displayClimate(fwg.climateData, false));
      uiContext.imageContext.updateImage(1, fwg.worldMap);
    }

    uiContext.helpContext.showHelpTextBox("Climate Gen");
    ImGui::SeparatorText("Generate climate map or drop it in");

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData),
         UI::PrerequisiteChecker::humidity(fwg.climateData),
         UI::PrerequisiteChecker::temperature(fwg.climateData)});

    if (guard.ready()) {
      if (!cfg.fantasyClimate &&
          ImGui::Button(
              "Generate Climate Zones from Temperature and Heightmap Data")) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              if (uiContext.generationContext.redoHumidity) {
                fwg.genTemperatures(cfg);
                fwg.genHumidity(cfg);
                uiContext.generationContext.redoHumidity = false;
              }
              fwg.genClimate(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      } else if (cfg.fantasyClimate &&
                 ImGui::Button("Generate completely random fantasy climate")) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              fwg.genTemperatures(cfg);
              fwg.genHumidity(cfg);
              uiContext.generationContext.redoHumidity = false;
              fwg.genClimate(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        if (uiContext.climateUI.climateInputMap.initialised()) {
          uiContext.asyncContext.computationFutureBool =
              uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
                fwg.loadClimate(cfg, uiContext.climateUI.climateInputMap);
                fwg.genWorldMap(cfg);
                uiContext.imageContext.resetTexture();
                return true;
              });
        } else {
          int amountClassificationsNeeded;
          auto climateInput =
              Fwg::IO::Reader::readGenericImage(uiContext.draggedFile, cfg);
          // load a valid map if no classificationsNeeded
          if (Input::analyzeClimateMap(cfg, fwg, climateInput, uiContext)) {
            fwg.loadClimate(cfg, climateInput);
            uiContext.imageContext.resetTexture();
          } else {
            Fwg::Utils::Logging::logLine(
                "You are trying to load a climate input that has "
                "incompatible "
                "colours. If you want to use a complex climate map as input, "
                "please use the Climate Input tab label the climate zones. "
                "The "
                "resulting map will be used as "
                "climate input here automatically.");
          }
        }
        uiContext.triggeredDrag = false;
        uiContext.imageContext.resetTexture();
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
          0, Fwg::Gfx::Climate::displayClimate(fwg.climateData, true));
      uiContext.imageContext.updateImage(
          1, Fwg::Gfx::Climate::displayTreeDensity(fwg.climateData));
    }
    uiContext.helpContext.showHelpTextBox("Forests");

    ImGui::SeparatorText("Forest Density Configuration");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      grid.AddInputDouble("Boreal Density", &cfg.borealDensity, 0.0, 1.0);
      grid.AddInputDouble("Temperate Needle", &cfg.temperateNeedleDensity, 0.0,
                          1.0);
      grid.AddInputDouble("Temperate Mixed", &cfg.temperateMixedDensity, 0.0,
                          1.0);
      grid.AddInputDouble("Sparse Density", &cfg.sparseDensity, 0.0, 1.0);
      grid.AddInputDouble("Tropical Dry", &cfg.tropicalDryDensity, 0.0, 1.0);
      grid.AddInputDouble("Tropical Moist", &cfg.tropicalMoistDensity, 0.0,
                          1.0);
    }

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData),
         UI::PrerequisiteChecker::humidity(fwg.climateData),
         UI::PrerequisiteChecker::temperature(fwg.climateData)});

    if (guard.ready()) {
      if (UI::Elements::ImportantStepButton("Generate Forest Map",
                                            ImVec2(200, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              fwg.genForests(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        fwg.loadForests(cfg, uiContext.draggedFile);
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
      uiContext.asyncContext.computationFutureBool =
          uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
            // fwg.genWasteland(cfg);
            uiContext.imageContext.resetTexture();
            return true;
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