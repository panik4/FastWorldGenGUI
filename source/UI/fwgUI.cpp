#include "UI/FwgUI.h"
#include <cmath>
#include <stdexcept>

namespace Fwg::UI {

FwgUI::FwgUI() : landUI() {}

Fwg::FastWorldGenerator &FwgUI::currentGenerator() {
  return *standaloneGenerator;
}

std::string FwgUI::windowTitle() const { return "FastWorldGen 0.9.1"; }

std::string FwgUI::frameTitle() const { return "FastWorldGen"; }

void FwgUI::onWindowReady() {}

void FwgUI::renderTopBar() {}

void FwgUI::renderSettingsTabs(Fwg::Cfg &cfg) {
  defaultTabs(cfg, currentGenerator());
}

void FwgUI::renderGenericContents(Fwg::Cfg &cfg) {
  showGeneric(cfg, currentGenerator());
}

void FwgUI::renderGenericControls(Fwg::Cfg &cfg) {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 100, 144, 40));
  ImGui::BeginChild(
      "GenericWrapper",
      ImVec2(ImGui::GetContentRegionAvail().x,
             std::max<float>(ImGui::GetContentRegionAvail().y * 0.3f, 100.0f)),
      false, ImGuiWindowFlags_None);
  renderGenericContents(cfg);
  ImGui::EndChild();
  const auto childMin = ImGui::GetItemRectMin();
  const auto childMax = ImGui::GetItemRectMax();
  ImGui::GetWindowDrawList()->AddRect(
      childMin, childMax, IM_COL32(50, 91, 120, 255), 0.0f, 0, 2.0f);
  ImGui::PopStyleColor();
}

void FwgUI::logWrapper() {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 100, 144, 40));
  ImGui::BeginChild("Log",
                    ImVec2(ImGui::GetContentRegionAvail().x * 1.0f,
                           ImGui::GetContentRegionAvail().y * 1.0f),
                    false, ImGuiWindowFlags_None);
  {
    const auto logSnapshot =
        Fwg::Utils::Logging::Logger::logInstance.getFullLog();
    ImGui::TextUnformatted(logSnapshot.c_str());
    if (!ImGui::IsWindowHovered()) {
      // scroll to bottom
      ImGui::SetScrollHereY(1.0f);
    }
    ImGui::PopStyleColor();
  }
  ImGui::EndChild();
  // Draw a frame around the child region
  ImVec2 childMin = ImGui::GetItemRectMin();
  ImVec2 childMax = ImGui::GetItemRectMax();
  ImGui::GetWindowDrawList()->AddRect(
      childMin, childMax, IM_COL32(64, 69, 112, 255), 0.0f, 0, 2.0f);
}

