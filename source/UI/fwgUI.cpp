#include "UI/FwgUI.h"

namespace Fwg {
// Data
int g_ResizeWidth = 0;
int g_ResizeHeight = 0;
int FwgUI::seed = 0;
static bool analyze = false;
static int amountClassificationsNeeded = 0;

FwgUI::FwgUI() {

  uiUtils = std::make_shared<UIUtils>();
  landUI = LandUI(uiUtils);
}

bool FwgUI::initializeGraphics() {
  if (!glfwInit())
    return false;

  // Request a modern GL core context
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  window = glfwCreateWindow(1280, 720, "FastWorldGen", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return false;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // vsync

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return false;

  return true;
}

void FwgUI::initializeImGui() {
  uiUtils->setupImGuiContextAndStyle();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 450");
}

void FwgUI::genericWrapper(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 100, 144, 40));

    ImGui::BeginChild(
        "GenericWrapper",
        ImVec2(
            ImGui::GetContentRegionAvail().x * 1.0f,
            std::max<float>(ImGui::GetContentRegionAvail().y * 0.3f, 100.0f)),
        false, ImGuiWindowFlags_None);
    showGeneric(cfg, fwg);
    ImGui::EndChild();
    // Draw a frame around the child region
    ImVec2 childMin = ImGui::GetItemRectMin();
    ImVec2 childMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRect(
        childMin, childMax, IM_COL32(50, 91, 120, 255), 0.0f, 0, 2.0f);
    ImGui::PopStyleColor();
  }
}

void FwgUI::logWrapper() {
  ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(30, 100, 144, 40));
  ImGui::BeginChild("Log",
                    ImVec2(ImGui::GetContentRegionAvail().x * 1.0f,
                           ImGui::GetContentRegionAvail().y * 1.0f),
                    false, ImGuiWindowFlags_None);
  {
    ImGui::TextUnformatted(log->str().c_str());
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

  float modif = 1.0 - (uiUtils->secondaryTexture != 0) * 0.5;
  if (uiUtils->textureWidth > 0 && uiUtils->textureHeight > 0) {
    float aspectRatio =
        (float)uiUtils->textureWidth / (float)uiUtils->textureHeight;
    auto scale = std::min<float>(
        (ImGui::GetContentRegionAvail().y) * modif / uiUtils->textureHeight,
        (ImGui::GetContentRegionAvail().x) / uiUtils->textureWidth);
    auto texWidth = uiUtils->textureWidth * scale;
    auto texHeight = uiUtils->textureHeight * scale;

    // Handle smooth zooming with Ctrl + mouse wheel
    if (io.KeyCtrl && io.MouseWheel != 0.0f) {
      float zoomFactor = 1.1f; // 10% per wheel notch
      if (io.MouseWheel > 0)
        zoom *= pow(zoomFactor, io.MouseWheel);
      else
        zoom /= pow(zoomFactor, -io.MouseWheel);

      // Clamp zoom to reasonable bounds
      zoom = std::clamp(zoom, 0.98f, 5.0f);
    }

    // Create a child window for the image
    ImGui::BeginChild("ImageContainer", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);
    {
      ImGui::BeginChild("Image", ImVec2(texWidth, texHeight), false,
                        ImGuiWindowFlags_HorizontalScrollbar |
                            ImGuiWindowFlags_AlwaysVerticalScrollbar);
      if (uiUtils->primaryTexture != 0) {
        ImGui::Image((void *)uiUtils->primaryTexture,
                     ImVec2(texWidth * zoom, texHeight * zoom));

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
        uiUtils->imageClick(scale, io);
      }
      ImGui::EndChild();

      if (uiUtils->secondaryTexture != 0) {
        ImGui::BeginChild("ImageSecondary", ImVec2(texWidth, texHeight), false,
                          ImGuiWindowFlags_HorizontalScrollbar |
                              ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::Image((void *)uiUtils->secondaryTexture,
                     ImVec2(uiUtils->textureWidth * scale * 0.98,
                            uiUtils->textureHeight * scale * 0.98));
        ImGui::EndChild();
      }
    }
    ImGui::EndChild();
  }
}

void FwgUI::init(Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  this->uiUtils->loadHelpTextsFromFile(Fwg::Cfg::Values().resourcePath);
  this->uiUtils->loadHelpImagesFromPath(Fwg::Cfg::Values().resourcePath);
  uiUtils->setClickOffsets(cfg.width, 1);
  frequency = cfg.overallFrequencyModifier;
  log = std::make_shared<std::stringstream>();
  *log << Fwg::Utils::Logging::Logger::logInstance.getFullLog();
  Fwg::Utils::Logging::Logger::logInstance.attachStream(log);
  fwg.configure(cfg);
  loadHeightmapConfigs();
  initAllowedInput(cfg, fwg.climateData, cfg.terrainConfig.landformDefinitions);
}

void FwgUI::defaultTabs(Fwg::Cfg &cfg, FastWorldGenerator &fwg) {
  showElevationTabs(cfg, fwg);
  showClimateInputTab(cfg, fwg);
  showClimateOverview(cfg, fwg);
  showAreasTab(cfg, fwg);
}

void FwgUI::computationRunningCheck() {
  // Check if the computation is done
  if (computationRunning && computationFutureBool.wait_for(std::chrono::seconds(
                                0)) == std::future_status::ready) {
    computationRunning = false;
    uiUtils->resetTexture();
  }

  if (computationRunning) {
    computationStarted = false;
    ImGui::Text("Working, please be patient");
  } else {
    ImGui::Text("Ready!");
  }
}

void FwgUI::cleanup() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();
}

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id,
                            GLenum severity, GLsizei length,
                            const GLchar *message, const void *userParam) {
  fprintf(stderr, "GL ERROR: %s\n", message);
}

