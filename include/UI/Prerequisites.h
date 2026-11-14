#pragma once
#include "FastWorldGenerator.h"
#include "utils/PrerequisiteChecker.h"
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

  explicit PrerequisiteGuard(const Fwg::PrerequisiteResult &result)
      : disabled(!result.ready()) {
    if (disabled) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.2f, 1.0f));
      ImGui::TextUnformatted("Missing prerequisites:");
      ImGui::PopStyleColor();
      for (const auto &entry : result.missing)
        ImGui::BulletText("%.*s", static_cast<int>(entry.message.size()),
                          entry.message.data());
      ImGui::Spacing();
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
  static Prerequisite availableData(std::string_view name,
                                    std::string_view missingMessage,
                                    bool available) {
    return {name, missingMessage, [available] { return available; }};
  }

  // =========================================================================
  // Check Methods
  // =========================================================================

  // Check multiple prerequisites and display UI for missing ones
  // Returns a guard that disables UI if any prerequisite is missing
  static PrerequisiteGuard
  require(std::initializer_list<Prerequisite> prerequisites) {
    Fwg::PrerequisiteResult result;

    for (const auto &prereq : prerequisites) {
      if (!prereq.check()) {
        result.missing.push_back({prereq.name, prereq.missingMessage});
      }
    }

    return PrerequisiteGuard(result);
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
