#include "UI/UIUtils.h"

void UIUtils::imageClick(float scale, ImGuiIO &io) {
  // ensure we have an image to click on
  if (!activeImages[0].size()) {
    return;
  }
  // Check if the mouse is clicked on the image
  if ((ImGui::IsItemHovered() && ImGui::IsMouseReleased(0) ||
       ImGui::IsMouseReleased(1))) {
    // Determine which mouse button was pressed
    auto pressedKey =
        ImGui::IsMouseReleased(0) ? ImGuiKey_MouseLeft : ImGuiKey_MouseRight;

    // Get the mouse position relative to the image
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 imagePos = ImGui::GetItemRectMin();
    auto itemSize = ImGui::GetItemRectSize();
    ImVec2 mousePosRelative =
        ImVec2(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

    // Calculate the pixel position in the texture
    int pixelX = static_cast<int>((mousePosRelative.x / itemSize.x) *
                                  activeImages[0].width());
    int pixelY = activeImages[0].height() -
                 static_cast<int>((mousePosRelative.y / itemSize.y) *
                                  activeImages[0].height());

    // Determine the type of interaction based on the mouse button pressed and
    // whether the Ctrl key is held down
    auto interactionType = pressedKey == ImGuiKey_MouseLeft
                               ? InteractionType::CLICK
                               : InteractionType::RCLICK;

    // Calculate the index of the pixel in the texture data
    int pixelIndex = (pixelY * activeImages[0].width() + pixelX);

    // If click events are being processed, add this event to the queue
    if (processClickEventsEnabled()) {
      if (clickEvents.size() < 1) {
        clickEvents.push({pixelIndex, interactionType});
      }
    }
  }
}
ClickEvent UIUtils::getClickedPixel() {
  if (clickEvents.size() > 0) {
    ClickEvent event = clickEvents.front();
    clickEvents.pop();
    return event;
  }
  return {
      -1,
      InteractionType::NONE,
  };
}
std::vector<std::pair<ClickEvent, double>>
UIUtils::getAffectedPixels(const ClickEvent &clickEvent) {
  if (clickEvent.pixel < 0)
    return {};
  auto &cfg = Fwg::Cfg::Values();

  std::vector<std::pair<ClickEvent, double>> affectedPixels;
  for (int i = 0; i < clickOffsets.size(); i++) {
    auto pix = clickEvent.pixel + clickOffsets[i];
    if (pix < cfg.processingArea && pix >= 0) {
      auto strength = brushStrength; /* *
          (brushHardness + (1.0 - brushHardness) * (1.0 - clickStrengths[i]));*/
      affectedPixels.push_back(
          std::make_pair(ClickEvent{pix, clickEvent.type}, strength));
    }
  }
  return affectedPixels;
}
std::vector<std::pair<ClickEvent, double>> UIUtils::getLatestAffectedPixels() {
  return getAffectedPixels(getClickedPixel());
}
void UIUtils::brushSettingsHeader() {
  auto &cfg = Fwg::Cfg::Values();
  bool update = false;
  if (ImGui::SliderInt("<--Brushsize", &brushSize, 0, 100)) {
    update = true;
  }

  ImGui::SameLine();
  ImGui::SliderFloat("<--Brush strength", &brushStrength, 0.0f, 1.0f);
  ImGui::SameLine();
  // showHelpTextBox("Drawing");
  //  ImGui::SameLine();
  //  ImGui::SliderFloat("<--Brush hardness", &brushHardness, 0.0f, 1.0f);
  if (update) {
    setClickOffsets(cfg.width, brushSize);
    // for (int i = 0; i < clickOffsets.size(); i++) {
    //   auto p1 = clickOffsets[i];
    //   auto p2 = 0;
    //   auto xmod = abs(p1 % cfg.width);
    //   const double x1 = xmod < cfg.width / 2 ? xmod : xmod - cfg.width;
    //   const double x2 = p2 % cfg.width;
    //   const double y1 = (double)p1 / (double)cfg.width;
    //   const double y2 = (double)p2 / (double)cfg.width;
    //   clickStrengths.push_back(
    //       (sqrt(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)))) /
    //       (double)brushSize);
    // }
  }
}

void UIUtils::setClickOffsets(int width, int brushSize) {
  clickOffsets = Fwg::Utils::getCircularOffsets(width, brushSize);
}
void UIUtils::resetTexture(int id) {
  if (id == 0) {
    updateTexture1 = true;
  } else {
    updateTexture2 = true;
  }
}

void UIUtils::resetTexture() {
  resetTexture(0);
  resetTexture(1);
}

bool UIUtils::tabSwitchEvent(bool processClickEvents) {
  this->processClickEvents = processClickEvents;
  if (ImGui::IsMouseReleased(0) && ImGui::IsItemHovered()) {
    resetTexture();
    return true;
  }
  return updateTexture1 || updateTexture2;
}

void UIUtils::updateImage(int index, const Fwg::Gfx::Image &image) {
  GLuint &texture = (index == 0) ? primaryTexture : secondaryTexture;
  bool &updateFlag = (index == 0) ? updateTexture1 : updateTexture2;

  // Free existing GL texture
  freeTexture(&texture);
  updateFlag = false;

  if (!image.initialised() || image.imageData.empty())
    return;

  try {
    activeImages[index] = image;

    // Upload OpenGL texture
    if (!getResourceView(image, &texture, &textureWidth, &textureHeight)) {
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

void UIUtils::freeTexture(GLuint *texture) {
  if (texture && *texture != 0) {
    glDeleteTextures(1, texture);
    *texture = 0;
  }
}

bool UIUtils::getResourceView(const Fwg::Gfx::Image &image, GLuint *out_tex,
                              int *out_width, int *out_height) {
  if (!out_tex)
    return false;

  const int w = image.width();
  const int h = image.height();
  const std::vector<unsigned char> pixels = image.getFlipped32bit();

  *out_tex = 0;

  glGenTextures(1, out_tex);
  glBindTexture(GL_TEXTURE_2D, *out_tex);

  // Texture upload
  glTexImage2D(GL_TEXTURE_2D, 0,
               GL_RGBA, // internal format
               w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

  // Basic sampling
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_2D, 0);

  if (out_width)
    *out_width = w;
  if (out_height)
    *out_height = h;

  return true;
}

ImGuiIO &UIUtils::setupImGuiContextAndStyle() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();
  return io;
}

GLFWwindow *UIUtils::createAndConfigureWindow(int width, int height,
                                              const char *title) {
  if (!glfwInit())
    return nullptr;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window)
    return nullptr;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // vsync

  // Load GL functions
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return nullptr;

  return window;
}

void UIUtils::setupImGuiBackends(GLFWwindow *window) {
  // GLFW backend
  ImGui_ImplGlfw_InitForOpenGL(window, true);

  // OpenGL3 backend (GLSL 330 core is the safest cross-platform default)
  ImGui_ImplOpenGL3_Init("#version 330 core");
}

void UIUtils::cleanupGLResources() {
  for (auto &kv : advancedHelpTextures)
    glDeleteTextures(1, &kv.second);

  if (primaryTexture)
    glDeleteTextures(1, &primaryTexture);

  if (secondaryTexture)
    glDeleteTextures(1, &secondaryTexture);
}

void UIUtils::renderImGui(const ImVec4 &clear_color, GLFWwindow *window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  const float clear_color_with_alpha[4] = {
      clear_color.x * clear_color.w, clear_color.y * clear_color.w,
      clear_color.z * clear_color.w, clear_color.w};

  glClearColor(clear_color_with_alpha[0], clear_color_with_alpha[1],
               clear_color_with_alpha[2], clear_color_with_alpha[3]);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);
}

void UIUtils::shutdownImGui() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

namespace Fwg::UI::Utils::Masks {
std::vector<std::vector<int>>
getLandmaskEvaluationAreas(std::vector<bool> &mask) {
  std::vector<std::vector<int>> evaluationAreas(2);
  for (int i = 0; auto pix : mask) {
    if (pix) {
      evaluationAreas[0].push_back(i);
    } else {
      evaluationAreas[1].push_back(i);
    }
    i++;
  }
  return evaluationAreas;
}

} // namespace Fwg::UI::Utils::Masks

void Fwg::UI::Elements::borderChild(const std::string &label,
                                    const std::function<void()> &guiCalls) {
  {
    ::ImGui::BeginChild(label.c_str(), ImVec2(400, 150),
                        true); // Begin child window with border}
    guiCalls();                // Call the function passed as parameter
    ImGui::EndChild();         // End child window
  }
}

// Loads texts containing information for users from a file
void UIUtils::loadHelpTextsFromFile(const std::string &filePath) {
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
void UIUtils::loadHelpImagesFromPath(const std::string &path) {
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
    if (!getResourceView(image, &texture, &w, &h))
      continue;

    advancedHelpTextures[filename] = texture;
    advancedHelpTexturesAspectRatio[filename] = float(h) / float(w);
  }
}

void UIUtils::showHelpTextBox(const std::string &key, bool switchkey) {
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

void UIUtils::showAdvancedTextBox() {
  auto key = activeKey;
  if (showExtendedHelp &&
      advancedHelpTexts.find(key) != advancedHelpTexts.end()) {
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

    auto value = advancedHelpTexts[key];
    // Display heading
    ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1.0f), "%s", key.c_str());

    // Spacing
    ImGui::Spacing();

    float fullWidth = ImGui::GetContentRegionAvail().x;
    float imageWidth = fullWidth * 0.4f; // 48% for padding
    float textWidth = fullWidth * 0.6f;

    // Start horizontal layout
    ImGui::BeginGroup();
    if (advancedHelpTextures.contains(key)) {
      ImGui::Image((ImTextureID)(intptr_t)advancedHelpTextures[key],
                   ImVec2(imageWidth,
                          imageWidth * advancedHelpTexturesAspectRatio[key]));
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