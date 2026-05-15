#pragma once
#include "glad/glad.h"
#define GLFW_INCLUDE_NONE
#include "FastWorldGenerator.h"
#include "GLFW/glfw3.h"
#include "UI/UIUtils.h"
#include "UI/UiElements.h"
#include "utils/Cfg.h"
#include <map>

namespace Fwg::UI {
namespace Drawing {
enum class InteractionType { NONE, CLICK, RCLICK };

struct ClickEvent {
  int pixel;
  InteractionType type;
};

struct DrawContext {
  // ui config
  int brushSize = 1;
  float brushStrength = 1.0f;
  float brushHardness = 1.0f;
  bool processClickEvents = false;
  std::queue<ClickEvent> clickEvents;
};
} // namespace Drawing

struct ImageContext {

  std::array<Fwg::Gfx::Image, 2> activeImages;
  // ui state
  GLuint primaryTexture = 0;
  GLuint secondaryTexture = 0;

  float zoom = 1.0f;
  bool updateTexture1;
  bool updateTexture2;
  int textureWidth;
  int textureHeight;

  bool isPrimaryTextureActive() { return primaryTexture != 0; }
  bool isSecondaryTextureActive() { return secondaryTexture != 0; }

  bool hasTextureDimensions() const {
    return textureWidth > 0 && textureHeight > 0;
  }
  float getTextureAspectRatio() const {
    if (!hasTextureDimensions())
      return 1.0f; // default aspect ratio
    return static_cast<float>(textureWidth) / static_cast<float>(textureHeight);
  }

  void resetTexture(int id) {
    if (id == 0) {
      updateTexture1 = true;
    } else {
      updateTexture2 = true;
    }
  }

  void resetTexture() {
    resetTexture(0);
    resetTexture(1);
  }

  void updateImage(int index, const Fwg::Gfx::Image &image) {
    GLuint &texture = (index == 0) ? primaryTexture : secondaryTexture;
    bool &updateFlag = (index == 0) ? updateTexture1 : updateTexture2;

    // Free existing GL texture
    Fwg::UI::Utils::freeTexture(&texture);
    updateFlag = false;

    if (!image.initialised() || image.imageData.empty())
      return;

    try {
      activeImages[index] = image;

      // Upload OpenGL texture
      if (!Fwg::UI::Utils::getResourceView(image, &texture, &textureWidth,
                                           &textureHeight)) {
        Fwg::Utils::Logging::logLine("ERROR: Couldn't create OpenGL texture");
        return;
      }

      textureWidth = image.width();
      textureHeight = image.height();
    } catch (const std::exception &e) {
      Fwg::Utils::Logging::logLine(
          std::string("ERROR: Exception in updateImage: ") + e.what());
    }
  }
};

struct HelpContext {
  std::map<std::string, std::string> helpTexts;
  std::map<std::string, std::string> advancedHelpTexts;
  std::map<std::string, Fwg::Gfx::Image> advancedHelpImages;
  std::map<std::string, float> advancedHelpTexturesAspectRatio;
  std::map<std::string, GLuint> advancedHelpTextures;
  std::string activeKey = "";
  bool showExtendedHelp = false;