void FwgUI::imageWrapper(ImGuiIO &io) {
  static ImVec2 cursorPos;

  float modif = 1.0 - (uiContext.imageContext.isSecondaryTextureActive()) * 0.5;
  if (uiContext.imageContext.hasTextureDimensions()) {
    auto scale = std::min<float>((ImGui::GetContentRegionAvail().y) * modif /
                                     uiContext.imageContext.textureHeight,
                                 (ImGui::GetContentRegionAvail().x) /
                                     uiContext.imageContext.textureWidth);
    auto texWidth = uiContext.imageContext.textureWidth * scale;
    auto texHeight = uiContext.imageContext.textureHeight * scale;
    // Handle smooth zooming with Ctrl + mouse wheel
    if (io.KeyCtrl && io.MouseWheel != 0.0f) {
      float zoomFactor = 1.1f; // 10% per wheel notch
      if (io.MouseWheel > 0)
        uiContext.imageContext.zoom *= pow(zoomFactor, io.MouseWheel);
      else
        uiContext.imageContext.zoom /= pow(zoomFactor, -io.MouseWheel);

      // Clamp zoom to reasonable bounds
      uiContext.imageContext.zoom =
          std::clamp(uiContext.imageContext.zoom, 0.98f, 5.0f);
    }

    // Create a child window for the image
    ImGui::BeginChild("ImageContainer", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);
    {
      ImGui::BeginChild("Image", ImVec2(texWidth, texHeight), false,
                        ImGuiWindowFlags_HorizontalScrollbar |
                            ImGuiWindowFlags_AlwaysVerticalScrollbar);
      if (uiContext.imageContext.isPrimaryTextureActive()) {
        ImGui::Image((void *)uiContext.imageContext.primaryTexture,
                     ImVec2(texWidth * uiContext.imageContext.zoom,
                            texHeight * uiContext.imageContext.zoom));

        if (uiContext.imageContext.pixelDescription && ImGui::IsItemHovered()) {
          const auto mouse = ImGui::GetMousePos();
          const auto imageMin = ImGui::GetItemRectMin();
          const auto imageSize = ImGui::GetItemRectSize();
          const auto relativeX = mouse.x - imageMin.x;
          const auto relativeY = mouse.y - imageMin.y;
          if (imageSize.x > 0.0f && imageSize.y > 0.0f && relativeX >= 0.0f &&
              relativeY >= 0.0f && relativeX < imageSize.x &&
              relativeY < imageSize.y) {
            const auto pixelX = std::clamp(
                static_cast<int>(relativeX / imageSize.x *
                                 uiContext.imageContext.textureWidth),
                0, uiContext.imageContext.textureWidth - 1);
            const auto pixelY = std::clamp(
                static_cast<int>(relativeY / imageSize.y *
                                 uiContext.imageContext.textureHeight),
                0, uiContext.imageContext.textureHeight - 1);
            const auto description =
                uiContext.imageContext.pixelDescription(pixelX, pixelY);
            if (!description.empty())
              ImGui::SetTooltip("%s", description.c_str());
          }
        }

        if (io.KeyCtrl && io.MouseWheel) {
          // Get the mouse position relative to the image
          ImVec2 mouse_pos = ImGui::GetMousePos();
          ImVec2 image_pos = ImGui::GetItemRectMin();
          auto itemsize = ImGui::GetItemRectSize();
          ImVec2 mouse_pos_relative =
              ImVec2(mouse_pos.x - image_pos.x, mouse_pos.y - image_pos.y);
          // Calculate the pixel position in the texture
          float pixel_x = ((mouse_pos_relative.x / itemsize.x));
          float pixel_y = ((mouse_pos_relative.y / itemsize.y));
          ImGui::SetScrollHereY(std::clamp(pixel_y, 0.0f, 1.0f));
          ImGui::SetScrollHereX(std::clamp(pixel_x, 0.0f, 1.0f));
        }

        // Handle dragging
        if (ImGui::IsMouseDragging(2, 0.0f)) {
          ImVec2 drag_delta = ImGui::GetMouseDragDelta(2, 0.0f);
          ImGui::ResetMouseDragDelta(2);
          ImGui::SetScrollX(ImGui::GetScrollX() - drag_delta.x);
          ImGui::SetScrollY(ImGui::GetScrollY() - drag_delta.y);
        }
        Fwg::UI::Drawing::imageClick(io, uiContext);
      }
      ImGui::EndChild();

      if (uiContext.imageContext.isSecondaryTextureActive()) {
        ImGui::BeginChild("ImageSecondary", ImVec2(texWidth, texHeight), false,
                          ImGuiWindowFlags_HorizontalScrollbar |
                              ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Image(
            (void *)uiContext.imageContext.secondaryTexture,
            ImVec2(uiContext.imageContext.textureWidth * scale * 0.98,
                   uiContext.imageContext.textureHeight * scale * 0.98));
        ImGui::EndChild();
      }
    }
    ImGui::EndChild();
  }
}

void FwgUI::applyAutoUIScale() {
  static bool styleBackupCreated = false;
  static ImGuiStyle styleBackup;
  static float lastAppliedScale = 1.0f;
  constexpr float baseReferenceHeight = 1369.0f;
  constexpr float scaleStep = 0.25f;
  constexpr float minScale = 1.0f;
  constexpr float maxScale = 4.0f;

  // Create backup of original unscaled style on first call
  if (!styleBackupCreated) {
    styleBackup = ImGui::GetStyle();
    styleBackupCreated = true;
  }

  // Calculate scale based on window height (discrete jumps keep text sharper)
  ImGuiIO &io = ImGui::GetIO();
  float targetScale =
      (io.DisplaySize.y / baseReferenceHeight) * uiScaleMultiplier;
  targetScale = std::max(targetScale, minScale);
  targetScale = std::ceil(targetScale / scaleStep) * scaleStep;
  targetScale = std::clamp(targetScale, minScale, maxScale);
  // Only apply if scale changed (avoid redundant updates)
  if (std::abs(targetScale - lastAppliedScale) > 0.01f) {
    uiContext.layoutContext.uiScale = targetScale;
    ImGuiStyle &style = ImGui::GetStyle();

    // Reset to unscaled style
    style = styleBackup;

    // Apply new scale
    style.ScaleAllSizes(targetScale);
    io.FontGlobalScale = targetScale;

    lastAppliedScale = targetScale;
  }
}

void FwgUI::init(Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  this->uiContext.helpContext.loadHelpTextsFromFile(
      Fwg::Cfg::Values().resourcePath);
  this->uiContext.helpContext.loadHelpImagesFromPath(
      Fwg::Cfg::Values().resourcePath);
  fwg.configure(cfg);
  Fwg::UI::Heightmap::loadHeightmapConfigs(uiContext);
  initAllowedInput(cfg, fwg.climateData, cfg.terrainConfig.landformDefinitions);
}

void FwgUI::defaultTabs(Fwg::Cfg &cfg, FastWorldGenerator &fwg) {
  showElevationTabs(cfg, fwg);
  showClimateTabs(cfg, fwg);
  showAreasTab(cfg, fwg);
}

void FwgUI::computationRunningCheck() {
  if (uiContext.asyncContext.busy()) {
    ImGui::TextUnformatted("Background task is running. Please wait.");
    if (closingAfterTask)
      ImGui::TextUnformatted("Finishing task before closing");
  } else {
    const auto &message = uiContext.asyncContext.statusMessage();
    ImGui::TextUnformatted(message.empty() ? "Ready!" : message.c_str());
  }
}

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *userParam) {
  fprintf(stderr, "GL ERROR: %s\n", message);
}

