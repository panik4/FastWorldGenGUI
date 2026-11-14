#include "UI/AreaUI.h"

namespace Fwg::UI::Areas {
namespace {
bool hasCurrentVisualizationDimensions(const Fwg::Cfg &config,
                                       const char *visualizationName) {
  if (Fwg::Gfx::isCurrentVisualizationDimensions(config.width, config.height,
                                                 config.processingArea))
    return true;
  Fwg::Utils::Logging::logLine("Skipping ", visualizationName,
                               " visualization: map dimensions are stale");
  return false;
}
} // namespace

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
      const bool currentDimensions =
          hasCurrentVisualizationDimensions(cfg, "density");
      uiContext.imageContext.updateImage(
          0, fwg.hasHabitabilityData(cfg) && currentDimensions
                 ? Gfx::displayHabitability(fwg.climateData.habitabilities,
                                            cfg.climateGenerationContext())
                 : Fwg::Gfx::Image());
      uiContext.imageContext.updateImage(
          1, fwg.hasWorldImageData(cfg) ? fwg.worldMap : Fwg::Gfx::Image{});
    }

    uiContext.helpContext.showHelpTextBox("Density");

    ImGui::SeparatorText("Province & State Density Map");
    ImGui::TextWrapped("This map determines the density of provinces and "
                       "states based on climate habitability.");

    ImGui::Spacing();
    {

      {
        UI::PrerequisiteGuard actionGuard{
            Fwg::PrerequisiteChecker::canGenerateHabitability(fwg, cfg)};
        if (UI::Elements::ImportantStepButton(
                "Generate Density from Climate Data", ImVec2(250, 0))) {
          uiContext.asyncContext.runAsync(
              [&fwg, &cfg]() { return fwg.genHabitability(cfg); });
        }
      }

      if (uiContext.triggeredDrag) {
        fwg.loadHabitability(cfg, uiContext.draggedFile);
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
      if (hasCurrentVisualizationDimensions(cfg, "supersegments") &&
          cfg.processingArea > 0 && fwg.hasWorldImageData(cfg) &&
          fwg.hasSuperSegmentsData(cfg)) {
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

    {
      {
        UI::PrerequisiteGuard actionGuard{Fwg::PrerequisiteChecker::check(
            {Fwg::PrerequisiteChecker::climate(fwg, cfg),
             Fwg::PrerequisiteChecker::landforms(fwg, cfg),
             Fwg::PrerequisiteChecker::habitability(fwg, cfg)})};
        if (UI::Elements::Button("Generate Template Images", false,
                                 ImVec2(220, 0))) {
          Fwg::Gfx::Land::displaySimpleLandType(
              fwg.terrainData, fwg.areaData, fwg.worldMap, true, false, false,
              cfg.visualizationContext());
        }
      }

      ImGui::SameLine();

      {
        UI::PrerequisiteGuard actionGuard{
            Fwg::PrerequisiteChecker::canGenerateSuperSegments(fwg, cfg)};
        if (UI::Elements::ImportantStepButton("Generate SuperSegments",
                                              ImVec2(220, 0))) {
          uiContext.asyncContext.runAsync(
              [&fwg, &cfg]() { return fwg.genSuperSegments(cfg); });
        }
      }

      if (uiContext.triggeredDrag) {
        UI::PrerequisiteGuard importGuard{
            Fwg::PrerequisiteChecker::canLoadSuperSegments(fwg, cfg)};
        if (importGuard.ready()) {
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
                  Fwg::Gfx::Filter::fillBlackPixelsByArea(image,
                                                          evaluationAreas);
                  if (!fwg.loadSuperSegments(cfg, image))
                    return false;
                }
                return true;
              },
              [&uiContext](TaskResult::Outcome) {
                uiContext.triggeredDrag = false;
              });
        } else {
          uiContext.triggeredDrag = false;
        }
      }
    }

    ImGui::EndTabItem();
  }
}
void showSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                    UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Segments")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, fwg.hasSegmentImageData(cfg) ? fwg.segmentMap : Gfx::Image{});
      uiContext.imageContext.updateImage(1, fwg.errorMap);
    }
    uiContext.helpContext.showHelpTextBox("Segments");

    ImGui::SeparatorText("Segment Configuration");

    {
      UI::Elements::GridLayout grid(2, uiContext.scaled(200.0f),
                                    uiContext.scaled(12.0f));

      grid.AddInputFloat("Cost Influence",
                         &cfg.areaConfig.segments.costInfluence, 0.0f, 10.0f);
      grid.AddInputFloat("Distance Influence",
                         &cfg.areaConfig.segments.distanceInfluence, 0.0f,
                         10.0f);
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
    {
      UI::PrerequisiteGuard generationGuard{
          Fwg::PrerequisiteChecker::canGenerateSegments(fwg, cfg)};
      if (UI::Elements::ImportantStepButton("Generate Segments",
                                            ImVec2(220, 0))) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() { return fwg.genSegments(cfg); },
            [&uiContext](TaskResult::Outcome) {
              uiContext.generationContext.modifiedAreas = true;
            });
      }
    }

    {
      ImGui::SeparatorText("Segment Statistics");

      {
        UI::Elements::GridLayout grid(3, uiContext.scaled(150.0f),
                                      uiContext.scaled(12.0f));
        grid.AddText("Land Segments", "%d", (int)fwg.areaData.landSegments);
        grid.AddText("Sea Segments", "%d", (int)fwg.areaData.seaSegments);
        grid.AddText("Lake Segments", "%d", (int)fwg.areaData.lakeSegments);
      }

      ImGui::Spacing();

      {
        UI::PrerequisiteGuard actionGuard{Fwg::PrerequisiteChecker::check(
            {Fwg::PrerequisiteChecker::climate(fwg, cfg),
             Fwg::PrerequisiteChecker::landforms(fwg, cfg),
             Fwg::PrerequisiteChecker::habitability(fwg, cfg),
             Fwg::PrerequisiteChecker::superSegments(fwg, cfg)})};
        if (UI::Elements::Button("Generate Template Images", false,
                                 ImVec2(220, 0))) {
          Fwg::Gfx::Land::displaySimpleLandType(
              fwg.terrainData, fwg.areaData, fwg.worldMap, false, true, false,
              cfg.visualizationContext());
        }
      }

      ImGui::SameLine();
      bool fixRiverBordersClicked = false;
      {
        UI::PrerequisiteGuard repairGuard{
            Fwg::PrerequisiteChecker::canFixSegmentBordersAtRivers(fwg, cfg)};
        fixRiverBordersClicked = UI::Elements::ImportantStepButton(
            "Fix River Borders", ImVec2(220, 0));
      }
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
        UI::PrerequisiteGuard importGuard{
            Fwg::PrerequisiteChecker::canLoadSegments(fwg, cfg)};
        if (importGuard.ready()) {
          uiContext.asyncContext.runAsync(
              [&fwg, &cfg, &uiContext]() {
                const auto evaluationAreas =
                    Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                        fwg.terrainData.landMask);
                if (cfg.areaConfig.common.inputMode ==
                    Fwg::Areas::AreaInputType::SOLID) {
                  if (!fwg.loadSegments(
                          cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                                   uiContext.draggedFile,
                                   cfg.imageReadContext(), evaluationAreas)))
                    return false;
                } else {
                  auto image = Fwg::IO::Reader::readGenericImage(
                      uiContext.draggedFile, cfg.imageReadContext());
                  Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                      image, evaluationAreas);
                  Fwg::Gfx::Filter::fillBlackPixelsByArea(image,
                                                          evaluationAreas);
                  if (!fwg.loadSegments(cfg, image))
                    return false;
                }
                return true;
              },
              [&uiContext](TaskResult::Outcome) {
                uiContext.triggeredDrag = false;
                uiContext.generationContext.modifiedAreas = true;
              });
        } else {
          uiContext.triggeredDrag = false;
        }
      }
    }

    ImGui::EndTabItem();
  }
}
int showProvincesTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg,
                     UIContext &uiContext) {
  if (UI::Elements::BeginSubTabItem("Provinces")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, fwg.hasProvinceImageData(cfg) ? fwg.provinceMap : Gfx::Image{});
      uiContext.imageContext.updateImage(
          1, fwg.hasSegmentImageData(cfg) ? fwg.segmentMap : Gfx::Image{});
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

    {
      {
        UI::PrerequisiteGuard actionGuard{Fwg::PrerequisiteChecker::check(
            {Fwg::PrerequisiteChecker::climate(fwg, cfg),
             Fwg::PrerequisiteChecker::landforms(fwg, cfg),
             Fwg::PrerequisiteChecker::habitability(fwg, cfg),
             Fwg::PrerequisiteChecker::superSegments(fwg, cfg),
             Fwg::PrerequisiteChecker::segments(fwg, cfg)})};
        if (UI::Elements::Button("Generate Template Images", false,
                                 ImVec2(220, 0))) {
          Fwg::Gfx::Land::displaySimpleLandType(
              fwg.terrainData, fwg.areaData, fwg.worldMap, false, false, true,
              cfg.visualizationContext());
        }
      }

      ImGui::SameLine();

      {
        UI::PrerequisiteGuard actionGuard{
            Fwg::PrerequisiteChecker::canGenerateProvinces(fwg, cfg)};
        if (UI::Elements::ImportantStepButton("Generate Provinces",
                                              ImVec2(220, 0))) {
          uiContext.asyncContext.runAsync(
              [&fwg, &cfg]() { return fwg.genProvinces(cfg); },
              [&uiContext](TaskResult::Outcome) {
                uiContext.generationContext.modifiedAreas = true;
              });
        }
      }

      if (uiContext.triggeredDrag) {
        UI::PrerequisiteGuard importGuard{
            Fwg::PrerequisiteChecker::canLoadProvinces(fwg, cfg)};
        if (importGuard.ready()) {
          uiContext.asyncContext.runAsync(
              [&fwg, &cfg, &uiContext]() {
                auto evaluationAreas =
                    Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                        fwg.terrainData.landMask);
                if (cfg.areaConfig.common.inputMode ==
                    Fwg::Areas::AreaInputType::SOLID) {
                  if (!fwg.loadProvinces(
                          cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                                   uiContext.draggedFile,
                                   cfg.imageReadContext(), evaluationAreas)))
                    return false;
                } else {
                  auto image = Fwg::IO::Reader::readGenericImage(
                      uiContext.draggedFile, cfg.imageReadContext());
                  Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                      image, evaluationAreas);
                  Fwg::Gfx::Filter::fillBlackPixelsByArea(image,
                                                          evaluationAreas);
                  if (!fwg.loadProvinces(cfg, image))
                    return false;
                }
                return true;
              },
              [&uiContext](TaskResult::Outcome) {
                uiContext.triggeredDrag = false;
                uiContext.generationContext.modifiedAreas = true;
                uiContext.imageContext.resetTexture();
              });
        } else {
          uiContext.triggeredDrag = false;
        }
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

    {
      ImGui::SeparatorText("Generate a region map");

      {
        UI::PrerequisiteGuard actionGuard{
            Fwg::PrerequisiteChecker::canGenerateRegions(fwg, cfg)};
        if (ImGui::Button("Generate Region Map from Segments and Provinces")) {
          uiContext.asyncContext.runAsync(
              [&fwg, &cfg]() { return fwg.genRegions(cfg); });
        }
      }
      ImGui::Text("The map has %i regions",
                  static_cast<int>(fwg.areaData.regions.size()));

      if (uiContext.triggeredDrag) {
        fwg.loadRegions(cfg, uiContext.draggedFile);
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
      if (hasCurrentVisualizationDimensions(cfg, "continents") &&
          fwg.hasContinentsData(cfg) && fwg.hasHeightmapData(cfg)) {
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

    {
      {
        UI::PrerequisiteGuard actionGuard{
            Fwg::PrerequisiteChecker::canGenerateContinents(fwg, cfg)};
        if (UI::Elements::ImportantStepButton("Generate Continents",
                                              ImVec2(220, 0))) {
          uiContext.asyncContext.runAsync(
              [&fwg, &cfg]() { return fwg.genContinents(cfg); },
              [&uiContext](TaskResult::Outcome) {
                uiContext.generationContext.modifiedAreas = true;
              });
        }
      }

      if (uiContext.triggeredDrag) {
        UI::PrerequisiteGuard importGuard{
            Fwg::PrerequisiteChecker::canLoadContinents(fwg, cfg)};
        if (importGuard.ready()) {
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
                uiContext.imageContext.resetTexture();
              });
        } else {
          uiContext.triggeredDrag = false;
        }
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

} // namespace Fwg::UI::Areas
