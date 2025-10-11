#include "UI/FwgUI.h"

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd,
                                                             UINT msg,
                                                             WPARAM wParam,
                                                             LPARAM lParam);
namespace Fwg {
// Data
ID3D11Device *FwgUI::g_pd3dDevice = nullptr;
ID3D11DeviceContext *FwgUI::g_pd3dDeviceContext = nullptr;
IDXGISwapChain *FwgUI::g_pSwapChain = nullptr;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
ID3D11RenderTargetView *FwgUI::g_mainRenderTargetView = nullptr;
int FwgUI::seed = 0;

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if
// dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your
// main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to
// your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from
// your application based on those two flags.
// LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg) {
  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED)
      return 0;
    g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
    g_ResizeHeight = (UINT)HIWORD(lParam);
    return 0;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }
  return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

FwgUI::FwgUI() {

  uiUtils = std::make_shared<UIUtils>();
  landUI = LandUI(uiUtils);

}

WNDCLASSEXW FwgUI::initializeWindowClass() {
  WNDCLASSEXW wc = {sizeof(wc),
                    CS_CLASSDC,
                    WndProc,
                    0L,
                    0L,
                    GetModuleHandle(nullptr),
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    L"FastWorldGen",
                    nullptr};

  HICON hIcon = (HICON)LoadImage(
      NULL,
      (Cfg::Values().workingDirectory + "//resources//worldMap.ico").c_str(),
      IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
  if (hIcon) {
    wc.hIcon = hIcon;
    HWND consoleWindow = GetConsoleWindow();
    SendMessage(consoleWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessage(consoleWindow, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
  }
  return wc;
}

bool FwgUI::initializeGraphics(HWND hwnd) {
  if (!CreateDeviceD3D(hwnd)) {
    CleanupDeviceD3D();
    return false;
  }
  ::ShowWindow(hwnd, SW_SHOWDEFAULT);
  ::UpdateWindow(hwnd);
  return true;
}

void FwgUI::initializeImGui(HWND hwnd) {
  uiUtils->setupImGuiContextAndStyle();
  ImGui::StyleColorsDark();
  uiUtils->setupImGuiBackends(hwnd, g_pd3dDevice, g_pd3dDeviceContext);
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
    ImGui::EndChild();
    ImGui::PopStyleColor();
    // Draw a frame around the child region
    ImVec2 childMin = ImGui::GetItemRectMin();
    ImVec2 childMax = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRect(
        childMin, childMax, IM_COL32(25, 91, 133, 255), 0.0f, 0, 2.0f);
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

  float modif = 1.0 - (uiUtils->secondaryTexture != nullptr) * 0.5;
  if (uiUtils->textureWidth > 0 && uiUtils->textureHeight > 0) {
    float aspectRatio =
        (float)uiUtils->textureWidth / (float)uiUtils->textureHeight;
    auto scale = std::min<float>(
        (ImGui::GetContentRegionAvail().y) * modif / uiUtils->textureHeight,
        (ImGui::GetContentRegionAvail().x) / uiUtils->textureWidth);
    auto texWidth = uiUtils->textureWidth * scale;
    auto texHeight = uiUtils->textureHeight * scale;

    // Handle zooming
    if (io.KeyCtrl) {
      zoom += io.MouseWheel * 0.1f;
    }

    // Create a child window for the image
    ImGui::BeginChild("ImageContainer", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar |
                          ImGuiWindowFlags_AlwaysVerticalScrollbar);
    {
      ImGui::BeginChild("Image", ImVec2(texWidth, texHeight), false,
                        ImGuiWindowFlags_HorizontalScrollbar |
                            ImGuiWindowFlags_AlwaysVerticalScrollbar);
      if (uiUtils->primaryTexture != nullptr) {
        ImGui::Image((void *)uiUtils->primaryTexture,
                     ImVec2(texWidth * zoom * 0.98, texHeight * zoom * 0.98));
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
        if (io.KeyCtrl && ImGui::IsMouseDragging(0, 0.0f)) {
          ImVec2 drag_delta = ImGui::GetMouseDragDelta(0, 0.0f);
          ImGui::ResetMouseDragDelta(0);
          ImGui::SetScrollX(ImGui::GetScrollX() - drag_delta.x);
          ImGui::SetScrollY(ImGui::GetScrollY() - drag_delta.y);
        }
        if (!io.KeyCtrl) {
          uiUtils->imageClick(scale, io);
        }
      }
      // End the child window
      ImGui::EndChild();

      if (uiUtils->secondaryTexture != nullptr) {
        ImGui::BeginChild("ImageSecondary", ImVec2(texWidth, texHeight), false,
                          ImGuiWindowFlags_HorizontalScrollbar |
                              ImGuiWindowFlags_AlwaysVerticalScrollbar);
        // images are less wide, on a usual 16x9 monitor, it is better
        // to place them besides each other if (aspectRatio <= 2.0)
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
  initAllowedInput(cfg, fwg.climateData, fwg.terrainData.elevationTypes);
}

void FwgUI::initDraggingPoll(bool &done) {
  // reset dragging all the time in case it wasn't handled in a tab on
  // purpose
  triggeredDrag = false;
  // Poll and handle messages (inputs, window resize, etc.)
  // See the WndProc() function below for our to dispatch events to the
  // Win32 backend.
  MSG msg;
  while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
    if (msg.message == WM_QUIT)
      done = true;
    else if (msg.message == WM_DROPFILES) {
      HDROP hDrop = reinterpret_cast<HDROP>(msg.wParam);

      // extract files here
      std::vector<std::string> files;
      char filename[MAX_PATH];

      UINT count = DragQueryFileA(hDrop, -1, NULL, 0);
      for (UINT i = 0; i < count; ++i) {
        if (DragQueryFileA(hDrop, i, filename, MAX_PATH)) {
          files.push_back(filename);
          // Fwg::Utils::Logging::logLine("Loaded file ", filename);
        }
      }
      draggedFile = files.back();
      triggeredDrag = true;
      DragFinish(hDrop);
    }
  }

  // Handle window resize (we don't resize directly in the WM_SIZE
  // handler)
  if (g_ResizeWidth != 0 && g_ResizeHeight != 0) {
    CleanupRenderTarget();
    g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight,
                                DXGI_FORMAT_UNKNOWN, 0);
    g_ResizeWidth = g_ResizeHeight = 0;
    CreateRenderTarget();
  }
}

void FwgUI::defaultTabs(Fwg::Cfg &cfg, FastWorldGenerator &fwg) {
  showElevationTabs(cfg, fwg);
  showClimateInputTab(cfg, fwg);
  showClimateOverview(cfg, fwg);
  showAreasTab(cfg, fwg);
  //showCivilizationTab(cfg, fwg);
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

void FwgUI::cleanup(HWND hwnd, const WNDCLASSEXW &wc) {
  uiUtils->shutdownImGui();
  CleanupDeviceD3D();
  ::DestroyWindow(hwnd);
  ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

int FwgUI::shiny(Fwg::FastWorldGenerator &fwg) {
  try {
    //  Create application window
    //  ImGui_ImplWin32_EnableDpiAwareness();
    auto wc = initializeWindowClass();

    HWND consoleWindow = GetConsoleWindow();

    ::RegisterClassExW(&wc);
    HWND hwnd = uiUtils->createAndConfigureWindow(wc, wc.lpszClassName,
                                                  L"FastWorldGen 0.9.1");
    initializeGraphics(hwnd);
    initializeImGui(hwnd);
    auto &io = ImGui::GetIO();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    auto &cfg = Fwg::Cfg::Values();
    // Main loop
    bool done = false;
    //--- prior to main loop:
    DragAcceptFiles(hwnd, TRUE);
    uiUtils->primaryTexture = nullptr;
    uiUtils->device = g_pd3dDevice;
    namespace fs = std::filesystem;

    init(cfg, fwg);

    while (!done) {
      try {
        initDraggingPoll(done);
        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
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
              ImGui::SeparatorText(
                  "Different Steps of the generation, usually go "
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
              ImGui::GetWindowDrawList()->AddRect(childMin, childMax,
                                                  IM_COL32(100, 90, 180, 255),
                                                  0.0f, 0, 2.0f);
            }
            genericWrapper(cfg, fwg);
            logWrapper();
          }
          ImGui::SameLine();
          imageWrapper(io);

          ImGui::End();
        }

        // Rendering
        uiUtils->renderImGui(g_pd3dDeviceContext, g_mainRenderTargetView,
                             clear_color, g_pSwapChain);
      } catch (std::exception e) {
        Fwg::Utils::Logging::logLine("Error in GUI main loop: ", e.what());
      }
    }

    cleanup(hwnd, wc);
    return 0;
  } catch (std::exception e) {
    Fwg::Utils::Logging::logLine("Error in GUI startup: ", e.what());
    return -1;
  }
}

void FwgUI::disableBlock(const Fwg::Gfx::Bitmap &bitmap) {
  if (!bitmap.initialised())
    ImGui::BeginDisabled();
}
void FwgUI::reenableBlock(const Fwg::Gfx::Bitmap &bitmap) {
  if (!bitmap.initialised())
    ImGui::EndDisabled();
}
void FwgUI::initAllowedInput(
    Fwg::Cfg &cfg, Fwg::Climate::ClimateData &climateData,
    std::vector<Terrain::ElevationType> &elevationTypes) {
  Fwg::Utils::Logging::logLine("Initialising allowed input");
  auto &climateTypes = climateData.climateTypes;
  climateUI.allowedClimateInputs.clear();
  for (const auto &climateType : climateTypes) {
    climateUI.allowedClimateInputs.setValue(climateType.primaryColour,
                                            climateType);
  }

  for (const auto &elevationType : elevationTypes) {
    landUI.allowedLandInputs.setValue(elevationType.colour, elevationType);
  }
};

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
  // uiUtils->brushSettingsHeader();
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
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0, landUI.landInput);
      uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
    }
    uiUtils->showHelpTextBox("Land");

    if (ImGui::Checkbox("<--Classify land input", &cfg.complexLandInput)) {
      if (cfg.complexLandInput) {
        cfg.readHeightmapConfig(cfg.workingDirectory +
                                "//configs//heightmap//" + "mappedInput.json");
      } else {
        cfg.readHeightmapConfig(cfg.workingDirectory +
                                "//configs//heightmap//" + "default.json");
      }
    }
    if (cfg.complexLandInput && landUI.landInput.size()) {
      landUI.complexLandMapping(cfg, fwg, analyze, amountClassificationsNeeded);
    }
    // drag event
    if (triggeredDrag) {
      cfg.allowHeightmapModification = true;
      originalLandInput = draggedFile;
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        landUI.triggeredLandInput(cfg, fwg, originalLandInput,
                                  cfg.complexLandInput);
        uiUtils->updateImage(0, landUI.landInput);
        // after the first drag, we have saved the original input to this new
        // file now we always want to reload it from here to get the progressive
        // changes, never overwriting the original
        originalLandInput = cfg.mapsPath + "//classifiedLandInput.bmp";
        // in case of complex input and a drag, we NEED to initially analyze
        if (cfg.complexLandInput) {
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
  if (UI::Elements::BeginSubTabItem("Heightmap")) {
    if (uiUtils->tabSwitchEvent()) {
      if (fwg.terrainData.detailedHeightMap.size()) {
        auto heightmap =
            Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap);
        uiUtils->updateImage(1, heightmap);
        uiUtils->updateImage(0, heightmap);
        uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
        // wrap around because we want to show two images
        if (updateLayer) {
          if (selectedLayer < fwg.layerData.size() &&
              fwg.layerData[selectedLayer].size() &&
              fwg.terrainData.detailedHeightMap.size()) {
            uiUtils->updateImage(
                1, Fwg::Gfx::Bitmap(cfg.width, cfg.height, 24,
                                    fwg.layerData[selectedLayer]));
          }
          updateLayer = false;
        } else {
          if (fwg.terrainData.landMap.size()) {
            uiUtils->updateImage(1, Fwg::Gfx::landMap(fwg.terrainData));
          } else {
            uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
          }
        }
      } else {

        uiUtils->updateImage(0, Fwg::Gfx::Bitmap());
        uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
      }
    }
    static bool layeredit = false;
    uiUtils->showHelpTextBox("Heightmap");
    // showCutCfg(cfg, fwg);
    ImGui::PushItemWidth(100.0f);
    ImGui::SeparatorText(
        "Generate a simple overview of land area from heightmap "
        "or drop it in");
    if (!cfg.complexLandInput) {
      // UI::Elements::LabeledInputInt("SeaLevel", cfg.seaLevel, 1, 10, 0, 255);
      // UI::Elements::LabeledInputInt("SeaLevel", cfg.seaLevel, 1, 10, 0, 255);
      // ImGui::SliderFloat("<--Landpercentage", &cfg.landPercentage,
      // 0.00, 1.0); UI::Elements::LabeledInputInt("<--Height Adjustments",
      //                              cfg.heightAdjustments, 1, 10, -255, 255);
      // UI::Elements::LabeledInputFloat("<--Landlayer coastal distance factor",
      //                                cfg.layerApplicationFactor, 0.01f, 0.1f,
      //                                0.0f, 1.0f);
      ImGui::InputInt("<--SeaLevel", &cfg.seaLevel, 1, 10);
      ImGui::SliderFloat("<--Landpercentage", &cfg.landPercentage, 0.00, 1.0);
      ImGui::InputInt("<--Height Adjustments", &cfg.heightAdjustments);
      ImGui::InputFloat("<--Landlayer coastal distance factor",
                        &cfg.layerApplicationFactor, 0.1f, 0.1f);
    } else {
      ImGui::Text("Sealevel: %d", cfg.seaLevel);
      ImGui::Text("Landpercentage: %f", cfg.landPercentage);
    }
    // ImGui::InputDouble("<--Inclination factor", &cfg.inclinationFactor,
    // 0.1f);
    ImGui::InputDouble("<--Maximum Lake Size Factor", &cfg.lakeMaxShare, 0.01f);
    if (ImGui::InputInt("<--Maximum Land height", &cfg.maxLandHeight)) {
      cfg.maxLandHeight = std::max<int>(cfg.maxLandHeight, cfg.seaLevel);
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(100.0f);
    ImGui::SameLine();
    if (ImGui::SliderFloat("<--Overall Frequency", &frequency, 0.1f, 10.0f,
                           "ratio = %.1f")) {
    } else {
      if (!ImGui::IsMouseDown(0) &&
          frequency != (float)cfg.overallFrequencyModifier) {
        cfg.overallFrequencyModifier = frequency;
        update = true;
      }
    }
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
        ImGui::InputInt("<--Noise Type", &cfg.noiseType[selectedLayer]);
        ImGui::InputInt("<--Fractal Type", &cfg.fractalType[selectedLayer]);
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
    std::string buttonText = "Generate land shape";
    if (!cfg.complexLandInput && ImGui::Button(buttonText.c_str())) {
      // we have dragged in a terrain map before, if we generate, we just want
      // to generate with changed parameters
      // This case is only used in case of simple input
      if (landUI.loadedTerrainFile.size()) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genHeightFromInput(cfg, this->landUI.loadedTerrainFile);
          fwg.genLand();
          uiUtils->resetTexture();
          updateLayer = true;
          return true;
        });
      } else {
        computationFutureBool = runAsync([&fwg, this]() {
          fwg.genHeight();
          uiUtils->resetTexture();
          updateLayer = true;
          return true;
        });
        updateLayer = true;
      }
    }

    ImGui::PopItemWidth();
    landUI.configureLandElevationFactors(cfg, fwg);

    ImGui::PushItemWidth(300.0f);
    // only allow this if classification has been done
    if (amountClassificationsNeeded > 0 || analyze) {
      ImGui::BeginDisabled();
    }
    const auto str = cfg.complexLandInput
                         ? "Generate heightmap from land classification"
                         : "Generate land classification from heightmap";
    if (ImGui::Button(str)) {
      if (cfg.complexLandInput) {
        fwg.genHeightFromInput(cfg, cfg.mapsPath + "//classifiedLandInput.bmp");
        uiUtils->updateImage(
            0, Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap));
        uiUtils->updateImage(1, Fwg::Gfx::landMap(fwg.terrainData));
      }
      computationFutureBool = runAsync([&fwg, this]() {
        fwg.genLand();
        updateLayer = true;
        uiUtils->resetTexture();
        return true;
      });
    }
    // if (ImGui::Button("Generate everything from heightmap")) {
    //   // run the generation async
    //   computationFutureBool = runAsync([&fwg, this]() {
    //     worldGenerationView = true;
    //     fwg.generateRemaining();
    //     worldGenerationView = false;
    //     return true;
    //   });
    // }
    if (amountClassificationsNeeded > 0 || analyze) {
      ImGui::EndDisabled();
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

void FwgUI::clearColours(Fwg::Gfx::Bitmap &image) {
  static int severity = 0;
  // first count every colour in a colourTMap
  Utils::ColourTMap<std::vector<int>> colourCounter;
  for (int pix = 0; pix < image.size(); pix++) {
    const auto &col = image[pix];
    if (!colourCounter.find(col)) {
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
  // std::sort(colorsSortedByDistance.begin(), colorsSortedByDistance.end(),
  //           [](const Fwg::Gfx::Colour a, const Fwg::Gfx::Colourb) {
  //             return (a.dis < *b);
  //           }););
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
    ImGui::InputInt("Sobelfactor", &cfg.sobelFactor);
    ImGui::PopItemWidth();
    if (fwg.terrainData.detailedHeightMap.size()) {
      if (ImGui::Button("Generate Normalmap")) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.genSobelMap(cfg);
          uiUtils->resetTexture(0);
          return true;
        });
      }
    } else {
      ImGui::Text("Have a heightmap first before you can generate it from the "
                  "heightmap");
    }
    // drag event
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
      // force update so sub-selected tabs get updated
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
    if (fwg.terrainData.landForms.size() > 0 &&
        fwg.terrainData.landForms.size() ==
            fwg.terrainData.detailedHeightMap.size() &&
        UI::Elements::AutomationStepButton(
            "Generate whole climate automatically")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genTemperatures(cfg);
        fwg.genHumidity(cfg);
        fwg.genRivers(cfg);
        fwg.genClimate(cfg);
        fwg.genWorldMap(cfg);
        // force update so sub-selected tabs get updated
        uiUtils->setForceUpdate();
        uiUtils->resetTexture();
        return true;
      });
    }
    ImGui::PopItemWidth();
    if (!fwg.terrainData.landForms.size()) {

      ImGui::Text("Have a heightmap, land classification and normalmap "
                  "first");
      ImGui::BeginDisabled();
    }
    if (UI::Elements::BeginSubTabBar("Climate Generation", 0.0f)) {
      showTemperatureMap(cfg, fwg);
      showHumidityTab(cfg, fwg);
      showRiverTab(cfg, fwg);
      showClimateTab(cfg, fwg);
      showTreeTab(cfg, fwg);
      UI::Elements::EndSubTabBar();
    }

    if (!fwg.terrainData.landForms.size()) {
      ImGui::EndDisabled();
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
      uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
    }
    ImGui::SeparatorText("This step is OPTIONAL! You can also generate a "
                         "random climate in the next tab");
    uiUtils->showHelpTextBox("Climate Input");
    if (triggeredDrag) {
      triggeredDrag = false;
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        // don't immediately generate from the input, instead allow to manually
        // classify all present colours
        climateUI.climateInputMap =
            Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
        // create a map from secondary colours to primary colours
        Fwg::Utils::ColourTMap<Fwg::Climate::ClimateType> secondaryToPrimary;
        for (auto &type : fwg.climateData.climateTypes) {
          for (auto &secondary : type.secondaryColours) {
            secondaryToPrimary.setValue(secondary, type);
          }
        }

        // preprocess input to convert to primary colours where possible
        for (auto &col : climateUI.climateInputMap.imageData) {
          if (secondaryToPrimary.find(col)) {
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
    // only allow this if classification has been done
    if (amountClassificationsNeeded > 0 || analyze) {
      ImGui::BeginDisabled();
    }
    if (!amountClassificationsNeeded && climateUI.climateInputMap.size() &&
        ImGui::Button("Generate from labeled climate")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        cfg.complexClimateInput = true;
        Fwg::Gfx::Bmp::save(climateUI.climateInputMap,
                            cfg.mapsPath + "//classifiedClimateInput.bmp");
        fwg.loadClimate(cfg, climateUI.climateInputMap);
        uiUtils->resetTexture();
        return true;
      });
    }
    if (amountClassificationsNeeded > 0 || analyze) {
      ImGui::EndDisabled();
    }

    ImGui::EndTabItem();
  }
  return 0;
};