bool AsyncContext::dispatchPending() {
  if (!pendingTask || worker.valid())
    return false;
  auto task = std::move(pendingTask);
  pendingTask = {};
  try {
    worker = std::async(std::launch::async, std::move(task));
  } catch (const std::exception &e) {
    finish({TaskResult::Outcome::Failure, e.what()});
    return false;
  } catch (...) {
    finish({TaskResult::Outcome::Failure, "Could not start the task"});
    return false;
  }
  return true;
}

bool AsyncContext::pollCompletion() {
  if (!worker.valid() ||
      worker.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
    return false;

  TaskResult result;
  try {
    result = worker.get();
  } catch (const std::exception &e) {
    result = {TaskResult::Outcome::Failure, e.what()};
  } catch (...) {
    result = {TaskResult::Outcome::Failure, "Unknown task exception"};
  }
  finish(std::move(result));
  return true;
}

void AsyncContext::finish(TaskResult result) {
  // This is only called by the UI thread, including launch failures.
  auto onComplete = std::move(completion);
  completion = {};
  if (onComplete) {
    try {
      onComplete(result.outcome);
    } catch (const std::exception &e) {
      result.outcome = TaskResult::Outcome::Failure;
      result.message = e.what();
    } catch (...) {
      result.outcome = TaskResult::Outcome::Failure;
      result.message = "Unknown completion exception";
    }
  }

  if (result.outcome == TaskResult::Outcome::Failure) {
    lastMessage = "Background task failed: " + result.message;
    Fwg::Utils::Logging::logLine(lastMessage);
  } else {
    lastMessage =
        result.message.empty() ? "Background task completed" : result.message;
  }
  computationRunning = false;
}

void AsyncContext::waitForCompletion() {
  if (pendingTask)
    dispatchPending();
  if (worker.valid())
    worker.wait();
  pollCompletion();
}

int FwgUI::shiny(Fwg::FastWorldGenerator &fwg) {
  standaloneGenerator = &fwg;
  return runApplication();
}

int FwgUI::runApplication() {
  if (!Fwg::UI::Utils::CreateDeviceGL(window, windowTitle().c_str(), 0, 0)) {
    Fwg::Utils::Logging::logLine("Could not initialize the OpenGL window");
    return -1;
  }

  bool contextCreated = false;
  bool glfwBackendReady = false;
  bool rendererBackendReady = false;
  int result = 0;
  try {
    Fwg::UI::Utils::setupImGuiContextAndStyle();
    contextCreated = true;
    onWindowReady();
    glfwSetWindowUserPointer(window, this);
    glfwSetDropCallback(
        window, [](GLFWwindow *win, int count, const char **paths) {
          auto *ui = static_cast<FwgUI *>(glfwGetWindowUserPointer(win));
          if (!ui || ui->uiContext.asyncContext.busy() || count <= 0)
            return;
          ui->uiContext.draggedFile = paths[count - 1];
          ui->uiContext.triggeredDrag = true;
        });

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
      throw std::runtime_error("Could not initialize the ImGui GLFW backend");
    glfwBackendReady = true;
    if (!ImGui_ImplOpenGL3_Init("#version 130"))
      throw std::runtime_error("Could not initialize the ImGui OpenGL backend");
    rendererBackendReady = true;

    auto &cfg = Fwg::Cfg::Values();
    init(cfg, currentGenerator());
    auto &io = ImGui::GetIO();
    result = runLoop(io);
  } catch (const std::exception &e) {
    Fwg::Utils::Logging::logLine("Error in GUI: ", e.what());
    result = -1;
  } catch (...) {
    Fwg::Utils::Logging::logLine("Unknown error in GUI");
    result = -1;
  }

  if (uiContext.asyncContext.busy())
    uiContext.asyncContext.waitForCompletion();
  currentGenerator().setImageProgress({});
  uiContext.generationPreview.clear();
  if (rendererBackendReady)
    ImGui_ImplOpenGL3_Shutdown();
  if (glfwBackendReady)
    ImGui_ImplGlfw_Shutdown();
  if (contextCreated)
    ImGui::DestroyContext();
  Fwg::UI::Utils::CleanupDeviceGL(window);
  return result;
}

int FwgUI::runLoop(ImGuiIO &io) {
  auto &cfg = Fwg::Cfg::Values();
  auto lastPreview = std::chrono::steady_clock::now();
  closingAfterTask = false;

  while (true) {
    if (!uiContext.asyncContext.busy())
      uiContext.triggeredDrag = false;
    glfwPollEvents();

    if (uiContext.asyncContext.pollCompletion()) {
      currentGenerator().setImageProgress({});
      uiContext.generationPreview.clear();
      uiContext.imageContext.resetTexture();
    }

    if (glfwWindowShouldClose(window)) {
      if (uiContext.asyncContext.busy()) {
        closingAfterTask = true;
        glfwSetWindowShouldClose(window, GLFW_FALSE);
      } else {
        break;
      }
    }
    if (closingAfterTask && !uiContext.asyncContext.busy())
      break;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    applyAutoUIScale();

    const auto now = std::chrono::steady_clock::now();
    if (uiContext.asyncContext.busy() &&
        now - lastPreview >= std::chrono::milliseconds(50)) {
      Gfx::Image snapshot;
      if (uiContext.generationPreview.consume(snapshot))
        uiContext.imageContext.updateImage(0, snapshot);
      lastPreview = now;
    }

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y});
    ImGui::Begin(frameTitle().c_str());
    renderTopBar();
    ImGui::BeginChild("LeftContent",
                      ImVec2(ImGui::GetContentRegionAvail().x *
                                 uiContext.layoutContext.leftColumnWidth,
                             ImGui::GetContentRegionAvail().y),
                      false);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(78, 90, 204, 40));
    ImGui::BeginChild("SettingsContent",
                      ImVec2(ImGui::GetContentRegionAvail().x,
                             ImGui::GetContentRegionAvail().y * 0.8f),
                      false);
    ImGui::SeparatorText("Different Steps of the generation, usually go "
                         "from left to right");
    // Do not submit an empty tab bar while busy: ImGui would forget its tabs.
    if (!uiContext.asyncContext.busy() &&
        Fwg::UI::Elements::BeginMainTabBar("Steps")) {
      renderSettingsTabs(cfg);
      Fwg::UI::Elements::EndMainTabBar();
    }
    ImGui::PopStyleColor();
    ImGui::EndChild();
    const ImVec2 childMin = ImGui::GetItemRectMin();
    const ImVec2 childMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRect(
        childMin, childMax, IM_COL32(100, 90, 180, 255), 0.0f, 0, 2.0f);

    computationRunningCheck();
    if (!uiContext.asyncContext.busy())
      renderGenericControls(cfg);
    logWrapper();
    ImGui::EndChild();
    ImGui::SameLine();
    if (uiContext.asyncContext.busy())
      uiContext.drawContext.clickEvents = {};
    imageWrapper(io);
    ImGui::End();

    if (uiContext.helpContext.showExtendedHelp)
      uiContext.helpContext.showAdvancedTextBox();

    ImGui::Render();
    int displayWidth = 0;
    int displayHeight = 0;
    glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight);
    glClearColor(0.45f, 0.55f, 0.60f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);

    if (uiContext.asyncContext.hasPendingTask()) {
      uiContext.generationPreview.clear();
      currentGenerator().setImageProgress(
          [this](const Gfx::Image &image, std::span<const int> pixels) {
            uiContext.generationPreview.publish(image, pixels);
          });
      uiContext.asyncContext.dispatchPending();
      if (!uiContext.asyncContext.busy())
        currentGenerator().setImageProgress({});
    }
  }
  return 0;
}

