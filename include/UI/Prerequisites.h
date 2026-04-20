#pragma once
#include "FastWorldGenerator.h"
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

namespace Fwg::UI {

// Represents a single prerequisite check
struct Prerequisite {
  std::string name;
  std::string missingMessage;
  std::function<bool()> check;

  Prerequisite(std::string_view name, std::string_view msg,
               std::function<bool()> checkFn)
      : name(name), missingMessage(msg), check(std::move(checkFn)) {}
};

// RAII guard that handles BeginDisabled/EndDisabled
class PrerequisiteGuard {
  bool disabled = false;

public:
  explicit PrerequisiteGuard(bool shouldDisable) : disabled(shouldDisable) {
    if (disabled) {
      ImGui::BeginDisabled();
    }
  }

  ~PrerequisiteGuard() {
    if (disabled) {
      ImGui::EndDisabled();
    }
  }

  // Returns true if prerequisites are met (not disabled)
  explicit operator bool() const { return !disabled; }
  bool ready() const { return !disabled; }
};

// Main prerequisite checker class
class PrerequisiteChecker {
public:
  // =========================================================================
  // Terrain Prerequisites
  // =========================================================================

  static Prerequisite heightmap(const Terrain::TerrainData &terrainData) {
    return {"Heightmap", "Generate or load a heightmap first",
            [&]() { return !terrainData.detailedHeightMap.empty(); }};
  }

  static Prerequisite landMask(const Terrain::TerrainData &terrainData) {
    return {"Land mask", "Generate land/sea classification first",
            [&]() { return !terrainData.landMask.empty(); }};
  }

  static Prerequisite landforms(const Terrain::TerrainData &terrainData) {
    return {"Landform classification", "Generate landform types first",
            [&]() { return !terrainData.landFormIds.empty(); }};
  }

  static Prerequisite normalMap(const Terrain::TerrainData &terrainData) {
    return {"Normal map", "Generate normal/sobel map first",
            [&]() { return !terrainData.sobelData.empty(); }};
  }

  // =========================================================================
  // Climate Prerequisites
  // =========================================================================

  static Prerequisite temperature(const Climate::ClimateData &climateData) {
    return {"Temperature data", "Generate temperature map first",
            [&]() { return !climateData.averageTemperatures.empty(); }};
  }

  static Prerequisite humidity(const Climate::ClimateData &climateData) {
    return {"Humidity data", "Generate humidity map first",
            [&]() { return !climateData.humidities.empty(); }};
  }

  static Prerequisite climate(const Climate::ClimateData &climateData) {
    return {"Climate zones", "Generate climate classification first",
            [&]() { return !climateData.climateChances.empty(); }};
  }

  static Prerequisite habitability(const Climate::ClimateData &climateData) {
    return {"Habitability data", "Generate habitability/density map first",
            [&]() { return !climateData.habitabilities.empty(); }};
  }

  // =========================================================================
  // Area Prerequisites
  // =========================================================================

  static Prerequisite segments(const Areas::AreaData &areaData) {
    return {"Segments", "Generate segments first",
            [&]() { return !areaData.segments.empty(); }};
  }

  static Prerequisite superSegments(const Areas::AreaData &areaData) {
    return {"Super segments", "Generate super segments first",
            [&]() { return !areaData.superSegments.empty(); }};
  }

  static Prerequisite provinces(const Areas::AreaData &areaData) {
    return {"Provinces", "Generate provinces first",
            [&]() { return !areaData.provinces.empty(); }};
  }

  static Prerequisite regions(const Areas::AreaData &areaData) {
    return {"Regions", "Generate regions first",
            [&]() { return !areaData.regions.empty(); }};
  }

  static Prerequisite landBodies(const Areas::AreaData &areaData) {
    return {"Land bodies", "Generate land body detection first",
            [&]() { return !areaData.landBodies.empty(); }};
  }

  static Prerequisite continents(const Areas::AreaData &areaData) {
    return {"Continents", "Generate continents first",
            [&]() { return !areaData.continents.empty(); }};
  }

  // =========================================================================
  // Image Prerequisites
  // =========================================================================

  static Prerequisite worldMap(const Gfx::Image &worldMap) {
    return {"World map", "Generate world map visualization first",
            [&]() { return worldMap.initialised(); }};
  }

  static Prerequisite provinceMap(const Gfx::Image &provinceMap) {
    return {"Province map", "Generate province map first",
            [&]() { return provinceMap.initialised(); }};
  }

  // =========================================================================
  // Check Methods
  // =========================================================================

  // Check multiple prerequisites and display UI for missing ones
  // Returns a guard that disables UI if any prerequisite is missing
  static PrerequisiteGuard
  require(std::initializer_list<Prerequisite> prerequisites) {
    std::vector<std::string> missing;

    for (const auto &prereq : prerequisites) {
      if (!prereq.check()) {
        missing.push_back(prereq.missingMessage);
      }
    }

    if (!missing.empty()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
      ImGui::TextUnformatted("Missing prerequisites:");
      ImGui::PopStyleColor();

      for (const auto &msg : missing) {
        ImGui::BulletText("%s", msg.c_str());
      }
      ImGui::Spacing();
    }

    return PrerequisiteGuard(!missing.empty());
  }

  // Compact version - single line summary
  static PrerequisiteGuard
  requireCompact(std::initializer_list<Prerequisite> prerequisites) {
    std::vector<std::string> missingNames;

    for (const auto &prereq : prerequisites) {
      if (!prereq.check()) {
        missingNames.push_back(prereq.name);
      }
    }

    if (!missingNames.empty()) {
      std::string summary = "Requires: ";
      for (size_t i = 0; i < missingNames.size(); ++i) {
        if (i > 0)
          summary += ", ";
        summary += missingNames[i];
      }
      ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", summary.c_str());
    }

    return PrerequisiteGuard(!missingNames.empty());
  }

  // Silent check - no UI output, just returns guard
  static PrerequisiteGuard
  requireSilent(std::initializer_list<Prerequisite> prerequisites) {
    for (const auto &prereq : prerequisites) {
      if (!prereq.check()) {
        return PrerequisiteGuard(true);
      }
    }
    return PrerequisiteGuard(false);
  }
};


} // namespace Fwg::UI