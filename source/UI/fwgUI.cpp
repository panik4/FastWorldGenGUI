#include "UI/FwgUI.h"

namespace Fwg {

FwgUI::FwgUI() : landUI() {}

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

void FwgUI::init(Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  this->uiContext.helpContext.loadHelpTextsFromFile(
      Fwg::Cfg::Values().resourcePath);
  this->uiContext.helpContext.loadHelpImagesFromPath(
      Fwg::Cfg::Values().resourcePath);
  Fwg::UI::Drawing::setClickOffsets(cfg.width, 1);
  log = std::make_shared<std::stringstream>();
  *log << Fwg::Utils::Logging::Logger::logInstance.getFullLog();
  Fwg::Utils::Logging::Logger::logInstance.attachStream(log);
  fwg.configure(cfg);
  heightmapUI.loadHeightmapConfigs();
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
  if (uiContext.asyncContext.computationRunning &&
      uiContext.asyncContext.computationFutureBool.wait_for(
          std::chrono::seconds(0)) == std::future_status::ready) {
    uiContext.asyncContext.computationRunning = false;
    uiContext.imageContext.resetTexture();
  }

  if (uiContext.asyncContext.computationRunning) {
    uiContext.asyncContext.computationStarted = false;
    ImGui::Text("Working, please be patient");
  } else {
    ImGui::Text("Ready!");
  }
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

  Fwg::UI::Utils::setupImGuiContextAndStyle();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  glfwSetWindowUserPointer(window, this);
  glfwSetDropCallback(
      window, [](GLFWwindow *win, int count, const char **paths) {
        auto *fwgui = reinterpret_cast<FwgUI *>(glfwGetWindowUserPointer(win));
        fwgui->uiContext.triggeredDrag = (count > 0);
        fwgui->uiContext.draggedFile =
            (count > 0) ? std::string(paths[count - 1]) : "";
      });
  // glEnable(GL_DEBUG_OUTPUT);
  // glDebugMessageCallback(DebugCallback, nullptr);

  auto &cfg = Fwg::Cfg::Values();
  auto &io = Fwg::UI::Utils::setupImGuiContextAndStyle();
  init(cfg, fwg);

  while (!glfwWindowShouldClose(window)) {
    uiContext.triggeredDrag = false;
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
            if (uiContext.asyncContext.computationRunning) {
              ImGui::BeginDisabled();
            }

            defaultTabs(cfg, fwg);
            // Re-enable inputs if computation is running
            if (uiContext.asyncContext.computationRunning &&
                !uiContext.asyncContext.computationStarted) {
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
  Fwg::UI::Utils::CleanupDeviceGL(window);
  return 0;
}

void FwgUI::initAllowedInput(
    Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
    std::vector<Terrain::LandformDefinition> &landformDefinitions) {
  Fwg::Utils::Logging::logLine("Initialising allowed input");
  auto &climateClassDefinitions = climateData.climateClassDefinitions;
  uiContext.climateUI.allowedClimateInputs.clear();
  for (const auto &climateType : climateClassDefinitions) {
    uiContext.climateUI.allowedClimateInputs.setValue(climateType.primaryColour,
                                            climateType);
  }

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
    ImGui::PushItemWidth(200.0f);
    ImGui::InputInt("Width", &cfg.width);
    ImGui::InputInt("Height", &cfg.height);
    ImGui::PopItemWidth();
    ImGui::EndTabItem();
  }
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
    uiContext.asyncContext.computationFutureBool =
        uiContext.asyncContext.runAsyncInitialDisable([&fwg, &cfg, this]() {
          fwg.generateWorld();
          uiContext.imageContext.resetTexture();
          uiContext.generationContext.modifiedAreas = true;
          return true;
        });
  }
  ImGui::PopItemWidth();
  return true;
}

int FwgUI::showElevationTabs(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {

  if (UI::Elements::BeginMainTabItem("Land Tabs")) {
    uiContext.tabSwitchEvent();
    if (UI::Elements::BeginSubTabBar("Land Tabs", 0.0f)) {
      showLandTab(cfg, fwg);
      heightmapUI.showHeightmapTab(cfg, fwg, uiContext);
      showNormalMapTab(cfg, fwg);
      UI::Elements::EndSubTabBar();
    }
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showLandTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Land Input")) {
    if (uiContext.tabSwitchEvent(true)) {
      uiContext.imageContext.updateImage(0, landUI.landInput);
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
      if (cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM) {
        uiContext.imageContext.updateImage(
            1, Fwg::Gfx::Land::displayLayerWeights(fwg.terrainData));
      }
    }
    uiContext.helpContext.showHelpTextBox("Land");
    //  Selection of land input mode
    ImGui::TextUnformatted("Land Input Mode");
    ImGui::RadioButton("Heightmap. A finished heightmap",
                       cfg.landInputMode == Fwg::Terrain::InputMode::HEIGHTMAP);
    if (ImGui::IsItemClicked()) {
      cfg.landInputMode = Fwg::Terrain::InputMode::HEIGHTMAP;
      cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
                              "default.json");
      cfg.terrainConfig.heightmapPipeline =
          Fwg::Terrain::createDefaultPipeline();
    }
    // ImGui::RadioButton("Heightmap Sketch. A rough outline of a heightmap",
    //                    cfg.landInputMode ==
    //                        Fwg::Terrain::InputMode::HEIGHTSKETCH);
    // if (ImGui::IsItemClicked()) {
    //   cfg.landInputMode = Fwg::Terrain::InputMode::HEIGHTSKETCH;
    //   cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
    //                           "mappedInput.json");
    //   cfg.terrainConfig.heightmapPipeline =
    //       Fwg::Terrain::createDefaultPipeline();
    // }
    ImGui::RadioButton("Land Mask. A simple land/water mask",
                       cfg.landInputMode == Fwg::Terrain::InputMode::LANDMASK);
    if (ImGui::IsItemClicked()) {
      cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
                              "default.json");
      cfg.landInputMode = Fwg::Terrain::InputMode::LANDMASK;
      cfg.terrainConfig.heightmapPipeline =
          Fwg::Terrain::createDefaultPipeline();
    }
    ImGui::RadioButton(
        "Landform. A detailed input map of various colours, that can be mapped "
        "to mountains, lakes, valleys, plains, and more.",
        cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM);
    if (ImGui::IsItemClicked()) {
      cfg.landInputMode = Fwg::Terrain::InputMode::LANDFORM;
      cfg.readHeightmapConfig(cfg.workingDirectory + "configs/heightmap/" +
                              "mappedInput.json");
      cfg.terrainConfig.heightmapPipeline =
          Fwg::Terrain::createLandformPipeline();
    }
    if (cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM &&
        landUI.landInput.size()) {
      landUI.complexLandMapping(
          cfg, fwg, uiContext.generationContext.analyze,
          uiContext.generationContext.amountClassificationsNeeded, uiContext);
    }

    if (uiContext.triggeredDrag) {
      cfg.allowHeightmapModification = true;
      uiContext.asyncContext.computationFutureBool =
          uiContext.asyncContext.runAsync([&fwg, &cfg, this]() {
            landUI.triggeredLandInput(cfg, fwg, uiContext.draggedFile,
                                      cfg.landInputMode);

            // in case of complex input and a drag, we NEED to initially analyze
            if (cfg.landInputMode == Fwg::Terrain::InputMode::LANDFORM) {
              uiContext.generationContext.analyze = true;
            }
            uiContext.triggeredDrag = false;
            uiContext.imageContext.resetTexture();
            return true;
          });
    }
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showNormalMapTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Normalmap")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(
          0, Fwg::Gfx::displaySobelMap(fwg.terrainData.sobelData));
      uiContext.imageContext.updateImage(
          1, Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap));
    }
    uiContext.helpContext.showHelpTextBox("Normalmap");

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
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, this]() {
              fwg.genSobelMap(cfg);
              uiContext.imageContext.resetTexture(0);
              return true;
            });
      }
    }

    if (uiContext.triggeredDrag) {
      uiContext.triggeredDrag = false;
      auto heightMap =
          IO::Reader::readHeightmapImage(uiContext.draggedFile, cfg);
      fwg.loadHeight(cfg, heightMap);
      uiContext.asyncContext.computationFutureBool =
          uiContext.asyncContext.runAsync([&fwg, &cfg, this]() {
            fwg.genSobelMap(cfg);
            uiContext.imageContext.resetTexture();
            return true;
          });
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showClimateInputTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {

  if (UI::Elements::BeginMainTabItem("Climate Input")) {
    static bool analyze = false;
    static int amountClassificationsNeeded = 0;
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.updateImage(0, uiContext.climateUI.climateInputMap);
      uiContext.imageContext.updateImage(1, Fwg::Gfx::Image());
    }
    ImGui::SeparatorText("This step is OPTIONAL! You can also generate a "
                         "random climate in the next tab");
    uiContext.helpContext.showHelpTextBox("Climate Input");
    auto guard = UI::PrerequisiteChecker::require(
        {UI::PrerequisiteChecker::heightmap(fwg.terrainData),
         UI::PrerequisiteChecker::landforms(fwg.terrainData),
         UI::PrerequisiteChecker::landMask(fwg.terrainData)});

    if (guard.ready()) {
      if (uiContext.triggeredDrag) {
        uiContext.triggeredDrag = false;
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, this]() {
              // don't immediately generate from the input, instead allow to
              // manually classify all present colours
              uiContext.climateUI.climateInputMap =
                  Fwg::IO::Reader::readGenericImage(uiContext.draggedFile, cfg);
              // create a map from secondary colours to primary colours
              Fwg::Utils::ColourTMap<Fwg::Climate::ClimateClassDefinition>
                  secondaryToPrimary;
              for (auto &type : fwg.climateData.climateClassDefinitions) {
                for (auto &secondary : type.secondaryColours) {
                  secondaryToPrimary.setValue(secondary, type);
                }
              }

              // preprocess input to convert to primary colours where possible
              for (auto &col : uiContext.climateUI.climateInputMap.imageData) {
                if (secondaryToPrimary.contains(col)) {
                  col = secondaryToPrimary[col].primaryColour;
                }
              }

              if (uiContext.climateUI.climateInputMap.size() !=
                  fwg.terrainData.detailedHeightMap.size()) {
                Utils::Logging::logLine(
                    "Climate input map size does not match height "
                    "map size. Please ensure that the input map is "
                    "the same size as the height map");
                uiContext.climateUI.climateInputMap.clear();
              } else {
                analyze = true;
              }
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
      if (uiContext.climateUI.climateInputMap.initialised()) {
        if (Fwg::UI::Climate::Input::complexTerrainMapping(
                cfg, fwg, uiContext)) {
          uiContext.imageContext.resetTexture();
        }
      }
    }

    // Classification guard
    auto classificationGuard =
        UI::PrerequisiteChecker::requireSilent({UI::Prerequisite{
            "Classification", "Complete climate classification",
            [&]() { return amountClassificationsNeeded <= 0 && !analyze; }}});

    if (classificationGuard.ready() && uiContext.climateUI.climateInputMap.size() &&
        ImGui::Button("Generate from labeled climate")) {
      uiContext.asyncContext.computationFutureBool =
          uiContext.asyncContext.runAsync([&fwg, &cfg, this]() {
            cfg.complexClimateInput = true;
            Fwg::Gfx::Png::save(uiContext.climateUI.climateInputMap,
                                cfg.mapsPath + "/classifiedClimateInput.png");
            fwg.loadClimate(cfg, uiContext.climateUI.climateInputMap);
            uiContext.imageContext.resetTexture();
            return true;
          });
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showClimateOverview(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginMainTabItem("Climate Generation")) {
    if (uiContext.tabSwitchEvent()) {
      uiContext.imageContext.resetTexture();
    }
    uiContext.helpContext.showHelpTextBox("Climate");

    ImGui::SeparatorText("Base Climate Parameters");

    {
      UI::Elements::GridLayout grid(2, 200.0f, 12.0f);

      if (grid.AddInputDouble("Base Temperature", &cfg.baseTemperature, -100.0,
                              100.0) |
          grid.AddInputDouble("Base Humidity", &cfg.baseHumidity, 0.0, 100.0) |
          grid.AddInputDouble("Fantasy Frequency", &cfg.fantasyClimateFrequency,
                              0.0, 10.0)) {
        uiContext.generationContext.redoHumidity = true;
      }

      // Fantasy climate checkbox on new row
      grid.NextRow();
      ImGui::Checkbox("Fantasy Climate", &cfg.fantasyClimate);
      if (ImGui::IsItemDeactivatedAfterEdit()) {
        uiContext.generationContext.redoHumidity = true;
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
        uiContext.generationContext.redoHumidity = true;
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
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, this]() {
              fwg.genTemperatures(cfg);
              fwg.genHumidity(cfg);
              fwg.genRivers(cfg);
              fwg.genClimate(cfg);
              fwg.genWorldMap(cfg);
              uiContext.imageContext.resetTexture();
              return true;
            });
      }
    }

    if (UI::Elements::BeginSubTabBar("Climate Generation", 0.0f)) {
      Fwg::UI::Climate::showTemperatureMap(cfg, fwg, uiContext);
      Fwg::UI::Climate::showHumidityTab(cfg, fwg, uiContext);
      Fwg::UI::Climate::showRiverTab(cfg, fwg, uiContext);
      Fwg::UI::Climate::showClimateTab(cfg, fwg, uiContext);
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
        uiContext.asyncContext.computationFutureBool =
            uiContext.asyncContext.runAsync([&fwg, &cfg, this]() {
              uiContext.generationContext.modifiedAreas = true;
              fwg.genHabitability(cfg);
              fwg.genSuperSegments(cfg);
              fwg.genSegments(cfg);
              fwg.genProvinces();
              fwg.genContinents(cfg);
              uiContext.imageContext.resetTexture();
              return true;
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

} // namespace Fwg