  // Loads texts containing information for users from a file
  void loadHelpTextsFromFile(const std::string &filePath) {
    Fwg::Utils::Logging::logLine("Loading help texts from ", filePath);
    auto lines = Fwg::Parsing::getLines(filePath + "//uiHelpTexts.txt");
    for (auto &line : lines) {
      auto tokens = Fwg::Parsing::getTokens(line, '|');
      helpTexts[tokens[0]].append(tokens[0]);
      for (auto i = 1; i < tokens.size(); i++) {
        helpTexts[tokens[0]].append("\n" + tokens[i]);
      }
    }

    for (const auto &entry :
         std::filesystem::directory_iterator(filePath + "//uiHelpTexts//")) {
      if (entry.is_directory()) {
        continue; // Skip directories
      }
      lines = Fwg::Parsing::getLines(entry.path().string());
      auto text = Fwg::Parsing::readFile(entry.path().string());
      auto filename = entry.path().filename().string();
      // remove the extension
      filename = filename.substr(0, filename.find_last_of('.'));
      advancedHelpTexts[filename] = text;
      // for (auto &line : lines) {
      //   auto tokens = Fwg::Parsing::getTokens(line, '|');
      //   advancedHelpTexts[filename][tokens[0]].append(tokens[0]);
      //   for (auto i = 1; i < tokens.size(); i++) {
      //     advancedHelpTexts[filename][tokens[0]].append("\n" + tokens[i]);
      //   }
      // }
    }
  }
  void loadHelpImagesFromPath(const std::string &path) {
    const std::string imageDirectory = path + "/uiHelpImages/";

    for (const auto &entry :
         std::filesystem::directory_iterator(imageDirectory)) {
      if (entry.is_directory())
        continue;

      // Strip extension
      std::string filename = entry.path().filename().string();
      filename = filename.substr(0, filename.find_last_of('.'));

      Fwg::Gfx::Image image = Fwg::Gfx::Png::load(entry.path().string());
      int w = image.width();
      int h = image.height();

      if (w <= 0 || h <= 0)
        continue;

      GLuint texture = 0;
      if (!Fwg::UI::Utils::getResourceView(image, &texture, &w, &h))
        continue;

      advancedHelpTextures[filename] = texture;
      advancedHelpTexturesAspectRatio[filename] = float(h) / float(w);
    }
  }

  void showHelpTextBox(const std::string &key, bool switchkey = true) {
    ImGui::BeginChild("Help Box",
                      ImVec2(ImGui::GetContentRegionAvail().x,
                             ImGui::GetContentRegionAvail().y * 0.1f),
                      false);

    activeKey = key;
    if (Fwg::UI::Elements::HelpButton("Extended Help")) {
      showExtendedHelp = !showExtendedHelp;
    }
    // display help text in a text box
    if (helpTexts.find(key) != helpTexts.end()) {
      ImGui::TextWrapped("Help: %s", helpTexts[key].c_str());
    }
    ImGui::EndChild();
  }

  void RenderEmphasizedText(const std::string &text, float textWidth) {
    ImGui::BeginGroup();
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + textWidth);

    size_t pos = 0;

    while (pos < text.size()) {
      // Find line end
      size_t lineEnd = text.find('\n', pos);
      std::string line = (lineEnd == std::string::npos)
                             ? text.substr(pos)
                             : text.substr(pos, lineEnd - pos);

      size_t cursor = 0;
      bool firstSegment = true;

      while (cursor < line.size()) {
        size_t startBold = line.find("**", cursor);

        if (startBold != std::string::npos) {
          if (startBold > cursor) {
            std::string normalPart = line.substr(cursor, startBold - cursor);
            if (!firstSegment)
              ImGui::SameLine();
            ImGui::TextUnformatted(normalPart.c_str());
            firstSegment = false;
          }

          size_t endBold = line.find("**", startBold + 2);
          if (endBold == std::string::npos) {
            std::string remaining = line.substr(startBold);
            if (!firstSegment)
              ImGui::SameLine();
            ImGui::TextUnformatted(remaining.c_str());
            break;
          }

          std::string boldPart =
              line.substr(startBold + 2, endBold - (startBold + 2));
          if (!firstSegment)
            ImGui::SameLine();
          ImGui::SetWindowFontScale(1.3f);
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
          ImGui::TextUnformatted(boldPart.c_str());
          ImGui::PopStyleColor();
          ImGui::SetWindowFontScale(1.0f);
          firstSegment = false;

          cursor = endBold + 2;
        } else {
          std::string normalPart = line.substr(cursor);
          if (!firstSegment)
            ImGui::SameLine();
          ImGui::TextUnformatted(normalPart.c_str());
          break;
        }
      }

      pos = (lineEnd == std::string::npos) ? text.size() : lineEnd + 1;
    }

    ImGui::PopTextWrapPos();
    ImGui::EndGroup();
  }