int FwgUI::shiny(Fwg::FastWorldGenerator &fwg) {
  Fwg::Utils::Logging::logLine("Starting GUI init");

  // Initialize GLFW, window, GLAD, ImGui
  if (!glfwInit()) {
    Fwg::Utils::Logging::logLine("Starting GUI init FAILED");
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(1280, 720, "FastWorldGen 0.9.1", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // vsync

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return -1;

  uiUtils->setupImGuiContextAndStyle();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 450");

  glfwSetWindowUserPointer(window, this);
  glfwSetDropCallback(
      window, [](GLFWwindow *win, int count, const char **paths) {
        auto *fwgui = reinterpret_cast<FwgUI *>(glfwGetWindowUserPointer(win));
        fwgui->triggeredDrag = (count > 0);
        fwgui->draggedFile = (count > 0) ? std::string(paths[count - 1]) : "";
      });
  // glEnable(GL_DEBUG_OUTPUT);
  // glDebugMessageCallback(DebugCallback, nullptr);

  auto &cfg = Fwg::Cfg::Values();
  auto &io = uiUtils->setupImGuiContextAndStyle();
  init(cfg, fwg);

  while (!glfwWindowShouldClose(window)) {
    triggeredDrag = false;
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
      ImGui::SetNextWindowPos({0, 0});
      ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y});
      ImGui::Begin("FastWorldGen");
      // observer checks for "Error"

      ImGui::BeginChild("LeftContent",
                        ImVec2(ImGui::GetContentRegionAvail().x * 0.4f,
                               ImGui::GetContentRegionAvail().y * 1.0f),
                        false);
      {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(78, 90, 204, 40));
        // Create a child window for the left content
        ImGui::BeginChild("SettingsContent",
                          ImVec2(ImGui::GetContentRegionAvail().x * 1.0f,
                                 ImGui::GetContentRegionAvail().y * 0.8f),
                          false);
        {
          ImGui::SeparatorText("Different Steps of the generation, usually go "
                               "from left to right");

          if (UI::Elements::BeginMainTabBar("Steps")) {
            // Disable all inputs if computation is running
            if (computationRunning) {
              ImGui::BeginDisabled();
            }

            defaultTabs(cfg, fwg);
            // Re-enable inputs if computation is running
            if (computationRunning && !computationStarted) {
              ImGui::EndDisabled();
            }
            computationRunningCheck();

            UI::Elements::EndMainTabBar();
          }

          ImGui::PopStyleColor();
          ImGui::EndChild();

          // Draw a frame around the child region
          ImVec2 childMin = ImGui::GetItemRectMin();
          ImVec2 childMax = ImGui::GetItemRectMax();
          ImGui::GetWindowDrawList()->AddRect(
              childMin, childMax, IM_COL32(100, 90, 180, 255), 0.0f, 0, 2.0f);
        }
        genericWrapper(cfg, fwg);
        logWrapper();
      }
      ImGui::SameLine();
      imageWrapper(io);

      ImGui::End();
    }

    // Render
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  CleanupDeviceGL();
  return 0;
}

void FwgUI::initAllowedInput(
    Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
    std::vector<Terrain::LandformDefinition> &landformDefinitions) {
  Fwg::Utils::Logging::logLine("Initialising allowed input");
  auto &climateClassDefinitions = climateData.climateClassDefinitions;
  climateUI.allowedClimateInputs.clear();
  for (const auto &climateType : climateClassDefinitions) {
    climateUI.allowedClimateInputs.setValue(climateType.primaryColour,
                                            climateType);
  }

  for (const auto &landformDefinition : landformDefinitions) {
    landUI.allowedLandInputs.setValue(landformDefinition.colour,
                                      landformDefinition);
  }
}

void FwgUI::loadHeightmapConfigs() {
  auto &cfg = Fwg::Cfg::Values();
  auto heightmapConfigFolder = cfg.workingDirectory + "configs/heightmap/";
  // gather all files in the working heightmapConfigFolder
  for (const auto &entry :
       std::filesystem::directory_iterator(heightmapConfigFolder)) {
    if (!entry.is_directory() && entry.path().string().contains(".json")) {
      heightmapConfigFiles.push_back(entry.path().string());
      Fwg::Utils::Logging::logLine("Found heightmap config: ", entry);
    }
  }
}
int FwgUI::showFwgConfigure(Fwg::Cfg &cfg) {
  if (UI::Elements::BeginSubTabItem("Fwg config")) {
    // remove the images, and set pretext for them to be auto loaded after
    // switching tabs again
    uiUtils->tabSwitchEvent();
    ImGui::PushItemWidth(200.0f);
    ImGui::InputInt("Width", &cfg.width);
    ImGui::InputInt("Height", &cfg.height);
    ImGui::PopItemWidth();
    ImGui::EndTabItem();
  }
  return 0;
}
// display cut configuration options
int FwgUI::showCutCfg(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  ImGui::PushItemWidth(200.0f);
  ImGui::Checkbox("Cut from input", &cfg.cut);
  if (cfg.cut) {
    ImGui::SameLine();
    ImGui::InputInt("MinX", &cfg.minX, 1, 64);
    ImGui::SameLine();
    ImGui::InputInt("MinY", &cfg.minY, 1, 64);
    ImGui::SameLine();
    ImGui::InputInt("MaxX", &cfg.maxX, 1, 64);
    ImGui::SameLine();
    ImGui::InputInt("MaxY", &cfg.maxY, 1, 64);
    ImGui::Checkbox("Scale", &cfg.scale);
    ImGui::SameLine();
    ImGui::InputInt("ScaleX", &cfg.scaleX, 64, 256);
    ImGui::SameLine();
    ImGui::InputInt("ScaleY", &cfg.scaleY, 64, 256);
    ImGui::SameLine();
    ImGui::Checkbox("Keep Ratio", &cfg.keepRatio);
  }
  ImGui::PopItemWidth();
  return 0;
}

int FwgUI::showGeneric(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  ImGui::PushItemWidth(200.0f);
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
  if (ImGui::Button("Generate all fwg data")) {
    fwg.resetData();
    // reset this because now we randomly generate all data, so heightmap
    // modifications MUST be allowed again
    cfg.allowHeightmapModification = true;

    // run the generation async
    computationFutureBool = runAsyncInitialDisable([&fwg, &cfg, this]() {
      fwg.generateWorld();
      uiUtils->resetTexture();
      modifiedAreas = true;
      return true;
    });
  }
  ImGui::PopItemWidth();
  return true;
}

