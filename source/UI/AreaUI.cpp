#include "UI/AreaUI.h"

namespace Fwg::UI::Areas {

void areaInputSelector(Fwg::Cfg &cfg) {
  ImGui::TextUnformatted("Area Input Mode");

  ImGui::RadioButton("Solid", cfg.areaConfig.common.inputMode ==
                                  Fwg::Areas::AreaInputType::SOLID);
  if (ImGui::IsItemClicked()) {
    cfg.areaConfig.common.inputMode = Fwg::Areas::AreaInputType::SOLID;
  }
  ImGui::RadioButton("Borders", cfg.areaConfig.common.inputMode ==
                                    Fwg::Areas::AreaInputType::BORDERS);
  if (ImGui::IsItemClicked()) {
    cfg.areaConfig.common.inputMode = Fwg::Areas::AreaInputType::BORDERS;
  }
}

int showDensityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                   UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Density")) {
    if (uiContext.tabSwitchEvent()) {
      const auto area = static_cast<size_t>(cfg.processingArea);
      // pre-create density map, if not existing yet, so users see the default
      // map and can then decide to overwrite (or change parameters)
      if (fwg.climateData.habitabilities.empty() && cfg.processingArea > 0 &&
          fwg.climateData.climateChances.size() == area &&
          fwg.terrainData.landFormIds.size() == area &&
          fwg.terrainData.distance.size() == area &&
          fwg.terrainData.inclination.size() == area &&
          fwg.terrainData.altitudes.size() == area) {
        fwg.genHabitability(cfg);
      }
      uiContext.imageContext.updateImage(
          0, cfg.processingArea > 0 &&
                     fwg.climateData.habitabilities.size() == area
                 ? Gfx::displayHabitability(fwg.climateData.habitabilities,
                                            cfg.climateGenerationContext())
                 : Fwg::Gfx::Image());
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
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genHabitability(cfg); });
      }

      if (uiContext.triggeredDrag) {
        fwg.loadHabitability(
            cfg, Fwg::IO::Reader::readGenericImage(uiContext.draggedFile,
                                                   cfg.imageReadContext()));
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
      if (cfg.processingArea > 0 &&
          fwg.worldMap.size() == static_cast<size_t>(cfg.processingArea) &&
          !fwg.areaData.superSegments.empty()) {
        uiContext.imageContext.updateImage(
            0, Fwg::Gfx::Segments::displaySuperSegments(
                   fwg.areaData.superSegments, fwg.areaGenerationContext(cfg)));
        uiContext.imageContext.updateImage(1, fwg.errorMap);
      } else {
        uiContext.imageContext.updateImage(0, Fwg::Gfx::Image());
        uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
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
                                              fwg.worldMap, true, false, false,
                                              cfg.visualizationContext());
      }

      ImGui::SameLine();

      if (UI::Elements::ImportantStepButton("Generate SuperSegments",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genSuperSegments(cfg); });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg, &uiContext]() {
              auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              if (cfg.areaConfig.common.inputMode ==
                  Fwg::Areas::AreaInputType::SOLID) {
                auto image = Fwg::IO::Reader::readGenericImageWithBorders(
                    uiContext.draggedFile, cfg.imageReadContext(),
                    evaluationAreas);
                if (image.size() == 0) {
                  return false;
                }
                if (!fwg.loadSuperSegments(cfg, image))
                  return false;
              } else {
                auto image = Fwg::IO::Reader::readGenericImage(
                    uiContext.draggedFile, cfg.imageReadContext());
                if (image.size() == 0) {
                  return false;
                }
                Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                    image, evaluationAreas);
                Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
                if (!fwg.loadSuperSegments(cfg, image))
                  return false;
              }
              return true;
            },
            [&uiContext](TaskResult::Outcome) {
              uiContext.triggeredDrag = false;
            });
      }
    }

    ImGui::EndTabItem();
  }
}
void showSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Segments")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(0, fwg.segmentMap);
      uiContext.imageContext.updateImage(1, fwg.errorMap);
    }
    uiContext.helpContext.showHelpTextBox("Segments");

    ImGui::SeparatorText("Segment Configuration");

    {
      UI::Elements::GridLayout grid(2, uiContext.scaled(200.0f),
                                    uiContext.scaled(12.0f));

      grid.AddInputDouble("Cost Influence",
                          &cfg.areaConfig.segments.costInfluence, 0.0, 10.0);
      grid.AddInputDouble("Distance Influence",
                          &cfg.areaConfig.segments.distanceInfluence, 0.0,
                          10.0);
      grid.AddInputInt("River Border Fix Threshold",
                       &cfg.areaConfig.segments.removeChunkThreshold, 1, 10000);

      if (grid.AddInputInt("Target Land Regions",
                           &cfg.areaConfig.regions.targetLandAmount, 1,
                           10000)) {
        cfg.areaConfig.regions.autoLandParameters = true;
      }

      if (grid.AddInputInt("Target Sea Regions",
                           &cfg.areaConfig.regions.targetSeaAmount, 1, 10000)) {
        cfg.areaConfig.regions.autoSeaParameters = true;
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
        UI::Elements::GridLayout grid(3, uiContext.scaled(150.0f),
                                      uiContext.scaled(12.0f));
        grid.AddText("Land Segments", "%d", (int)fwg.areaData.landSegments);
        grid.AddText("Sea Segments", "%d", (int)fwg.areaData.seaSegments);
        grid.AddText("Lake Segments", "%d", (int)fwg.areaData.lakeSegments);
      }

      ImGui::Spacing();

      if (UI::Elements::Button("Generate Template Images", false,
                               ImVec2(220, 0))) {
        Fwg::Gfx::Land::displaySimpleLandType(fwg.terrainData, fwg.areaData,
                                              fwg.worldMap, false, true, false,
                                              cfg.visualizationContext());
      }

      ImGui::SameLine();

      if (UI::Elements::ImportantStepButton("Generate Segments",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genSegments(cfg); },
            [&uiContext](TaskResult::Outcome) {
              uiContext.generationContext.modifiedAreas = true;
            });
      }

      ImGui::SameLine();
      const bool canFixRiverBorders =
          !fwg.areaData.segments.empty() && !fwg.climateData.rivers.empty();
      ImGui::BeginDisabled(!canFixRiverBorders);
      const bool fixRiverBordersClicked = UI::Elements::ImportantStepButton(
          "Fix River Borders", ImVec2(220, 0));
      ImGui::EndDisabled();
      if (fixRiverBordersClicked) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() {
              fwg.fixSegmentBordersAtRivers(cfg);
              return true;
            },
            [&uiContext](TaskResult::Outcome outcome) {
              if (outcome == TaskResult::Outcome::Success)
                uiContext.generationContext.modifiedAreas = true;
            });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg, &uiContext]() {
              const auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              if (cfg.areaConfig.common.inputMode ==
                  Fwg::Areas::AreaInputType::SOLID) {
                if (!fwg.loadSegments(
                        cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                                 uiContext.draggedFile, cfg.imageReadContext(),
                                 evaluationAreas)))
                  return false;
              } else {
                auto image = Fwg::IO::Reader::readGenericImage(
                    uiContext.draggedFile, cfg.imageReadContext());
                Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                    image, evaluationAreas);
                Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
                if (!fwg.loadSegments(cfg, image))
                  return false;
              }
              fwg.segmentMap = Fwg::Gfx::Segments::displaySegments(
                  fwg.areaData.segments, fwg.areaGenerationContext(cfg));
              return true;
            },
            [&uiContext](TaskResult::Outcome) {
              uiContext.triggeredDrag = false;
              uiContext.generationContext.modifiedAreas = true;
            });
      }
    }

    ImGui::EndTabItem();
  }
}
int showProvincesTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Provinces")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(0, fwg.provinceMap);
      uiContext.imageContext.updateImage(1, fwg.segmentMap);
    }
    uiContext.helpContext.showHelpTextBox("Provinces");

    ImGui::SeparatorText("Province Configuration");

    {
      UI::Elements::GridLayout grid(2, uiContext.scaled(200.0f),
                                    uiContext.scaled(12.0f));

      grid.AddInputDouble("Land Factor", &cfg.areaConfig.provinces.landFactor,
                          0.0, 10.0);
      grid.AddInputDouble("Sea Factor", &cfg.areaConfig.provinces.seaFactor,
                          0.0, 10.0);
      grid.AddInputDouble("Density Effects",
                          &cfg.areaConfig.provinces.densityEffects, 0.0, 1.0);
      grid.AddInputInt("Min Province Size", &cfg.areaConfig.provinces.minSize,
                       9, 1000);
      grid.AddInputInt("Max Province Count",
                       &cfg.areaConfig.provinces.maxAmount, 100, 100000);
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
                                              fwg.worldMap, false, false, true,
                                              cfg.visualizationContext());
      }

      ImGui::SameLine();

      if (UI::Elements::ImportantStepButton("Generate Provinces",
                                            ImVec2(220, 0))) {
        uiContext.generationContext.modifiedAreas = true;
        uiContext.asyncContext.runAsync([&fwg, &cfg]() {
          if (!fwg.genProvinces(cfg)) {
            return false;
          }
          return true;
        });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg, &uiContext]() {
              auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              if (cfg.areaConfig.common.inputMode ==
                  Fwg::Areas::AreaInputType::SOLID) {
                if (!fwg.loadProvinces(
                        cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                                 uiContext.draggedFile, cfg.imageReadContext(),
                                 evaluationAreas)))
                  return false;
              } else {
                auto image = Fwg::IO::Reader::readGenericImage(
                    uiContext.draggedFile, cfg.imageReadContext());
                Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                    image, evaluationAreas);
                Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
                if (!fwg.loadProvinces(cfg, image))
                  return false;
              }
              return true;
            },
            [&uiContext](TaskResult::Outcome) {
              uiContext.triggeredDrag = false;
              uiContext.generationContext.modifiedAreas = true;
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
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genRegions(cfg); });
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
            fwg.regionMap = Fwg::IO::Reader::readGenericImage(
                uiContext.draggedFile, cfg.imageReadContext());
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
    if (uiContext.tabSwitchEvent()) {
      if (cfg.processingArea > 0 && !fwg.areaData.provinces.empty() &&
          !fwg.areaData.regions.empty() &&
          fwg.terrainData.detailedHeightMap.size() ==
              static_cast<size_t>(cfg.processingArea)) {
        uiContext.imageContext.updateImage(
            0, Fwg::Gfx::simpleContinents(fwg.areaData.continents,
                                          fwg.areaData.seaBodies,
                                          fwg.areaGenerationContext(cfg)));
        uiContext.imageContext.updateImage(
            1, Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap,
                                          cfg.visualizationContext()));
      } else {
        uiContext.imageContext.updateImage(0, Fwg::Gfx::Image());
        uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
      }
    }
    uiContext.helpContext.showHelpTextBox("Continents");

    ImGui::SeparatorText("Continent Configuration");

    {
      UI::Elements::GridLayout grid(2, uiContext.scaled(200.0f),
                                    uiContext.scaled(12.0f));
      grid.AddInputInt("Max Continents", &cfg.areaConfig.continents.maxAmount,
                       1, 50);
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
          (fwg.areaData.continents.empty() && !uiContext.asyncContext.busy())) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genContinents(cfg); },
            [&uiContext](TaskResult::Outcome) {
              uiContext.generationContext.modifiedAreas = true;
            });
      }

      if (uiContext.triggeredDrag) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg, &uiContext]() {
              auto evaluationAreas =
                  Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                      fwg.terrainData.landMask);
              if (!fwg.loadContinents(
                      cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                               uiContext.draggedFile, cfg.imageReadContext(),
                               evaluationAreas)))
                return false;
              return true;
            },
            [&uiContext](TaskResult::Outcome) {
              uiContext.triggeredDrag = false;
              uiContext.generationContext.modifiedAreas = true;
            });
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

} // namespace Fwg::UI::Areas