void FwgUI::initAllowedInput(
    Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
    std::vector<Terrain::LandformDefinition> &landformDefinitions) {
  Fwg::Utils::Logging::logLine("Initialising allowed input");
  auto &climateClassDefinitions = cfg.climateConfig.climateClassDefinitions;
  uiContext.climateUI.allowedClimateInputs.clear();
  for (const auto &climateType : climateClassDefinitions) {
    uiContext.climateUI.allowedClimateInputs.setValue(climateType.primaryColour,
                                                      climateType);
  }

  landUI.allowedLandInputs.clear();
  for (const auto &landformDefinition : landformDefinitions) {
    landUI.allowedLandInputs.setValue(landformDefinition.colour,
                                      landformDefinition);
  }
}

int FwgUI::showFwgConfigure(Fwg::Cfg &cfg) {
  if (UI::Elements::BeginSubTabItem("Fwg config")) {
    // remove the images, and set pretext for them to be auto loaded after
    // switching tabs again
    uiContext.tabSwitchEvent();
    PushItemWidthScaled(200.0f);
    ImGui::InputInt("Width", &cfg.width);
    ImGui::InputInt("Height", &cfg.height);
    ImGui::PopItemWidth();
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showGeneric(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  PushItemWidthScaled(200.0f);
  if (ImGui::InputInt("<--Seed", &cfg.mapSeed)) {
    cfg.randomSeed = false;
    cfg.reRandomize();
  }
  ImGui::SameLine();
  if (ImGui::Button("Get random seed")) {
    cfg.randomSeed = true;
    cfg.reRandomize();
  }
  ImGui::SameLine();
  if (cfg.debugLevel > 5 && ImGui::Button("Display size")) {
    Fwg::Utils::Logging::logLine(fwg.size());
  }
  ImGui::SameLine();
  if (cfg.debugLevel > 5 && ImGui::Button("Clear")) {
    fwg.resetData();
    Fwg::Utils::Logging::logLine(fwg.size());
  }
  if (ImGui::Button(("Save current image to " + cfg.mapsPath).c_str())) {
    writeCurrentlyDisplayedImage(cfg);
  }
  ImGui::SameLine();
  ImGui::InputInt("<--Debug level", &cfg.debugLevel);

  // Show current UI scale in debug mode
  if (cfg.debugLevel > 5) {
    ImGui::Text("UI Scale: %.2fx (auto-adjusted for %dx%d)",
                uiContext.layoutContext.uiScale,
                (int)ImGui::GetIO().DisplaySize.x,
                (int)ImGui::GetIO().DisplaySize.y);
  }

  if (ImGui::SliderFloat("UI scale multiplier", &uiScaleMultiplier, 1.0f, 2.0f,
                         "%.2fx")) {
    uiScaleMultiplier = std::clamp(uiScaleMultiplier, 1.0f, 2.0f);
  }

  if (ImGui::Button("Generate all fwg data")) {
    // run the generation async
    uiContext.asyncContext.runAsync(
        [&fwg]() { return fwg.generateWorld(); },
        [this](TaskResult::Outcome) {
          uiContext.generationContext.modifiedAreas = true;
        });
  }
  ImGui::Separator();
  if (ImGui::Button("Save project")) {
    auto path = cfg.mapsPath + "fwg.data";
    auto configPath = path + ".config";
    try {
      fwg.save(path);
      if (fwg.saveConfigurationSnapshot(configPath, cfg)) {
        Fwg::Utils::Logging::logLine("Project and config saved to ", path,
                                     " and ", configPath);
      } else {
        Fwg::Utils::Logging::logLine(
            "Project data saved, but config snapshot save failed: ",
            configPath);
      }
    } catch (std::exception &e) {
      Fwg::Utils::Logging::logLine("Save failed: ", e.what());
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Load project")) {
    auto path = cfg.mapsPath + "fwg.data";
    auto configPath = path + ".config";
    try {
      if (fwg.loadConfigurationSnapshot(configPath, cfg)) {
        fwg.load(path);
        uiContext.generationContext.modifiedAreas = true;
        initAllowedInput(cfg, fwg.climateData,
                         cfg.terrainConfig.landformDefinitions);
        uiContext.imageContext.resetTexture();
        Fwg::Utils::Logging::logLine("Project and config loaded from ", path,
                                     " and ", configPath);
      }
    } catch (std::exception &e) {
      Fwg::Utils::Logging::logLine("Load failed: ", e.what());
    }
  }
  ImGui::PopItemWidth();
  return true;
}

int FwgUI::showElevationTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {

  if (UI::Elements::BeginMainTabItem("Land Tabs")) {
    uiContext.tabSwitchEvent();
    if (UI::Elements::BeginSubTabBar("Land Tabs", 0.0f)) {
      landUI.showLandInputTab(cfg, fwg, uiContext);
      Fwg::UI::Heightmap::showHeightmapTab(cfg, fwg, uiContext);
      Fwg::UI::Heightmap::showNormalMapTab(cfg, fwg, uiContext);
      UI::Elements::EndSubTabBar();
    }
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showClimateTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  enum class ClimateMode { DEFAULT, FANTASY, INPUT };
  static ClimateMode climateMode = ClimateMode::DEFAULT;
  if (UI::Elements::BeginMainTabItem("Climate Tabs")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.resetTexture();
    }
    uiContext.helpContext.showHelpTextBox("Climate");

    // === MODE SELECTOR ===
    ImGui::SeparatorText("Climate Generation Mode");

    if (ImGui::RadioButton("Generate Simulated Climate",
                           reinterpret_cast<int *>(&climateMode),
                           static_cast<int>(ClimateMode::DEFAULT))) {
      cfg.climateConfig.fantasyClimate = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Generate Random Climate",
                           reinterpret_cast<int *>(&climateMode),
                           static_cast<int>(ClimateMode::FANTASY))) {
      cfg.climateConfig.fantasyClimate = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Use Climate Input Map",
                           reinterpret_cast<int *>(&climateMode),
                           static_cast<int>(ClimateMode::INPUT))) {
      cfg.climateConfig.fantasyClimate = false;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SeparatorText("Base Climate Parameters");
    {
      UI::Elements::GridLayout grid(2, scaled(200.0f), scaled(12.0f));
      grid.AddInputDouble("Polar annual mean (°C)",
                          &cfg.climateConfig.polarAnnualMeanCelsius, -100.0,
                          80.0);
      grid.AddInputDouble("Equatorial annual mean (°C)",
                          &cfg.climateConfig.equatorialAnnualMeanCelsius,
                          -100.0, 80.0);
    }

    if (climateMode == ClimateMode::DEFAULT ||
        climateMode == ClimateMode::FANTASY) {
      {
        UI::Elements::GridLayout grid(2, scaled(200.0f), scaled(12.0f));
        grid.AddInputDouble("Base Humidity", &cfg.climateConfig.baseHumidity,
                            0.0, 100.0);
      }

      ImGui::Spacing();
      ImGui::SeparatorText("Latitude & River Settings");

      {
        UI::Elements::GridLayout grid(2, scaled(200.0f), scaled(12.0f));

        grid.AddInputDouble("Northern latitude (°)", &cfg.latitudeNorthDegrees,
                            -90.0, 90.0);
        grid.AddInputDouble("Southern latitude (°)", &cfg.latitudeSouthDegrees,
                            -90.0, 90.0);
        grid.AddInputDouble("River Amount", &cfg.riverConfig.riverFactor, 0.0,
                            10.0);
        grid.AddInputDouble("River Humidity",
                            &cfg.climateConfig.riverHumidityFactor, 0.0, 10.0);
        grid.AddInputDouble("River Range",
                            &cfg.climateConfig.riverEffectRangeFactor, 0.0,
                            10.0);
        grid.AddCheckbox("Carve River Canyons", &cfg.riverConfig.canyonEnabled);
        grid.AddInputInt("Canyon Range (pixels)", &cfg.riverConfig.canyonRange,
                         0, 64);
      }

      ImGui::Spacing();

      // Scoped guard - only affects the automation button
      {
        auto guard = UI::PrerequisiteChecker::require(
            {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
             UI::PrerequisiteChecker::landforms(fwg.terrainData),
             UI::PrerequisiteChecker::landMask(fwg.terrainData)});
        if (guard.ready() && UI::Elements::AutomationStepButton(
                                 "Generate whole climate automatically")) {
          uiContext.asyncContext.runAsync([&fwg, &cfg]() {
            return fwg.genTemperatures(cfg) && fwg.genHumidity(cfg) &&
                   fwg.genRivers(cfg) && fwg.genClimate(cfg) &&
                   fwg.genWorldMap(cfg);
          });
        }
      }
    }

    if (climateMode == ClimateMode::FANTASY) {
      UI::Elements::GridLayout grid(2, scaled(200.0f), scaled(12.0f));
      grid.AddInputDouble("Fantasy Frequency",
                          &cfg.climateConfig.fantasyClimateFrequency, 0.0,
                          10.0);
    }

    ImGui::InputFloat("Climate Noise Frequency",
                      &cfg.climateConfig.noiseFrequencyModifier);
    ImGui::InputDouble("Climate Noise Edge Limit",
                       &cfg.climateConfig.noiseEdgeLimitFactor);

    if (UI::Elements::BeginSubTabBar("Climate Generation", 0.0f)) {
      if (climateMode == ClimateMode::DEFAULT ||
          climateMode == ClimateMode::FANTASY) {
        Fwg::UI::Climate::showTemperatureMap(cfg, fwg, uiContext);
        Fwg::UI::Climate::showHumidityTab(cfg, fwg, uiContext);
      } else {
        Fwg::UI::Climate::Input::showClimateInputTab(cfg, fwg, uiContext);
      }
      Fwg::UI::Climate::showRiverTab(cfg, fwg, uiContext);
      Fwg::UI::Climate::showClimateZonesTab(cfg, fwg, uiContext);
      Fwg::UI::Climate::showTreeTab(cfg, fwg, uiContext);
      UI::Elements::EndSubTabBar();
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showAreasTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {

  if (UI::Elements::BeginMainTabItem("Areas")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.resetTexture();
    }
    {
      auto guard = UI::PrerequisiteChecker::require(
          {UI::PrerequisiteChecker::climate(fwg.climateData),
           UI::PrerequisiteChecker::landforms(fwg.terrainData)});

      if (guard.ready() && UI::Elements::AutomationStepButton(
                               "Generate all areas automatically")) {
        uiContext.asyncContext.runAsync(
            [&fwg, &cfg]() {
              return fwg.genHabitability(cfg) && fwg.genSuperSegments(cfg) &&
                     fwg.genSegments(cfg) && fwg.genProvinces(cfg) &&
                     fwg.genContinents(cfg);
            },
            [this](TaskResult::Outcome) {
              uiContext.generationContext.modifiedAreas = true;
            });
      }
    }
    Fwg::UI::Areas::areaInputSelector(cfg);

    if (UI::Elements::BeginSubTabBar("Area Tabs", 0.0f)) {
      Fwg::UI::Areas::showDensityTab(cfg, fwg, uiContext);
      Fwg::UI::Areas::showSuperSegmentTab(cfg, fwg, uiContext);
      Fwg::UI::Areas::showSegmentTab(cfg, fwg, uiContext);
      Fwg::UI::Areas::showProvincesTab(cfg, fwg, uiContext);
      Fwg::UI::Areas::showContinentTab(cfg, fwg, uiContext);
      UI::Elements::EndSubTabBar();
    }

    ImGui::EndTabItem();
  }
  return 0;
}

} // namespace Fwg::UI
