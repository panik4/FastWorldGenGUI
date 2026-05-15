#include "UI/AreaUI.h"

namespace Fwg::UI::Areas {

    void areaInputSelector(Fwg::Cfg &cfg) {
  ImGui::TextUnformatted("Area Input Mode");

  ImGui::RadioButton("Solid",
                     cfg.areaInputMode == Fwg::Areas::AreaInputType::SOLID);
  if (ImGui::IsItemClicked()) {
    cfg.areaInputMode = Fwg::Areas::AreaInputType::SOLID;
  }
  ImGui::RadioButton("Borders",
                     cfg.areaInputMode == Fwg::Areas::AreaInputType::BORDERS);
  if (ImGui::IsItemClicked()) {
    cfg.areaInputMode = Fwg::Areas::AreaInputType::BORDERS;
  }
}

int showDensityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                   UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Density")) {
    if (uiContext.tabSwitchEvent()) {
      // pre-create density map, if not existing yet, so users see the default
      // map and can then decide to overwrite (or change parameters)
      if (!fwg.climateData.habitabilities.size() &&
          fwg.climateData.climateChances.size() &&
          fwg.terrainData.landFormIds.size()) {
        fwg.genHabitability(cfg);
      }
      uiContext.imageContext.updateImage(
          0, Gfx::displayHabitability(fwg.climateData.habitabilities));
      uiContext.imageContext.updateImage(1, fwg.worldMap);
    }

    uiContext.helpContext.showHelpTextBox("Density");

