#include "UI/FwgUI.h"

namespace Fwg {
// Data
int g_ResizeWidth = 0;
int g_ResizeHeight = 0;
int FwgUI::seed = 0;

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
  initAllowedInput(cfg, fwg.climateData, fwg.terrainData.landformDefinitions);
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

static bool analyze = false;
static int amountClassificationsNeeded = 0;

int FwgUI::showLandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Land Input")) {
    if (uiUtils->tabSwitchEvent(true)) {
      uiUtils->updateImage(0, Fwg::Gfx::landFormMap(fwg.terrainData));
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

  if (UI::Elements::BeginSubTabItem("Heightmap")) {
    if (uiUtils->tabSwitchEvent()) {
      if (fwg.terrainData.detailedHeightMap.size()) {
        auto heightmap =
            Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap);
        uiUtils->updateImage(0, heightmap);
        uiUtils->updateImage(1, heightmap);
        // wrap around because we want to show two images
        if (updateLayer) {
          if (selectedLayer < fwg.terrainData.layerData.size() &&
              fwg.terrainData.layerData[selectedLayer].size() &&
              fwg.terrainData.detailedHeightMap.size()) {
            uiUtils->updateImage(
                1, Fwg::Gfx::Image(cfg.width, cfg.height, 24,
                                   fwg.terrainData.layerData[selectedLayer]));
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

    static bool layeredit = false;
    uiUtils->showHelpTextBox("Heightmap");
    ImGui::PushItemWidth(100.0f);
    ImGui::SeparatorText(
        "Generate a simple overview of land area from heightmap "
        "or drop it in");

    // selection of heightmap preset
    static int activeConfigIndex = -1;
    ImGui::Text("Heightmap configs");

    for (int i = 0; i < heightmapConfigFiles.size(); ++i) {
      bool isActive = (i == activeConfigIndex);

      if (ImGui::Selectable(heightmapConfigFiles[i].c_str(), isActive)) {
        activeConfigIndex = i;
        cfg.readHeightmapConfig(heightmapConfigFiles[i]);
      }

      if (isActive)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::Text("Sealevel: %d", cfg.seaLevel);

    if (cfg.landInputMode == Fwg::Terrain::InputMode::HEIGHTMAP) {
      ImGui::SliderFloat("<--Target Land Percentage", &cfg.landPercentage, 0.00,
                         1.0);
      ImGui::InputInt("<--Height Adjustments", &cfg.heightAdjustments);
      ImGui::InputFloat("<--Landlayer coastal distance factor",
                        &cfg.layerApplicationFactor, 0.1f, 0.1f);
    } else {
      ImGui::Text("Landpercentage: %f", cfg.landPercentage);
    }

    ImGui::InputDouble("<--Maximum Lake Size Factor", &cfg.lakeMaxShare, 0.01f);
    if (ImGui::InputInt("<--Maximum Land height", &cfg.maxLandHeight)) {
      cfg.maxLandHeight = std::clamp(cfg.maxLandHeight, cfg.seaLevel + 1, 255);
    }
    ImGui::PopItemWidth();
    landUI.configureLandElevationFactors(cfg, fwg);
    ImGui::PushItemWidth(100.0f);
    ImGui::SameLine();
    if (ImGui::SliderFloat("<--Heightmap Frequency",
                           &cfg.heightmapFrequencyModifier, 0.1f, 10.0f,
                           "ratio = %.1f")) {
    }
    ImGui::SliderFloat("<--Width edge Factor", &cfg.heightmapWidthEdgeModifier,
                       0.0f, 10.0f, "ratio = %.1f");
    ImGui::SliderFloat("<--Height edge Factor",
                       &cfg.heightmapHeightEdgeModifier, 0.0f, 10.0f,
                       "ratio = %.1f");
    ImGui::Checkbox("<--Layer edit", &layeredit);
    if (layeredit) {
      ImGui::SeparatorText(
          "Edit the settings of the selected layer. On the left is the "
          "finished "
          "heightmap, on the right is the currently selected layer");
      ImGui::SliderInt("<--Layers used for generation", &cfg.layerAmount, 1,
                       cfg.maxLayerAmount);
      {
        ImGui::BeginChild("LayerSelection",
                          ImVec2(ImGui::GetContentRegionAvail().x * 0.2f,
                                 ImGui::GetContentRegionAvail().y * 0.8f),
                          false);
        // TODO: ugly
        std::vector<std::string> stringRepr;
        stringRepr.resize(cfg.layerAmount);
        std::vector<const char *> cStrRepr;
        cStrRepr.resize(cfg.layerAmount);
        for (int i = 0; i < cfg.layerAmount; i++) {
          stringRepr[i] = std::to_string(i);
          cStrRepr[i] = stringRepr[i].c_str();
        }
        ImGui::Text("Select Layer");
        if (ImGui::ListBox("", &selectedLayer, cStrRepr.data(), cStrRepr.size(),
                           12)) {
          updateLayer = true;
          uiUtils->resetTexture(1);
        }
        ImGui::EndChild();
      }
      ImGui::SameLine();
      {
        ImGui::BeginChild("LayerEdit",
                          ImVec2(ImGui::GetContentRegionAvail().x * 1.0f,
                                 ImGui::GetContentRegionAvail().y * 0.8f),
                          false);
        ImGui::PushItemWidth(100.0f);
        bool landLayer = cfg.landLayer[selectedLayer];
        ImGui::Checkbox("<--LandLayer", &landLayer);
        cfg.landLayer[selectedLayer] = landLayer;
        bool seaLayer = cfg.seaLayer[selectedLayer];
        ImGui::SameLine();
        ImGui::Checkbox("<--SeaLayer", &seaLayer);
        cfg.seaLayer[selectedLayer] = seaLayer;

        static const char *NoiseTypeNames[] = {"OpenSimplex2", "OpenSimplex2S",
                                               "Cellular",     "Perlin",
                                               "ValueCubic",   "Value"};

        static const char *FractalTypeNames[] = {"None",
                                                 "FBm",
                                                 "Ridged",
                                                 "PingPong",
                                                 "DomainWarpProgressive",
                                                 "DomainWarpIndependent"};

        int &noiseType = cfg.noiseType[selectedLayer];
        int &fractalType = cfg.fractalType[selectedLayer];

        // Clamp indices for safety
        noiseType =
            std::clamp(noiseType, 0, (int)(IM_ARRAYSIZE(NoiseTypeNames)) - 1);
        fractalType = std::clamp(fractalType, 0,
                                 (int)(IM_ARRAYSIZE(FractalTypeNames)) - 1);

        // Show enum as nice dropdowns
        ImGui::Combo("Noise Type", &noiseType, NoiseTypeNames,
                     IM_ARRAYSIZE(NoiseTypeNames));
        ImGui::Combo("Fractal Type", &fractalType, FractalTypeNames,
                     IM_ARRAYSIZE(FractalTypeNames));

        ImGui::InputFloat("<--FractalFrequency",
                          &cfg.fractalFrequency[selectedLayer], 0.1);
        ImGui::InputInt("<--fractalOctaves", &cfg.fractalOctaves[selectedLayer],
                        1);
        ImGui::InputFloat("<--fractalGain", &cfg.fractalGain[selectedLayer],
                          0.05);
        ImGui::InputDouble("<--Weight", &cfg.weight[selectedLayer], 0.05);
        int tempInt = std::get<0>(cfg.heightRange[selectedLayer]);
        ImGui::InputInt("<--minHeight", &tempInt, 5);
        std::get<0>(cfg.heightRange[selectedLayer]) =
            static_cast<unsigned char>(tempInt);
        int tempInt2 = std::get<1>(cfg.heightRange[selectedLayer]);
        ImGui::InputInt("<--maxHeight", &tempInt2, 5);
        std::get<1>(cfg.heightRange[selectedLayer]) =
            static_cast<unsigned char>(tempInt2);
        ImGui::InputDouble("<--Tanfactor", &cfg.tanFactor[selectedLayer], 0.01);
        ImGui::InputDouble("<--widthEdge", &cfg.widthEdge[selectedLayer], 1);
        ImGui::InputDouble("<--heightEdge", &cfg.heightEdge[selectedLayer], 1);
        ImGui::InputDouble("<--Edge limit factor", &cfg.edgeLimitFactor, 0.1);
        ImGui::PopItemWidth();
        ImGui::EndChild();
      }
    }
    ImGui::SeparatorText("Generate using buttons or drop in file");
    ImGui::Checkbox("Change seed on every generation click", &rerandomiseSeed);
    std::string buttonText = "";

    switch (cfg.landInputMode) {
    case Fwg::Terrain::InputMode::HEIGHTMAP: {
      if (ImGui::Button("Generate random worldmap")) {
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
        updateLayer = true;
      }
      if (ImGui::Button("Generate detailed heightmap from heightmap shape")) {
        computationFutureBool = runAsync([&fwg, this]() {
          fwg.genLand();
          updateLayer = true;
          uiUtils->resetTexture();
          return true;
        });
      }
      break;
    }
    case Fwg::Terrain::InputMode::HEIGHTSKETCH: {
      if (ImGui::Button("Generate land shape from sketch heightmap")) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        fwg.genHeightFromInput(cfg, cfg.mapsPath + "/heightSketchInput.png",
                               cfg.landInputMode);
        uiUtils->resetTexture();
      }
      if (ImGui::Button("Generate detailed heightmap from heightmap shape")) {
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
          ImGui::Button("Generate detailed heightmap from landform input")) {
        if (rerandomiseSeed) {
          cfg.randomSeed = true;
          cfg.reRandomize();
        }
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          if (fwg.genHeightFromInput(cfg, cfg.mapsPath + "/landformInput.png",
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
      if (ImGui::Button("Generate detailed heightmap from landmask input")) {
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

    ImGui::PopItemWidth();
    ImGui::PushItemWidth(300.0f);
    if (cfg.landInputMode == Fwg::Terrain::InputMode::HEIGHTMAP &&
        ImGui::Button("Generate complete random heightmap from new seed")) {
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

    ImGui::PopItemWidth();
    // drag event
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

void FwgUI::clearColours(Fwg::Gfx::Image &image) {
  static int severity = 0;
  // first count every colour in a colourTMap
  Utils::ColourTMap<std::vector<int>> colourCounter;
  for (int pix = 0; pix < image.size(); pix++) {
    const auto &col = image[pix];
    if (!colourCounter.contains(col)) {
      colourCounter.setValue(col, {});
      colourCounter[col].push_back(pix);
    } else {
      colourCounter[col].push_back(pix);
    }
  }
  std::vector<Fwg::Gfx::Colour> colorsSortedByDistance;

  for (auto &elem : colourCounter.getMap()) {
    auto &col = elem.first;
    if (elem.second.size() <
        image.size() / std::clamp((100 - severity), 1, 100)) {
      for (auto &elem2 : colourCounter.getMap()) {
        if (elem2.second.size() &&
            col.distance(elem2.first) < (10 + severity)) {
          elem.second.insert(elem.second.end(), elem2.second.begin(),
                             elem2.second.end());

          for (auto pix : elem2.second) {
            image.setColourAtIndex(pix, col);
          }
          elem2.second.clear();
        }
      }
    }
  }
  severity++;
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
    ImGui::SeparatorText("Generate a NormalMap.");

    ImGui::PushItemWidth(200.0f);
    ImGui::InputDouble("Sobelfactor", &cfg.sobelFactor, 0.05, 0.5);
    // limit sobel to more than 0.00
    cfg.sobelFactor = std::max<double>(cfg.sobelFactor, 0.01);
    ImGui::PopItemWidth();

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData)});

    if (guard.ready() && ImGui::Button("Generate Normalmap")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genSobelMap(cfg);
        uiUtils->resetTexture(0);
        return true;
      });
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

    ImGui::PushItemWidth(200.0f);
    // evaluate multiple inputs at once so short-circuit evaluation doesn't
    // trigger flickering
    if (longCircuitLogicalOr(
            ImGui::InputDouble("<--Base temperature", &cfg.baseTemperature,
                               0.1),
            ImGui::InputDouble("<--Base humidity", &cfg.baseHumidity, 0.1),
            ImGui::InputDouble("<--Fantasy climate frequency modifier",
                               &cfg.fantasyClimateFrequency, 0.1),
            ImGui::Checkbox("<--Fantasy climate", &cfg.fantasyClimate))

    ) {
      redoHumidity = true;
    }
    if (longCircuitLogicalOr(
            ImGui::InputDouble("<--Latitude high", &cfg.latHigh, 0.1),
            ImGui::InputDouble("<--Latitude low", &cfg.latLow, 0.1),
            ImGui::InputDouble("<--River amount multiplier", &cfg.riverFactor,
                               0.1),
            ImGui::InputDouble("<--River humidity multiplier",
                               &cfg.riverHumidityFactor, 0.1),
            ImGui::InputDouble("<--River effect range multiplier",
                               &cfg.riverEffectRangeFactor, 0.1))) {
      redoHumidity = true;
    }
    ImGui::PopItemWidth();

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

    ImGui::SeparatorText("Generate temperature map or drop it in. You can also "
                         "draw in this map");
    static bool applyAltitudeEffect = false;
    ImGui::Checkbox("<--Apply elevation effect when loading temperature",
                    &applyAltitudeEffect);

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData)});

    if (guard.ready()) {
      if (ImGui::Button("Generate Temperature Map")) {
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
    ImGui::SeparatorText(
        "Generate humidity map or drop it in. You can also draw in this map");
    static bool applyEleveationEffect = false;
    ImGui::Checkbox("<--Apply elevation effect when loading humidity",
                    &applyEleveationEffect);

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData)});

    if (guard.ready()) {
      if (ImGui::Button("Generate Humidity Map")) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genHumidity(cfg);
          uiUtils->resetTexture(0);
          return true;
        });
      }

      if (triggeredDrag) {
        fwg.loadHumidity(cfg,
                         Fwg::IO::Reader::readGenericImage(draggedFile, cfg),
                         applyEleveationEffect);
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
    ImGui::InputDouble("River amount multiplier", &cfg.riverFactor);

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData),
         UI::PrerequisiteChecker::humidity(fwg.climateData)});

    if (guard.ready()) {
      if (ImGui::Button("Generate River Map")) {
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
    ImGui::Value("Amount of rivers: ", (int)fwg.climateData.rivers.size());

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

    ImGui::PushItemWidth(200.0f);
    ImGui::InputDouble("borealDensity", &cfg.borealDensity, 0.1);
    ImGui::InputDouble("temperateNeedleDensity", &cfg.temperateNeedleDensity,
                       0.1);
    ImGui::InputDouble("temperateMixedDensity", &cfg.temperateMixedDensity,
                       0.1);
    ImGui::InputDouble("sparseDensity", &cfg.sparseDensity, 0.1);
    ImGui::InputDouble("tropicalDryDensity", &cfg.tropicalDryDensity, 0.1);
    ImGui::InputDouble("tropicalMoistDensity", &cfg.tropicalMoistDensity, 0.1);
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData),
         UI::PrerequisiteChecker::humidity(fwg.climateData),
         UI::PrerequisiteChecker::temperature(fwg.climateData)});
    if (guard.ready()) {
      if (ImGui::Button("Generate Treemap")) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genForests(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }

      ImGui::PopItemWidth();

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

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData)});

    if (guard.ready()) {
      if (ImGui::Button(
              "Generate province and state density from climate data")) {
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

    ImGui::PushItemWidth(300.0f);
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData)});
    if (guard.ready()) {
      if (ImGui::Button("Generate supersegment template images to draw in.")) {
        Fwg::Gfx::Land::displaySimpleLandType(fwg.terrainData, fwg.areaData,
                                              fwg.worldMap, true, false, false);
      }
      if (ImGui::Button(
              "Generate SuperSegments from landbodies and ocean/water "
              "segments")) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genSuperSegments(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }
      ImGui::PopItemWidth();

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

    ImGui::PushItemWidth(300.0f);
    // To change the segmentCostInfluence value
    ImGui::InputDouble("Segment Cost Influence", &cfg.segmentCostInfluence,
                       0.01, 0.1);
    ImGui::InputDouble("Segment Distance Influence",
                       &cfg.segmentDistanceInfluence, 0.01, 0.1);
    if (ImGui::InputInt("targetLandRegionAmount",
                        &cfg.targetLandRegionAmount)) {
      cfg.autoLandRegionParams = true;
      cfg.calcAreaParameters();
    }
    if (ImGui::InputInt("targetSeaRegionAmount", &cfg.targetSeaRegionAmount)) {
      cfg.autoSeaRegionParams = true;
      cfg.calcAreaParameters();
    }
    ImGui::PopItemWidth();

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::superSegments(fwg.areaData)});

    if (guard.ready()) {
      ImGui::Text("The map has %i land segments",
                  static_cast<int>(fwg.areaData.landSegments));
      ImGui::Text("The map has %i sea segments",
                  static_cast<int>(fwg.areaData.seaSegments));
      ImGui::Text("The map has %i lake segments",
                  static_cast<int>(fwg.areaData.lakeSegments));

      if (ImGui::Button("Generate segment template images to draw in.")) {
        Fwg::Gfx::Land::displaySimpleLandType(fwg.terrainData, fwg.areaData,
                                              fwg.worldMap, false, true, false);
      }

      if (ImGui::Button("Generate Segments")) {

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

            // detect all areas, give them unique colours
            Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                image, evaluationAreas);
            Fwg::Gfx::Png::save(image, cfg.mapsPath + "test1.png");
            // now that we have modified the input image with colours filling
            // the areas between borders, we can remove the borders
            Fwg::Gfx::Filter::fillBlackPixelsByArea(image, evaluationAreas);
            Fwg::Gfx::Png::save(image, cfg.mapsPath + "test2.png");
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

    ImGui::PushItemWidth(200.0f);
    ImGui::SeparatorText("Generate a province map or drop it in");
    ImGui::InputDouble("Landprovincefactor", &cfg.landProvFactor, 0.1);
    ImGui::InputDouble("Seaprovincefactor", &cfg.seaProvFactor, 0.1);
    ImGui::InputDouble("Density Effects", &cfg.provinceDensityEffects, 0.1f);
    ImGui::InputInt("Minimum size of provinces", &cfg.minProvSize);
    // don't allow users to go too low
    cfg.minProvSize = std::max<int>(cfg.minProvSize, 9);
    ImGui::InputInt("Maximum amount of provinces", &cfg.maxProvAmount, 500);
    ImGui::PopItemWidth();
    cfg.landProvFactor = std::clamp(cfg.landProvFactor, 0.0, 10.0);
    cfg.seaProvFactor = std::clamp(cfg.seaProvFactor, 0.0, 10.0);
    cfg.provinceDensityEffects =
        std::clamp(cfg.provinceDensityEffects, 0.0, 1.0);
    ImGui::Text("The map has %i provinces",
                static_cast<int>(fwg.areaData.provinces.size()));
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::superSegments(fwg.areaData),
         UI::PrerequisiteChecker::segments(fwg.areaData)});

    if (guard.ready()) {
      if (ImGui::Button("Generate province template images to draw in.")) {
        Fwg::Gfx::Land::displaySimpleLandType(fwg.terrainData, fwg.areaData,
                                              fwg.worldMap, false, false, true);
      }

      if (ImGui::Button("Generate Provinces Map")) {
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

            // detect all areas, give them unique colours
            Fwg::Gfx::Filter::colouriseAreaBorderInputByBordersOnly(
                image, evaluationAreas);

            // now that we have modified the input image with colours filling
            // the areas between borders, we can remove the borders
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

    ImGui::PushItemWidth(200.0f);
    ImGui::InputInt("Maximum amount of continents", &cfg.maxAmountOfContinents,
                    1);
    ImGui::PopItemWidth();

    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::climate(fwg.climateData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::habitability(fwg.climateData),
         UI::PrerequisiteChecker::superSegments(fwg.areaData),
         UI::PrerequisiteChecker::segments(fwg.areaData),
         UI::PrerequisiteChecker::provinces(fwg.areaData)});

    if (guard.ready()) {
      if (ImGui::Button("Generate Continents from landbodies") ||
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
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    return false;
  }

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