  void showAdvancedTextBox() {
    if (showExtendedHelp &&
        advancedHelpTexts.find(activeKey) != advancedHelpTexts.end()) {
      ImGui::PushStyleColor(
          ImGuiCol_WindowBg,
          ImVec4(0.2f, 0.1f, 0.0f, 1.0f)); // Custom dark blueish background
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4.0f); // Thick border
      ImGui::PushStyleColor(ImGuiCol_Border,
                            ImVec4(0.9f, 0.6f, 0.1f, 1.0f)); // Border color
      auto size = ImGui::GetMainViewport()->Size;
      size.x *= 0.9f;
      size.y *= 0.9f;

      ImGui::SetNextWindowSize(size); // A bit smaller than current resolution
      ImGui::Begin("Extended Help", &showExtendedHelp,
                   ImGuiWindowFlags_NoCollapse);

      ImGui::Text("Detailed Help Section");
      if (ImGui::Button("Close Detailed Help Section")) {
        showExtendedHelp = !showExtendedHelp;
      }
      ImGui::Separator();

      // Scrollable region
      ImGui::BeginChild("HelpScrollArea", ImVec2(0, 0), false,
                        ImGuiWindowFlags_AlwaysVerticalScrollbar);

      auto value = advancedHelpTexts[activeKey];
      // Display heading
      ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1.0f), "%s", activeKey.c_str());

      // Spacing
      ImGui::Spacing();

      float fullWidth = ImGui::GetContentRegionAvail().x;
      float imageWidth = fullWidth * 0.4f; // 48% for padding
      float textWidth = fullWidth * 0.6f;

      // Start horizontal layout
      ImGui::BeginGroup();
      if (advancedHelpTextures.contains(activeKey)) {
        ImGui::Image(
            (ImTextureID)(intptr_t)advancedHelpTextures[activeKey],
            ImVec2(imageWidth,
                   imageWidth * advancedHelpTexturesAspectRatio[activeKey]));
      }
      ImGui::EndGroup();

      ImGui::SameLine();

      ImGui::BeginGroup();
      ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + textWidth);
      RenderEmphasizedText(value, textWidth);
      ImGui::PopTextWrapPos();
      ImGui::EndGroup();

      ImGui::Separator();
      ImGui::End();            // Ends the "Extended Help" window
      ImGui::PopStyleColor(2); // WindowBg + Border
      ImGui::PopStyleVar();    // Border size
    }
  }
};

struct AsyncContext {
  std::atomic<bool> computationRunning;
  std::atomic<bool> computationStarted;
  std::future<bool> computationFutureBool;

  // Function wrapper to run any function asynchronously
  template <typename Func, typename... Args>
  auto runAsync(Func func, Args &...args) {
    computationStarted = true;
    computationRunning = true;
    return std::async(std::launch::async, func, std::ref(args)...);
  }
  template <typename Func, typename... Args>
  auto runAsyncInitialDisable(Func func, Args &...args) {
    computationRunning = true;
    return std::async(std::launch::async, func, std::ref(args)...);
  }
};

struct LayoutContext {
  float leftColumnWidth = 0.4f;
};

struct GenerationContext {
  bool analyze = false;
  int amountClassificationsNeeded = 0;
  bool redoHumidity = false;
  bool modifiedAreas = false;
};
struct ClimateInput {
  Fwg::Gfx::Colour in;
  Fwg::Gfx::Colour out;
  std::string rgbName;
  Fwg::Climate::ClimateClassDefinition climate;
  ImVec4 colour;
  std::vector<int> pixels;
};
struct ClimateUiContext {

  bool analyze = false;
  int amountClassificationsNeeded = 0;
  std::set<Fwg::Gfx::Colour> highlightedInputs;
  Fwg::Gfx::Image climateInputMap;
  Fwg::Utils::ColourTMap<ClimateInput> climateInputColours;
  Fwg::Utils::ColourTMap<Fwg::Climate::ClimateClassDefinition>
      allowedClimateInputs;
};

struct UIContext {
  Drawing::DrawContext drawContext;
  ImageContext imageContext;
  HelpContext helpContext;
  AsyncContext asyncContext;
  LayoutContext layoutContext;
  GenerationContext generationContext;
  ClimateUiContext climateUI;

  std::string draggedFile = "";
  bool triggeredDrag = false;
  bool tabSwitchEvent(const bool processClickEvents = false) {
    this->drawContext.processClickEvents = processClickEvents;
    if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered()) {
      imageContext.resetTexture();
      return true;
    }
    return imageContext.updateTexture1 || imageContext.updateTexture2;
  }
};

} // namespace Fwg::UI