int FwgUI::showTemperatureMap(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Temperature")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(
          0, Fwg::Gfx::Climate::displayTemperature(fwg.climateData));
      uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
    }
    uiUtils->showHelpTextBox("Temperature");

    ImGui::SeparatorText("Generate temperature map or drop it in. You can also "
                         "draw in this map");
    if (ImGui::Button("Generate Temperature Map")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genTemperatures(cfg);
        uiUtils->resetTexture();
        return true;
      });
    }
    // drag event
    if (triggeredDrag) {
      fwg.loadTemperatures(cfg, draggedFile);
      triggeredDrag = false;
      uiUtils->resetTexture();
    }
    // if (uiUtils->simpleDraw(fwg.terrainData.landMap,
    //                         fwg.climateData.averageTemperatures, 1.0f)) {
    //   displayImage = Fwg::Gfx::Climate::displayTemperature(fwg.climateData);
    // }

    ImGui::EndTabItem();
  }
  return 0;
}
int FwgUI::showHumidityTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Humidity")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0,
                           Fwg::Gfx::Climate::displayHumidity(fwg.climateData));
      uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
    }
    uiUtils->showHelpTextBox("Humidity");
    ImGui::SeparatorText(
        "Generate humidity map or drop it in. You can also draw in this map");
    if (ImGui::Button("Generate Humidity Map")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genHumidity(cfg);
        uiUtils->resetTexture(0);
        return true;
      });
    }
    // drag event
    if (triggeredDrag) {
      fwg.loadHumidity(cfg,
                       Fwg::IO::Reader::readGenericImage(draggedFile, cfg));
      redoHumidity = false;
      triggeredDrag = false;
      uiUtils->resetTexture();
    }
    // if (uiUtils->simpleDraw(fwg.terrainData.landMap,
    // fwg.climateData.humidities,
    //                         1.0f)) {
    //   displayImage = Gfx::Climate::displayHumidity(fwg.climateData);
    //   // overwrite to be able to reset after applying river humidity
    //   fwg.preModifyHumidityMap = fwg.climateData.humidities;
    // }
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showRiverTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Rivers")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0, Gfx::riverMap(fwg.terrainData.detailedHeightMap,
                                            fwg.climateData.rivers));
      uiUtils->updateImage(1, Fwg::Gfx::Bitmap());
    }
    uiUtils->showHelpTextBox("Rivers");
    if (!fwg.climateData.humidities.size()) {
      ImGui::BeginDisabled();
    }
    ImGui::InputDouble("River amount multiplier", &cfg.riverFactor);
    if (ImGui::Button("Generate River Map")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genRivers(cfg);
        uiUtils->resetTexture();
        return true;
      });
    }

    // drag event
    if (triggeredDrag) {
      fwg.loadRivers(cfg, Fwg::IO::Reader::readGenericImage(draggedFile, cfg));
      uiUtils->resetTexture();
      triggeredDrag = false;
    }

    ImGui::Value("Amount of rivers: ", (int)fwg.climateData.rivers.size());
    if (!fwg.climateData.humidities.size())
      ImGui::EndDisabled();
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
    if (!fwg.climateData.humidities.size())
      ImGui::BeginDisabled();
    if (ImGui::Button("Generate")) {
      computationFutureBool =
          runAsync([&fwg, &cfg, this]() { // noticed a change of humidity
                                          // parameters, so we redo the humidity
            // generation before generating climate map
            if (redoHumidity) {
              fwg.genTemperatures(cfg);
              fwg.genHumidity(cfg);
              redoHumidity = false;
            }
            fwg.genClimate(cfg);
            uiUtils->resetTexture();
            return true;
          });
    }
    if (!fwg.climateData.humidities.size())
      ImGui::EndDisabled();
    // drag event
    if (triggeredDrag) {
      // only try to load climate if the complexTerrain is initialized
      if (climateUI.climateInputMap.initialised()) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          fwg.loadClimate(cfg, climateUI.climateInputMap);
          fwg.genWorldMap(cfg);
          uiUtils->resetTexture();
          return true;
        });
      } else {
        int amountClassificationsNeeded;
        auto climateInput = Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
        // load a valid map if no classificationsNeeded
        if (climateUI.analyzeClimateMap(cfg, fwg, climateInput,
                                        amountClassificationsNeeded)) {
          fwg.loadClimate(cfg, climateInput);
          uiUtils->resetTexture();
        } else {
          Utils::Logging::logLine(
              "You are trying to load a climate input that has incompatible "
              "colours. If you want to use a complex climate map as input, "
              "please use the Climate Input tab label the climate zones. The "
              "resulting map will be used as "
              "climate input here automatically.");
        }
      }
      triggeredDrag = false;
      uiUtils->resetTexture();
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

    if (ImGui::Button("Generate Treemap")) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genForests(cfg);
        uiUtils->resetTexture();
        return true;
      });
    }

    ImGui::PopItemWidth();
    // drag event
    if (triggeredDrag) {
      fwg.loadForests(cfg, draggedFile);
      triggeredDrag = false;
      uiUtils->resetTexture();
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
    // drag event
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
    // uiUtils->showHelpTextBox("Areas");
    //  Button to generate segments
    if (fwg.climateData.size() && fwg.climateData.climates.size() &&
        fwg.terrainData.landForms.size()) {
      if (UI::Elements::AutomationStepButton(
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
    if (UI::Elements::BeginSubTabBar("Area Tabs", 0.0f)) {
      showDensityTab(cfg, fwg);
      showSuperSegmentTab(cfg, fwg);
      showSegmentTab(cfg, fwg);
      showProvincesTab(cfg, fwg);
      showContinentTab(cfg, fwg);
      // showRegionTab(cfg, fwg, &primaryTexture);
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
          fwg.climateData.climates.size() && fwg.terrainData.landForms.size()) {
        fwg.genHabitability(cfg);
      }
      uiUtils->updateImage(
          0, Gfx::displayHabitability(fwg.climateData.habitabilities));
      uiUtils->updateImage(1, fwg.worldMap);
    }

    uiUtils->showHelpTextBox("Density");
    if (fwg.climateData.size() && fwg.climateData.climates.size() &&
        fwg.terrainData.landForms.size()) {
      if (ImGui::Button("Generate density map")) {
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
    } else {
      ImGui::Text("Generate climate first");
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
    if (ImGui::Button("Generate SuperSegments") ||
        (fwg.climateData.habitabilities.size() == cfg.bitmapSize &&
         fwg.areaData.superSegments.empty() && !computationRunning)) {
      computationFutureBool = runAsync([&fwg, &cfg, this]() {
        fwg.genSuperSegments(cfg);
        uiUtils->resetTexture();
        return true;
      });
    }
    ImGui::PopItemWidth();
    if (fwg.climateData.climates.size()) {
      if (triggeredDrag) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          triggeredDrag = false;
          fwg.loadSuperSegments(
              cfg, Fwg::IO::Reader::readGenericImage(draggedFile, cfg));
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
    if (fwg.climateData.climates.size()) {
      ImGui::Text("The map has %i land segments",
                  static_cast<int>(fwg.areaData.landSegments));
      ImGui::Text("The map has %i sea segments",
                  static_cast<int>(fwg.areaData.seaSegments));
      ImGui::Text("The map has %i lake segments",
                  static_cast<int>(fwg.areaData.lakeSegments));

      // Button to generate segments
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
          modifiedAreas = true;
          fwg.loadSegments(cfg,
                           Fwg::IO::Reader::readGenericImage(draggedFile, cfg));
          fwg.segmentMap =
              Fwg::Gfx::Segments::displaySegments(fwg.areaData.segments);
          triggeredDrag = false;
          uiUtils->resetTexture();
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
    if (fwg.terrainData.detailedHeightMap.size() &&
        fwg.terrainData.landForms.size() &&
        fwg.climateData.habitabilities.size()) {
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
        // redoRegions = true;
      }
      if (triggeredDrag) {
        modifiedAreas = true;
        triggeredDrag = false;
        fwg.loadProvinces(cfg, draggedFile);
        uiUtils->resetTexture();
      }
    } else {
      ImGui::SeparatorText("Generator other maps first");
    }

    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showRegionTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Regions")) {
    uiUtils->tabSwitchEvent();
    uiUtils->showHelpTextBox("Regions");

    if (fwg.terrainData.detailedHeightMap.size() &&
        fwg.terrainData.landForms.size() &&
        fwg.climateData.habitabilities.size() &&
        fwg.provinceMap.initialised()) {
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
          } catch (std::exception e) {
            Utils::Logging::logLine(
                "Couldn't load regions, fix input or try again");
            fwg.regionMap = Fwg::IO::Reader::readGenericImage(draggedFile, cfg);
          }
        }
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    } else {
      ImGui::SeparatorText("Generator other maps first");
    }
    ImGui::EndTabItem();
  }
  return 0;
}

int FwgUI::showContinentTab(Fwg::Cfg &cfg, Fwg::FastWorldGenerator &fwg) {
  if (UI::Elements::BeginSubTabItem("Continents")) {
    if (uiUtils->tabSwitchEvent()) {
      uiUtils->updateImage(0,
                           Fwg::Gfx::simpleContinents(fwg.areaData.continents,
                                                      fwg.areaData.seaBodies));
      uiUtils->updateImage(
          1, Fwg::Gfx::displayHeightMap(fwg.terrainData.detailedHeightMap));
    }
    uiUtils->showHelpTextBox("Continents");
    ImGui::InputInt("Maximum amount of continents", &cfg.maxAmountOfContinents,
                    1);
    if (fwg.areaData.landBodies.size() && fwg.areaData.provinces.size()) {
      if (ImGui::Button("Generate Continents") ||
          (fwg.areaData.continents.empty() && !computationRunning)) {
        computationFutureBool = runAsync([&fwg, &cfg, this]() {
          modifiedAreas = true;
          fwg.genContinents(cfg);
          uiUtils->resetTexture();
          return true;
        });
      }
      // drag event
      if (triggeredDrag) {
        modifiedAreas = true;
        fwg.loadContinents(cfg,
                           Fwg::IO::Reader::readGenericImage(draggedFile, cfg));
        triggeredDrag = false;
        uiUtils->resetTexture();
      }
    } else {
      if (!fwg.areaData.landBodies.size()) {
        ImGui::Text("Generate landbodies first");
      } else if (!fwg.areaData.provinces.size()) {
        ImGui::Text("Generate provinces first");
      }
    }

    ImGui::EndTabItem();
  }
  return 0;
}

// Helper functions

bool FwgUI::CreateDeviceD3D(HWND hWnd) {
  // Setup swap chain
  DXGI_SWAP_CHAIN_DESC sd;
  ZeroMemory(&sd, sizeof(sd));
  sd.BufferCount = 2;
  sd.BufferDesc.Width = 0;
  sd.BufferDesc.Height = 0;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  UINT createDeviceFlags = 0;
  // createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
  D3D_FEATURE_LEVEL featureLevel;
  const D3D_FEATURE_LEVEL featureLevelArray[2] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_0,
  };
  HRESULT res = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
      featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
      &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
  if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software
                                     // driver if hardware is not available.
    res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
  if (res != S_OK)
    return false;

  CreateRenderTarget();
  return true;
}

void FwgUI::CleanupDeviceD3D() {
  CleanupRenderTarget();
  if (g_pSwapChain) {
    g_pSwapChain->Release();
    g_pSwapChain = nullptr;
  }
  if (g_pd3dDeviceContext) {
    g_pd3dDeviceContext->Release();
    g_pd3dDeviceContext = nullptr;
  }
  uiUtils->cleanupDirect3DDevice(g_pd3dDevice);
}

void FwgUI::CreateRenderTarget() {
  ID3D11Texture2D *pBackBuffer;
  g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
  if (pBackBuffer != nullptr) {
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr,
                                         &g_mainRenderTargetView);
    pBackBuffer->Release();
  } else {
    throw(std::exception("Could not get buffer of RenderTargetView"));
  }
}

void FwgUI::CleanupRenderTarget() {
  if (g_mainRenderTargetView) {
    g_mainRenderTargetView->Release();
    g_mainRenderTargetView = nullptr;
  }
}

} // namespace Fwg