    ImGui::SeparatorText("Province & State Density Map");
    ImGui::TextWrapped("This map determines the density of provinces and "
                       "states based on climate habitability.");

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData)});

    if (guard.ready()) {
      if (UI::Elements::ImportantStepButton(
              "Generate Density from Climate Data", ImVec2(250, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              fwg.genHabitability(cfg);
              uiContext.imageContext.resetTexture(0);
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        fwg.loadHabitability(
            cfg, Fwg::IO::Reader::readGenericImage(uiContext.draggedFile, cfg));
        uiContext.imageContext.resetTexture(0);
        uiContext.triggeredDrag = false;
        uiContext.imageContext.resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
void showSuperSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                         UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("SuperSegments")) {
    if (uiContext.tabSwitchEvent()) {
      if (fwg.worldMap.size()) {
        uiContext.imageContext.updateImage(
            0, Fwg::Gfx::Segments::displaySuperSegments(
                   fwg.areaData.superSegments));
        uiContext.imageContext.updateImage(1, fwg.errorMap);
      }
    }
    uiContext.helpContext.showHelpTextBox("SuperSegments");

    ImGui::SeparatorText("SuperSegment Generation");
    ImGui::TextWrapped("SuperSegments are large landmasses and ocean regions.");

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData)});

    if (guard.ready()) {
      if (UI::Elements::Button("Generate Template Images", false,
                               ImVec2(220, 0))) {
        Fwg::Gfx::Land::displaySimpleLandType(fwg.terrainData, fwg.areaData,
                                              fwg.worldMap, true, false, false);
      }

      ImGui::SameLine();

      if (UI::Elements::ImportantStepButton("Generate SuperSegments",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg,  &uiContext]() {
              fwg.genSuperSegments(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              uiContext.triggeredDrag = false;
              auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              if (cfg.areaInputMode == Fwg::Areas::AreaInputType::SOLID) {
                fwg.loadSuperSegments(
                    cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                             uiContext.draggedFile, cfg, evaluationAreas));
              } else {
                auto image = Fwg::IO::Reader::readGenericImage(
                    uiContext.draggedFile, cfg);
                Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                    image, evaluationAreas);
                Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
                fwg.loadSuperSegments(cfg, image);
              }
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
    }

    ImGui::EndTabItem();
  }
}
void showSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext) {
  static auto lastEvent = std::chrono::high_resolution_clock::now();

  if (UI::Elements::BeginSubTabItem("Segments")) {
    // check if 50ms have passed since last event
    auto now = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEvent);
    if (uiContext.tabSwitchEvent() || duration.count() > 50) {
      lastEvent = now;
      uiContext.imageContext.updateImage(0, fwg.segmentMap);
      uiContext.imageContext.updateImage(1, fwg.errorMap);
    }
    uiContext.helpContext.showHelpTextBox("Segments");

    ImGui::SeparatorText("Segment Configuration");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      grid.AddInputDouble("Cost Influence", &cfg.segmentCostInfluence, 0.0,
                          10.0);
      grid.AddInputDouble("Distance Influence", &cfg.segmentDistanceInfluence,
                          0.0, 10.0);

      if (grid.AddInputInt("Target Land Regions", &cfg.targetLandRegionAmount,
                           1, 1000)) {
        cfg.autoLandRegionParams = true;
        cfg.calcAreaParameters();
      }

      if (grid.AddInputInt("Target Sea Regions", &cfg.targetSeaRegionAmount, 1,
                           1000)) {
        cfg.autoSeaRegionParams = true;
        cfg.calcAreaParameters();
      }
    }

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::superSegments(fwg.areaData)});

    if (guard.ready()) {
      ImGui::SeparatorText("Segment Statistics");

      {
        UI::Elements::GridLayout grid(3, 150.0f, 12.0f);
        grid.AddText("Land Segments", "%d", (int)fwg.areaData.landSegments);
        grid.AddText("Sea Segments", "%d", (int)fwg.areaData.seaSegments);
        grid.AddText("Lake Segments", "%d", (int)fwg.areaData.lakeSegments);
      }

      ImGui::Spacing();

      if (UI::Elements::Button("Generate Template Images", false,
                               ImVec2(220, 0))) {
        Fwg::Gfx::Land::displaySimpleLandType(fwg.terrainData, fwg.areaData,
                                              fwg.worldMap, false, true, false);
      }

      ImGui::SameLine();

      if (UI::Elements::ImportantStepButton("Generate Segments",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              uiContext.generationContext.modifiedAreas = true;
              fwg.genSegments(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              uiContext.triggeredDrag = false;
              const auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              if (cfg.areaInputMode == Fwg::Areas::AreaInputType::SOLID) {
                fwg.loadSegments(
                    cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                             uiContext.draggedFile, cfg, evaluationAreas));
              } else {
                auto image = Fwg::IO::Reader::readGenericImage(
                    uiContext.draggedFile, cfg);
                Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                    image, evaluationAreas);
                Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
                fwg.loadSegments(cfg, image);
              }
              fwg.segmentMap =
                  Fwg::Gfx::Segments::displaySegments(fwg.areaData.segments);
              uiContext.imageContext.resetTexture();
              uiContext.generationContext.modifiedAreas = true;
              return true;
            });
      }
    }

    ImGui::EndTabItem();
  }
}
int showProvincesTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext) {
  static auto lastEvent = std::chrono::high_resolution_clock::now();

  if (UI::Elements::BeginSubTabItem("Provinces")) {
    // check if 50ms have passed since last event
    auto now = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEvent);
    if (uiContext.tabSwitchEvent() || duration.count() > 50) {
      lastEvent = now;
      uiContext.imageContext.updateImage(0, fwg.provinceMap);
      uiContext.imageContext.updateImage(1, fwg.segmentMap);
    }
    uiContext.helpContext.showHelpTextBox("Provinces");

    ImGui::SeparatorText("Province Configuration");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      grid.AddInputDouble("Land Factor", &cfg.landProvFactor, 0.0, 10.0);
      grid.AddInputDouble("Sea Factor", &cfg.seaProvFactor, 0.0, 10.0);
      grid.AddInputDouble("Density Effects", &cfg.provinceDensityEffects, 0.0,
                          1.0);
      grid.AddInputInt("Min Province Size", &cfg.minProvSize, 9, 1000);
      grid.AddInputInt("Max Province Count", &cfg.maxProvAmount, 100, 100000);
      grid.AddText("Current Provinces", "%d",
                   (int)fwg.areaData.provinces.size());
    }

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::superSegments(fwg.areaData),
         UI::PrerequisiteChecker::segments(fwg.areaData)});

    if (guard.ready()) {
      if (UI::Elements::Button("Generate Template Images", false,
                               ImVec2(220, 0))) {
        Fwg::Gfx::Land::displaySimpleLandType(fwg.terrainData, fwg.areaData,
                                              fwg.worldMap, false, false, true);
      }

      ImGui::SameLine();

      if (UI::Elements::ImportantStepButton("Generate Provinces",
                                            ImVec2(220, 0))) {
        uiContext.generationContext.modifiedAreas = true;
        cfg.calcAreaParameters();
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              if (!fwg.genProvinces()) {
                return false;
              }
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              uiContext.generationContext.modifiedAreas = true;
              uiContext.triggeredDrag = false;
              auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              if (cfg.areaInputMode == Fwg::Areas::AreaInputType::SOLID) {
                fwg.loadProvinces(
                    cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                             uiContext.draggedFile, cfg, evaluationAreas));
              } else {
                auto image = Fwg::IO::Reader::readGenericImage(
                    uiContext.draggedFile, cfg);
                Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                    image, evaluationAreas);
                Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
                fwg.loadProvinces(cfg, image);
              }
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
int showRegionTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                  UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Regions")) {
    uiContext.tabSwitchEvent();
    uiContext.helpContext.showHelpTextBox("Regions");

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::provinceMap(fwg.provinceMap)});

    if (guard.ready()) {
      ImGui::SeparatorText("Generate a region map");

      if (ImGui::Button("Generate Region Map from Segments and Provinces")) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              fwg.genRegions(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
      ImGui::Text("The map has %i regions",
                  static_cast<int>(fwg.areaData.regions.size()));

      if (uiContext.triggeredDrag) {
        if (fwg.provinceMap.initialised()) {
          try {
            // fwg.loadRegions(cfg, uiContext.draggedFile);
            Fwg::Utils::Logging::logLine(
                "If you want to load regions, do it in the segments tab.");
          } catch (std::exception &e) {
            Fwg::Utils::Logging::logLine(
                "Couldn't load regions, fix input or try again");
            fwg.regionMap =
                Fwg::IO::Reader::readGenericImage(uiContext.draggedFile, cfg);
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

int showContinentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                            UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Continents")) {
    if (uiContext.tabSwitchEvent() && fwg.areaData.provinces.size() &&
        fwg.areaData.regions.size()) {
      uiContext.imageContext.updateImage(
          0, Fwg::Gfx::simpleContinents(fwg.areaData.continents,
                                        fwg.areaData.seaBodies));
      uiContext.imageContext.updateImage(
          1, Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap));
    }
    uiContext.helpContext.showHelpTextBox("Continents");

    ImGui::SeparatorText("Continent Configuration");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);
      grid.AddInputInt("Max Continents", &cfg.maxAmountOfContinents, 1, 50);
      grid.AddText("Current Continents", "%d",
                   (int)fwg.areaData.continents.size());
    }

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::superSegments(fwg.areaData),
         UI::PrerequisiteChecker::segments(fwg.areaData),
         UI::PrerequisiteChecker::provinces(fwg.areaData)});

    if (guard.ready()) {
      if (UI::Elements::ImportantStepButton("Generate Continents",
                                            ImVec2(220, 0)) ||
          (fwg.areaData.continents.empty() &&
           !uiContext.asyncContext.computationRunning)) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              uiContext.generationContext.modifiedAreas = true;
              fwg.genContinents(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, &uiContext]() {
              uiContext.generationContext.modifiedAreas = true;
              auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              fwg.loadContinents(
                  cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                           uiContext.draggedFile, cfg, evaluationAreas));
              uiContext.triggeredDrag = false;
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

} // namespace Fwg::UI::Areas