void FwgUI::areaInputSelector(Fwg::Cfg &cfg) {
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

int FwgUI::showElevationTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {

  if (UI::Elements::BeginMainTabItem("Land Tabs")) {
    uiUtils->tabSwitchEvent();
    if (UI::Elements::BeginSubTabBar("Land Tabs", 0.0f)) {
      showLandTab(cfg, fwg);
      showHeightmapTab(cfg, fwg);
      showNormalMapTab(cfg, fwg);
      UI::Elements::EndSubTabBar();
    }
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showLandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Land Input")) {
    if (uiUtils->tabSwitchEvent(true)) {
      uiUtils->updateImage(0, landUI.landInput);
      uiUtils->updateImage(1, Fwg::Gfx::Image());
    }
    uiUtils->showHelpTextBox("Land");
    //  Selection of land input mode
    ImGui::TextUnformatted("Land Input Mode");
    ImGui::RadioButton("Heightmap. A finished heightmap",
                       cfg.landInputMode == Fwg::Terrain::InputMode::HEIGHTMAP);
    if (ImGui::IsItemClicked()) {
      cfg.landInputMode = Fwg::Terrain::InputMode::HEIGHTMAP;
      cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
                              "default.json");
    }
    ImGui::RadioButton("Heightmap Sketch. A rough outline of a heightmap",
                       cfg.landInputMode ==
                           Fwg::Terrain::InputMode::HEIGHTSKETCH);
    if (ImGui::IsItemClicked()) {
      cfg.landInputMode = Fwg::Terrain::InputMode::HEIGHTSKETCH;
      cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
                              "mappedInput.json");
    }
    ImGui::RadioButton("Land Mask. A simple land/water mask",
                       cfg.landInputMode == Fwg::Terrain::InputMode::LANDMASK);
    if (ImGui::IsItemClicked()) {
      cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
                              "default.json");
      cfg.landInputMode = Fwg::Terrain::InputMode::LANDMASK;
    }
    ImGui::RadioButton(
        "Landform. A detailed input map of various colours, that can be mapped "
        "to mountains, lakes, valleys, plains, and more.",
        cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM);
    if (ImGui::IsItemClicked()) {
      cfg.landInputMode = Fwg::Terrain::InputMode::LANDFORM;
      cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
                              "mappedInput.json");
    }
    if (cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM &&
        landUI.landInput.size()) {
      landUI.complexLandMapping(cfg, fwg, analyze, amountClassificationsNeeded);
    }

    if (triggeredDrag) {
      cfg.allowHeightmapModification = true;
      originalLandInput = draggedFile;
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        landUI.triggeredLandInput(cfg, fwg, originalLandInput,
                                  cfg.landInputMode);
        // after the first drag, we have saved the original input to this new
        // file now we always want to reload it from here to get the progressive
        // changes, never overwriting the original
        originalLandInput = cfg.mapsPath + "/classifiedLandInput.png";
        // in case of complex input and a drag, we NEED to initially analyze
        if (cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM) {
          analyze = true;
        }
        triggeredDrag = false;
        return true;
      });
    }
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showHeightmapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  static bool updateLayer = false;
  static int selectedLayer = 0;
  static bool rerandomiseSeed = false;
  static int layerTypeSelection = 0; // 0=Shape, 1=Land, 2=Sea
  static int previousLayerTypeSelection = 0;

  if (UI::Elements::BeginSubTabItem("Heightmap")) {
    if (uiUtils->tabSwitchEvent()) {
      if (fwg.terrainData.detailedHeightMap.size()) {
        auto heightmap =
            Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap);
        uiUtils->updateImage(0, heightmap);
        uiUtils->updateImage(1, heightmap);

        if (updateLayer) {
          auto &selectedLayers =
              (layerTypeSelection == 0)   ? fwg.terrainData.shapeLayers
              : (layerTypeSelection == 1) ? fwg.terrainData.landLayers
                                          : fwg.terrainData.seaLayers;
          if (selectedLayer < selectedLayers.size() &&
              selectedLayers[selectedLayer].size() &&
              fwg.terrainData.detailedHeightMap.size()) {
            uiUtils->updateImage(
                1, Fwg::Gfx::Image(cfg.width, cfg.height, 24,
                                   selectedLayers[selectedLayer]));
          }
          updateLayer = false;
        } else {
          if (fwg.terrainData.landMask.size()) {
            uiUtils->updateImage(1, Fwg::Gfx::landFormMap(fwg.terrainData));
          } else {
            uiUtils->updateImage(1, Fwg::Gfx::Image());
          }
        }
      } else {
        uiUtils->updateImage(0, Fwg::Gfx::Image());
        uiUtils->updateImage(1, Fwg::Gfx::Image());
      }
    }

    uiUtils->showHelpTextBox("Heightmap");

    ImGui::SeparatorText("Heightmap Configuration Presets");

    // Heightmap preset selection with auto-initialization
    static int activeConfigIndex = -1;
    static bool initializedDefault = false;

    // Initialize with default config on first run
    if (!initializedDefault && heightmapConfigFiles.size() > 0) {
      // Try to find "default.json" in the list
      for (int i = 0; i < heightmapConfigFiles.size(); ++i) {
        if (heightmapConfigFiles[i].find("default.json") != std::string::npos) {
          activeConfigIndex = i;
          break;
        }
      }
      // If "default.json" not found, use first config
      if (activeConfigIndex == -1) {
        activeConfigIndex = 0;
      }
      initializedDefault = true;
    }

    ImGui::Text("Select Preset:");
    ImGui::SameLine();

    ImGui::PushItemWidth(300.0f);
    if (ImGui::BeginCombo("##HeightmapPresets",
                          activeConfigIndex >= 0 &&
                                  activeConfigIndex <
                                      heightmapConfigFiles.size()
                              ? heightmapConfigFiles[activeConfigIndex].c_str()
                              : "Select a preset...")) {
      for (int i = 0; i < heightmapConfigFiles.size(); ++i) {
        bool isActive = (i == activeConfigIndex);
        if (ImGui::Selectable(heightmapConfigFiles[i].c_str(), isActive)) {
          activeConfigIndex = i;
          cfg.readHeightmapConfig(heightmapConfigFiles[i]);
        }
        if (isActive)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::SeparatorText("Basic Parameters");

    // Main parameters grid
    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      grid.AddText("Sealevel", "%d", cfg.seaLevel);

      if (cfg.landInputMode == Fwg::Terrain::InputMode::HEIGHTMAP) {
        grid.AddSliderFloat("Target Land %", &cfg.landPercentage, 0.0f, 1.0f);
        grid.AddInputInt("Height Adjustments", &cfg.heightAdjustments, 0, 100);
        grid.AddInputFloat("Coastal Distance", &cfg.layerApplicationFactor,
                           0.0f, 10.0f);
      } else {
        grid.AddText("Land Percentage", "%.2f%%", cfg.landPercentage * 100.0f);
        grid.NextRow(); // Skip to next row for alignment
      }

      grid.AddInputDouble("Lake Size Factor", &cfg.lakeMaxShare, 0.0, 1.0);
      if (grid.AddInputInt("Max Land Height", &cfg.maxLandHeight,
                           cfg.seaLevel + 1, 255)) {
        cfg.maxLandHeight =
            std::clamp(cfg.maxLandHeight, cfg.seaLevel + 1, 255);
      }
    }

    ImGui::Spacing();

    landUI.configureLandElevationFactors(cfg, fwg);

    ImGui::Spacing();
    ImGui::SeparatorText("Advanced Heightmap Settings");

    // Advanced settings grid
    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      grid.AddSliderFloat("Heightmap Frequency",
                          &cfg.heightmapFrequencyModifier, 0.1f, 10.0f);
      grid.AddSliderFloat("Width Edge Factor", &cfg.heightmapWidthEdgeModifier,
                          0.0f, 10.0f);
      grid.AddSliderFloat("Height Edge Factor",
                          &cfg.heightmapHeightEdgeModifier, 0.0f, 10.0f);
    }

    ImGui::Spacing();

    // Replace checkbox with collapsing header
    if (ImGui::CollapsingHeader("Layer Editor", ImGuiTreeNodeFlags_None)) {
      ImGui::Spacing();

      // Layer type selector
      ImGui::Text("Layer Type:");
      ImGui::SameLine();
      ImGui::RadioButton("Shape", &layerTypeSelection, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Land", &layerTypeSelection, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Sea", &layerTypeSelection, 2);

      // Reset selection when switching layer types
      if (previousLayerTypeSelection != layerTypeSelection) {
        selectedLayer = 0;
        updateLayer = true;
        uiUtils->resetTexture(1);
        previousLayerTypeSelection = layerTypeSelection;
      }

      // Get current layer vector
      std::vector<LayerConfig> *currentLayers = nullptr;
      const char *layerTypeName = "";

      switch (layerTypeSelection) {
      case 0:
        currentLayers = &cfg.shapeLayers;
        layerTypeName = "Shape";
        break;
      case 1:
        currentLayers = &cfg.landLayers;
        layerTypeName = "Land";
        break;
      case 2:
        currentLayers = &cfg.seaLayers;
        layerTypeName = "Sea";
        break;
      }

      ImGui::Text("%s Layers: %zu", layerTypeName, currentLayers->size());
      ImGui::Spacing();

      // Layer selection and editing in two columns
      {
        ImGui::BeginChild("LayerSelection",
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.25f,
                                 ImGui::GetContentRegionAvail().y * 0.6f),
                          true, ImGuiWindowFlags_None);

        ImGui::TextUnformatted("Layer List");
        ImGui::Separator();

        for (int i = 0; i < currentLayers->size(); i++) {
          char label[64];
          snprintf(label, sizeof(label), "%s Layer %d", layerTypeName, i);

          ImGui::PushID(i);
          if (ImGui::Selectable(label, selectedLayer == i)) {
            selectedLayer = i;
            updateLayer = true;
            uiUtils->resetTexture(1);
          }
          ImGui::PopID();
        }

        ImGui::Separator();

        if (ImGui::Button("Add Layer", ImVec2(-1, 0))) {
          LayerConfig newLayer{};
          newLayer.noiseType = 3;
          newLayer.fractalType = 3;
          newLayer.type = static_cast<LayerType>(layerTypeSelection);
          newLayer.fractalFrequency = 0.5f;
          newLayer.fractalOctaves = 11;
          newLayer.fractalGain = 0.5f;
          newLayer.seed = RandNum::getRandom<int>();
          newLayer.widthEdge = 10.0;
          newLayer.heightEdge = 0.0;
          newLayer.weight = 1.0;
          newLayer.altitudeWeightStart = 0.0;
          newLayer.altitudeWeightEnd = 0.0;
          newLayer.minHeight = 0;
          newLayer.maxHeight = 100;
          newLayer.tanFactor = 0.0;

          currentLayers->push_back(newLayer);
          selectedLayer = currentLayers->size() - 1;
        }

        if (currentLayers->size() > 0 &&
            ImGui::Button("Remove Layer", ImVec2(-1, 0))) {
          if (selectedLayer < currentLayers->size()) {
            currentLayers->erase(currentLayers->begin() + selectedLayer);
            selectedLayer = std::max(0, selectedLayer - 1);
            updateLayer = true;
          }
        }

        ImGui::EndChild();
      }

      ImGui::SameLine();

      // Layer properties editor
      {
        ImGui::BeginChild("LayerEdit",
                          ImVec2(ImGui::GetContentRegionAvail().x * 1.0f,
                                 ImGui::GetContentRegionAvail().y * 0.6f),
                          true, ImGuiWindowFlags_None);

        if (selectedLayer < currentLayers->size()) {
          LayerConfig &layer = (*currentLayers)[selectedLayer];

          ImGui::Text("Editing %s Layer %d", layerTypeName, selectedLayer);
          ImGui::Separator();
          ImGui::Spacing();

          // Noise settings grid
          {
            UI::Elements::GridLayout grid(2, 180.0f, 8.0f);

            static const char *NoiseTypeNames[] = {
                "OpenSimplex2", "OpenSimplex2S", "Cellular",
                "Perlin",       "ValueCubic",    "Value"};

            static const char *FractalTypeNames[] = {"None",
                                                     "FBm",
                                                     "Ridged",
                                                     "PingPong",
                                                     "DomainWarpProgressive",
                                                     "DomainWarpIndependent"};

            // Clamp indices
            layer.noiseType = std::clamp(
                layer.noiseType, 0, (int)(IM_ARRAYSIZE(NoiseTypeNames)) - 1);
            layer.fractalType =
                std::clamp(layer.fractalType, 0,
                           (int)(IM_ARRAYSIZE(FractalTypeNames)) - 1);

            // Show combo boxes inline
            ImGui::PushItemWidth(180.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%-*s", 25, "Noise Type");
            ImGui::SameLine();
            ImGui::Combo("##NoiseType", &layer.noiseType, NoiseTypeNames,
                         IM_ARRAYSIZE(NoiseTypeNames));

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%-*s", 25, "Fractal Type");
            ImGui::SameLine();
            ImGui::Combo("##FractalType", &layer.fractalType, FractalTypeNames,
                         IM_ARRAYSIZE(FractalTypeNames));
            ImGui::PopItemWidth();

            grid.AddInputFloat("Frequency", &layer.fractalFrequency, 0.01f,
                               10.0f);
            grid.AddInputInt("Octaves", &layer.fractalOctaves, 1, 20);
            grid.AddInputFloat("Gain", &layer.fractalGain, 0.0f, 1.0f);
            grid.AddInputDouble("Weight", &layer.weight, 0.0, 10.0);

            int tempMinHeight = layer.minHeight;
            if (grid.AddInputInt("Min Height", &tempMinHeight, 0, 255)) {
              layer.minHeight =
                  static_cast<unsigned char>(std::clamp(tempMinHeight, 0, 255));
            }

            int tempMaxHeight = layer.maxHeight;
            if (grid.AddInputInt("Max Height", &tempMaxHeight, 0, 255)) {
              layer.maxHeight =
                  static_cast<unsigned char>(std::clamp(tempMaxHeight, 0, 255));
            }

            grid.AddInputDouble("Tan Factor", &layer.tanFactor, 0.0, 100.0);
            grid.AddInputDouble("Width Edge", &layer.widthEdge, 0.0, 100.0);
            grid.AddInputDouble("Height Edge", &layer.heightEdge, 0.0, 100.0);

            if (layerTypeSelection != 0) { // Land or Sea layers
              ImGui::Spacing();
              ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f),
                                 "Altitude Weight Settings");
              grid.AddInputDouble("Weight Start", &layer.altitudeWeightStart,
                                  0.0, 1.0);
              grid.AddInputDouble("Weight End", &layer.altitudeWeightEnd, 0.0,
                                  1.0);
            }
          }

          ImGui::Spacing();
          ImGui::Separator();
          ImGui::Spacing();

          if (ImGui::Button("Randomize Seed", ImVec2(150, 0))) {
            layer.seed = RandNum::getRandom<int>();
          }
          ImGui::SameLine();
          ImGui::Text("Current Seed: %d", layer.seed);

        } else {
          ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f),
                             "No layer selected or layer list is empty");
        }

        ImGui::EndChild();
      }
    } // End of CollapsingHeader
    // In showHeightmapTab(), after the Layer Editor CollapsingHeader
    ImGui::Spacing();

    // Pipeline Editor - called directly, not nested
    ImGui::SeparatorText("Heightmap Processing Pipeline");
    landUI.configurePipelineEditor(cfg);

    ImGui::Spacing();
    ImGui::SeparatorText("Generation Controls");

    ImGui::Checkbox("Randomize seed on each generation", &rerandomiseSeed);
    ImGui::Spacing();

    // Generation buttons based on input mode
    switch (cfg.landInputMode) {
    case Fwg::Terrain::InputMode::HEIGHTMAP: {
      if (UI::Elements::Button("Generate Random Worldmap", false,
                               ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        computationFutureBool = runAsync([&fwg, this]() {
          fwg.genHeight();
          uiUtils->resetTexture();
          updateLayer = true;
          return true;
        });
      }

      ImGui::SameLine();

      if (UI::Elements::Button("Apply Land/Sea Layers", false,
                               ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        computationFutureBool = runAsync([&fwg, this]() {
          fwg.genLand();
          updateLayer = true;
          uiUtils->resetTexture();
          return true;
        });
      }

      if (UI::Elements::ImportantStepButton(
              "Generate Complete Heightmap from new seed", ImVec2(250, 0))) {
        cfg.randomSeed = true;
        cfg.reRandomize();
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genHeight();
          fwg.genLand();
          uiUtils->resetTexture();
          updateLayer = true;
          return true;
        });
      }
      break;
    }

    case Fwg::Terrain::InputMode::HEIGHTSKETCH: {
      if (UI::Elements::Button("Generate from Sketch", false, ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        fwg.genHeightFromInput(cfg, cfg.mapsPath + "/heightSketchInput.png",
                               cfg.landInputMode);
        uiUtils->resetTexture();
      }

      ImGui::SameLine();

      if (UI::Elements::Button("Apply Detail Layers", false, ImVec2(250, 0))) {
        computationFutureBool = runAsync([&fwg, this]() {
          fwg.genLand();
          uiUtils->resetTexture();
          return true;
        });
      }
      break;
    }
    case Fwg::Terrain::InputMode::TOPOGRAPHY: {
      // TODO Not supported yet

      // if (ImGui::Button("Generate land shape from topography")) {
      // if (rerandomiseSeed) {
      //   cfg.randomSeed = true;
      //   cfg.reRandomize();
      // }
      //   fwg.genHeightFromInput(cfg, cfg.mapsPath + "/topographyInput.png",
      //                          cfg.landInputMode);
      // }
      break;
    }
    case Fwg::Terrain::InputMode::LANDFORM: {
      auto classificationGuard =
          UI::PrerequisiteChecker::requireSilent({UI::Prerequisite{
              "Classification", "Complete landform classification",
              [&]() { return amountClassificationsNeeded <= 0 && !analyze; }}});

      if (classificationGuard.ready() &&
          UI::Elements::ImportantStepButton("Generate from Landform Input",
                                            ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          if (fwg.genHeightFromInput(cfg, cfg.mapsPath + "/classifiedLandInput.png",
                                     cfg.landInputMode)) {
            fwg.genLand();
          }
          updateLayer = true;
          uiUtils->resetTexture();
          return true;
        });
      }
      break;
    }

    case Fwg::Terrain::InputMode::LANDMASK: {
      if (UI::Elements::ImportantStepButton("Generate from Landmask",
                                            ImVec2(250, 0))) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genHeightFromInput(cfg, cfg.mapsPath + "/landmaskInput.png",
                                 cfg.landInputMode);
          fwg.genLand();
          updateLayer = true;
          uiUtils->resetTexture();
          return true;
        });
      }
      break;
    }

    default:
      break;
    }

    // Drag & drop handler
    if (triggeredDrag) {
      triggeredDrag = false;
      cfg.allowHeightmapModification = false;
      fwg.loadHeight(cfg, IO::Reader::readHeightmapImage(draggedFile, cfg));
      uiUtils->resetTexture();
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showNormalMapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Normalmap")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(
          0, Fwg::Gfx::displaySobelMap(fwg.terrainData.sobelData));
      uiUtils->updateImage(
          1, Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap));
    }
    uiUtils->showHelpTextBox("Normalmap");

    ImGui::SeparatorText("Normal Map Configuration");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);
      grid.AddInputDouble("Sobel Factor", &cfg.sobelFactor, 0.01, 10.0);
    }

    ImGui::Spacing();
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData)});

    if (guard.ready()) {
      if (UI::Elements::ImportantStepButton("Generate Normalmap",
                                            ImVec2(200, 0))) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genSobelMap(cfg);
          uiUtils->resetTexture(0);
          return true;
        });
      }
    }

    if (triggeredDrag) {
      triggeredDrag = false;
      auto heightMap = IO::Reader::readHeightmapImage(draggedFile, cfg);
      fwg.loadHeight(cfg, heightMap);
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genSobelMap(cfg);
        uiUtils->resetTexture();
        return true;
      });
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showClimateOverview(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginMainTabItem("Climate Generation")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->setForceUpdate();
      uiUtils->resetTexture();
    }
    uiUtils->showHelpTextBox("Climate");

    ImGui::SeparatorText("Base Climate Parameters");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      if (grid.AddInputDouble("Base Temperature", &cfg.baseTemperature, -100.0,
                              100.0) |
          grid.AddInputDouble("Base Humidity", &cfg.baseHumidity, 0.0, 100.0) |
          grid.AddInputDouble("Fantasy Frequency", &cfg.fantasyClimateFrequency,
                              0.0, 10.0)) {
        redoHumidity = true;
      }

      // Fantasy climate checkbox on new row
      grid.NextRow();
      ImGui::Checkbox("Fantasy Climate", &cfg.fantasyClimate);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        redoHumidity = true;
      }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Latitude & River Settings");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      if (grid.AddInputDouble("Latitude High", &cfg.latHigh, -90.0, 90.0) |
          grid.AddInputDouble("Latitude Low", &cfg.latLow, -90.0, 90.0) |
          grid.AddInputDouble("River Amount", &cfg.riverFactor, 0.0, 10.0) |
          grid.AddInputDouble("River Humidity", &cfg.riverHumidityFactor, 0.0,
                              10.0) |
          grid.AddInputDouble("River Range", &cfg.riverEffectRangeFactor, 0.0,
                              10.0)) {
        redoHumidity = true;
      }
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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genTemperatures(cfg);
          fwg.genHumidity(cfg);
          fwg.genRivers(cfg);
          fwg.genClimate(cfg);
          fwg.genWorldMap(cfg);
          uiUtils->setForceUpdate();
          uiUtils->resetTexture();
          return true;
        });
      }
    }

    if (UI::Elements::BeginSubTabBar("Climate Generation", 0.0f)) {
      showTemperatureMap(cfg, fwg);
      showHumidityTab(cfg, fwg);
      showRiverTab(cfg, fwg);
      showClimateTab(cfg, fwg);
      showTreeTab(cfg, fwg);
      UI::Elements::EndSubTabBar();
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showClimateInputTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {

  if (UI::Elements::BeginMainTabItem("Climate Input")) {
    static bool analyze = false;
    static int amountClassificationsNeeded = 0;
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0, climateUI.climateInputMap);
      uiUtils->updateImage(1, Fwg::Gfx::Image());
    }
    ImGui::SeparatorText("This step is OPTIONAL! You can also generate a "
                         "random climate in the next tab");
    uiUtils->showHelpTextBox("Climate Input");
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData)});

    if (guard.ready()) {
      if (triggeredDrag) {
        triggeredDrag = false;
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          // don't immediately generate from the input, instead allow to
          // manually classify all present colours
          climateUI.climateInputMap =
              Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
          // create a map from secondary colours to primary colours
          Fwg::Utils::ColourTMap<Fwg::Climate::ClimateClassDefinition>
              secondaryToPrimary;
          for (auto &type : fwg.climateData.climateClassDefinitions) {
            for (auto &secondary : type.secondaryColours) {
              secondaryToPrimary.setValue(secondary, type);
            }
          }

          // preprocess input to convert to primary colours where possible
          for (auto &col : climateUI.climateInputMap.imageData) {
            if (secondaryToPrimary.contains(col)) {
              col = secondaryToPrimary[col].primaryColour;
            }
          }

          if (climateUI.climateInputMap.size() !=
              fwg.terrainData.detailedHeightMap.size()) {
            Utils::Logging::logLine(
                "Climate input map size does not match height "
                "map size. Please ensure that the input map is "
                "the same size as the height map");
            climateUI.climateInputMap.clear();
          } else {
            analyze = true;
          }
          uiUtils->resetTexture();
          return true;
        });
      }
      if (climateUI.climateInputMap.initialised()) {
        if (climateUI.complexTerrainMapping(cfg, fwg, analyze,
                                            amountClassificationsNeeded)) {
          uiUtils->resetTexture();
        }
      }
    }

    // Classification guard
    auto classificationGuard =
        UI::PrerequisiteChecker::requireSilent({UI::Prerequisite{
            "Classification", "Complete climate classification",
            [&]() { return amountClassificationsNeeded <= 0 && !analyze; }}});

    if (classificationGuard.ready() && climateUI.climateInputMap.size() &&
        ImGui::Button("Generate from labeled climate")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        cfg.complexClimateInput = true;
        Fwg::Gfx::Png::save(climateUI.climateInputMap,
                            cfg.mapsPath + "/classifiedClimateInput.png");
        fwg.loadClimate(cfg, climateUI.climateInputMap);
        uiUtils->resetTexture();
        return true;
      });
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showTemperatureMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Temperature")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(
          0, Fwg::Gfx::Climate::displayTemperature(fwg.climateData));
      uiUtils->updateImage(1, Fwg::Gfx::Image());
    }
    uiUtils->showHelpTextBox("Temperature");

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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genTemperatures(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        fwg.loadTemperatures(cfg, draggedFile, applyAltitudeEffect);
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showHumidityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Humidity")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0,
                           Fwg::Gfx::Climate::displayHumidity(fwg.climateData));
      uiUtils->updateImage(1, Fwg::Gfx::Image());
    }
    uiUtils->showHelpTextBox("Humidity");

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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genHumidity(cfg);
          uiUtils->resetTexture(0);
          return true;
        });
      }

      if (triggeredDrag) {
        fwg.loadHumidity(cfg,
                         Fwg::IO::Reader::readGenericImage(draggedFile, cfg),
                         applyElevationEffect);
        redoHumidity = false;
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showRiverTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Rivers")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0, Gfx::riverMap(fwg.terrainData.detailedHeightMap,
                                            fwg.climateData.rivers));
      uiUtils->updateImage(1, Fwg::Gfx::Image());
    }
    uiUtils->showHelpTextBox("Rivers");

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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genRivers(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        fwg.loadRivers(cfg,
                       Fwg::IO::Reader::readGenericImage(draggedFile, cfg));
        uiUtils->resetTexture();
        triggeredDrag = false;
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
int FwgUI::showClimateTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Climate")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(
          0, Fwg::Gfx::Climate::displayClimate(fwg.climateData, false));
      uiUtils->updateImage(1, fwg.worldMap);
    }

    uiUtils->showHelpTextBox("Climate Gen");
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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          if (redoHumidity) {
            fwg.genTemperatures(cfg);
            fwg.genHumidity(cfg);
            redoHumidity = false;
          }
          fwg.genClimate(cfg);
          uiUtils->resetTexture();
          return true;
        });
      } else if (cfg.fantasyClimate &&
                 ImGui::Button("Generate completely random fantasy climate")) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genTemperatures(cfg);
          fwg.genHumidity(cfg);
          redoHumidity = false;
          fwg.genClimate(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        if (climateUI.climateInputMap.initialised()) {
          computationFutureBool = runAsync([&fwg, &cfg, this]() {
            fwg.loadClimate(cfg, climateUI.climateInputMap);
            fwg.genWorldMap(cfg);
            uiUtils->resetTexture();
            return true;
          });
        } else {
          int amountClassificationsNeeded;
          auto climateInput =
              Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
          // load a valid map if no classificationsNeeded
          if (climateUI.analyzeClimateMap(cfg, fwg, climateInput,
                                          amountClassificationsNeeded)) {
            fwg.loadClimate(cfg, climateInput);
            uiUtils->resetTexture();
          } else {
            Utils::Logging::logLine(
                "You are trying to load a climate input that has "
                "incompatible "
                "colours. If you want to use a complex climate map as input, "
                "please use the Climate Input tab label the climate zones. "
                "The "
                "resulting map will be used as "
                "climate input here automatically.");
          }
        }
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showTreeTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Forests")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(
          0, Fwg::Gfx::Climate::displayClimate(fwg.climateData, true));
      uiUtils->updateImage(
          1, Fwg::Gfx::Climate::displayTreeDensity(fwg.climateData));
    }
    uiUtils->showHelpTextBox("Forests");

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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genForests(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        fwg.loadForests(cfg, draggedFile);
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showWastelandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Wasteland")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0, fwg.worldMap);
      uiUtils->updateImage(1, fwg.worldMap);
    }
    uiUtils->showHelpTextBox("Wasteland");
    ImGui::SeparatorText("Generate wasteland map or drop it in");
    if (ImGui::Button("Generate Wasteland Map")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        // fwg.genWasteland(cfg);
        uiUtils->resetTexture();
        return true;
      });
    }

    if (triggeredDrag) {
      // fwg.loadWasteland(cfg, draggedFile);
      triggeredDrag = false;
      uiUtils->resetTexture();
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showAreasTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {

  if (UI::Elements::BeginMainTabItem("Areas")) {
    if (uiUtils->tabSwitchEvent()) {
      // force update so sub-selected tabs get updated
      uiUtils->setForceUpdate();
      uiUtils->resetTexture();
    }
    {
      auto guard = UI::PrerequisiteChecker::require(
          {UI::PrerequisiteChecker::climate(fwg.climateData),
           UI::PrerequisiteChecker::landforms(fwg.terrainData)});

      if (guard.ready() && UI::Elements::AutomationStepButton(
                               "Generate all areas automatically")) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          modifiedAreas = true;
          fwg.genHabitability(cfg);
          fwg.genSuperSegments(cfg);
          fwg.genSegments(cfg);
          fwg.genProvinces();
          fwg.genContinents(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }
    }
    areaInputSelector(cfg);

    if (UI::Elements::BeginSubTabBar("Area Tabs", 0.0f)) {
      showDensityTab(cfg, fwg);
      showSuperSegmentTab(cfg, fwg);
      showSegmentTab(cfg, fwg);
      showProvincesTab(cfg, fwg);
      showContinentTab(cfg, fwg);
      UI::Elements::EndSubTabBar();
    }

    ImGui::EndTabItem();
  }
  return 0;
}
int FwgUI::showDensityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Density")) {
    if (uiUtils->tabSwitchEvent()) {
      // pre-create density map, if not existing yet, so users see the default
      // map and can then decide to overwrite (or change parameters)
      if (!fwg.climateData.habitabilities.size() &&
          fwg.climateData.climateChances.size() &&
          fwg.terrainData.landFormIds.size()) {
        fwg.genHabitability(cfg);
      }
      uiUtils->updateImage(
          0, Gfx::displayHabitability(fwg.climateData.habitabilities));
      uiUtils->updateImage(1, fwg.worldMap);
    }

    uiUtils->showHelpTextBox("Density");

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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genHabitability(cfg);
          uiUtils->resetTexture(0);
          return true;
        });
      }

      if (triggeredDrag) {
        fwg.loadHabitability(
            cfg, Fwg::IO::Reader::readGenericImage(draggedFile, cfg));
        uiUtils->resetTexture(0);
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
void FwgUI::showSuperSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("SuperSegments")) {
    if (uiUtils->tabSwitchEvent()) {
      if (fwg.worldMap.size()) {
        uiUtils->updateImage(0, Fwg::Gfx::Segments::displaySuperSegments(
                                    fwg.areaData.superSegments));
        uiUtils->updateImage(1, fwg.errorMap);
      }
    }
    uiUtils->showHelpTextBox("SuperSegments");

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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genSuperSegments(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          triggeredDrag = false;
          auto evaluationAreas =
              Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                  fwg.terrainData.landMask);
          if (cfg.areaInputMode == Fwg::Areas::AreaInputType::SOLID) {
            fwg.loadSuperSegments(cfg,
                                  Fwg::IO::Reader::readGenericImageWithBorders(
                                      draggedFile, cfg, evaluationAreas));
          } else {
            auto image = Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
            Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                image, evaluationAreas);
            Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
            fwg.loadSuperSegments(cfg, image);
          }
          uiUtils->resetTexture();
          return true;
        });
      }
    }

    ImGui::EndTabItem();
  }
}
void FwgUI::showSegmentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  static auto lastEvent = std::chrono::high_resolution_clock::now();

  if (UI::Elements::BeginSubTabItem("Segments")) {
    // check if 50ms have passed since last event
    auto now = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEvent);
    if (uiUtils->tabSwitchEvent() || duration.count() > 50) {
      lastEvent = now;
      uiUtils->updateImage(0, fwg.segmentMap);
      uiUtils->updateImage(1, fwg.errorMap);
    }
    uiUtils->showHelpTextBox("Segments");

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
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          modifiedAreas = true;
          fwg.genSegments(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          triggeredDrag = false;
          const auto evaluationAreas =
              Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                  fwg.terrainData.landMask);
          if (cfg.areaInputMode == Fwg::Areas::AreaInputType::SOLID) {
            fwg.loadSegments(cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                                      draggedFile, cfg, evaluationAreas));
          } else {
            auto image = Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
            Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                image, evaluationAreas);
            Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
            fwg.loadSegments(cfg, image);
          }
          fwg.segmentMap =
              Fwg::Gfx::Segments::displaySegments(fwg.areaData.segments);
          uiUtils->resetTexture();
          modifiedAreas = true;
          return true;
        });
      }
    }

    ImGui::EndTabItem();
  }
}
int FwgUI::showProvincesTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  static auto lastEvent = std::chrono::high_resolution_clock::now();

  if (UI::Elements::BeginSubTabItem("Provinces")) {
    // check if 50ms have passed since last event
    auto now = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastEvent);
    if (uiUtils->tabSwitchEvent() || duration.count() > 50) {
      lastEvent = now;
      uiUtils->updateImage(0, fwg.provinceMap);
      uiUtils->updateImage(1, fwg.segmentMap);
    }
    uiUtils->showHelpTextBox("Provinces");

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
        modifiedAreas = true;
        cfg.calcAreaParameters();
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          if (!fwg.genProvinces()) {
            return false;
          }
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          modifiedAreas = true;
          triggeredDrag = false;
          auto evaluationAreas =
              Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                  fwg.terrainData.landMask);
          if (cfg.areaInputMode == Fwg::Areas::AreaInputType::SOLID) {
            fwg.loadProvinces(cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                                       draggedFile, cfg, evaluationAreas));
          } else {
            auto image = Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
            Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                image, evaluationAreas);
            Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
            fwg.loadProvinces(cfg, image);
          }
          uiUtils->resetTexture();
          return true;
        });
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
int FwgUI::showRegionTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Regions")) {
    uiUtils->tabSwitchEvent();
    uiUtils->showHelpTextBox("Regions");

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::provinceMap(fwg.provinceMap)});

    if (guard.ready()) {
      ImGui::SeparatorText("Generate a region map");

      if (ImGui::Button("Generate Region Map from Segments and Provinces")) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genRegions(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }
      ImGui::Text("The map has %i regions",
                  static_cast<int>(fwg.areaData.regions.size()));

      if (triggeredDrag) {
        if (fwg.provinceMap.initialised()) {
          try {
            // fwg.loadRegions(cfg, draggedFile);
            Utils::Logging::logLine(
                "If you want to load regions, do it in the segments tab.");
          } catch (std::exception &e) {
            Utils::Logging::logLine(
                "Couldn't load regions, fix input or try again");
            fwg.regionMap = Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
          }
        }
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showContinentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Continents")) {
    if (uiUtils->tabSwitchEvent() && fwg.areaData.provinces.size() &&
        fwg.areaData.regions.size()) {
      uiUtils->updateImage(0,
                           Fwg::Gfx::simpleContinents(fwg.areaData.continents,
                                                      fwg.areaData.seaBodies));
      uiUtils->updateImage(
          1, Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap));
    }
    uiUtils->showHelpTextBox("Continents");

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
          (fwg.areaData.continents.empty() && !computationRunning)) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          modifiedAreas = true;
          fwg.genContinents(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      if (triggeredDrag) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          modifiedAreas = true;
          auto evaluationAreas =
              Fwg::UI::Utils::Masks::getLandmaskEvaluationAreas(
                  fwg.terrainData.landMask);
          fwg.loadContinents(cfg, Fwg::IO::Reader::readGenericImageWithBorders(
                                      draggedFile, cfg, evaluationAreas));
          triggeredDrag = false;
          uiUtils->resetTexture();
          return true;
        });
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}
// Helper functions

bool FwgUI::CreateDeviceGL(const char *title, int width, int height) {
  if (!glfwInit()) {
    return false;
  }

  // Configure GLFW for modern OpenGL
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWmonitor *primary = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(primary);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
  // Create a windowed fullscreen (borderless) window
  window = glfwCreateWindow(mode->width, mode->height, title, nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return false;
  }

  glfwMaximizeWindow(window);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable vsync

  // Load GL through GLAD
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    return false;

  return true;
}

void FwgUI::CleanupDeviceGL() {
  if (window) {
    glfwDestroyWindow(window);
    window = nullptr;
  }

  glfwTerminate();
}

} // namespace Fwg