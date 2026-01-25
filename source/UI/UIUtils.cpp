#include "UI//UIUtils.h"

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
  auto &texture = (index == 0) ? primaryTexture : secondaryTexture;
  auto &updateFlag = (index == 0) ? updateTexture1 : updateTexture2;

  // Fwg::Utils::Logging::logLine(
  //     "UIUtils::updateImage: Updating image at index " +
  //     std::to_string(index) + " with size: " + std::to_string(image.size()));
  freeTexture(&texture);
  updateFlag = false;

  const size_t expectedSize = textureWidth * textureHeight;
  if (image.initialised() && image.imageData.size()) {
    try {
      activeImages[index] = image;
      // auto flipped = Fwg::Gfx::Bmp::flip(image);
      if (image.imageData.size() == expectedSize) {
        if (!getResourceView(image, &texture, &textureWidth, &textureHeight,
                             device)) {
          Fwg::Utils::Logging::logLine("ERROR: Couldn't get resource view");
        }
      }
      textureWidth = image.width();
      textureHeight = image.height();
    } catch (const std::exception &e) {
      Fwg::Utils::Logging::logLine(
          std::string("ERROR: Exception in updateImage: ") + e.what());
    }
  }
}

void UIUtils::freeTexture(ID3D11ShaderResourceView **texture) {
  if ((*texture) != nullptr) {
    (*texture)->Release();
    *texture = nullptr;
  }
}

// Simple helper function to load an image into a DX11 texture with common
// settings
bool UIUtils::getResourceView(const Fwg::Gfx::Image &image,
                              ID3D11ShaderResourceView **out_srv,
                              int *out_width, int *out_height,
                              ID3D11Device *g_pd3dDevice) {
  // Load from disk into a raw RGBA buffer
  int image_width = image.width();
  int image_height = image.height();
  std::vector<unsigned char> image_data = image.getFlipped32bit();

  // Create texture
  D3D11_TEXTURE2D_DESC desc;
  ZeroMemory(&desc, sizeof(desc));
  desc.Width = image_width;
  desc.Height = image_height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;

  ID3D11Texture2D *pTexture = NULL;
  D3D11_SUBRESOURCE_DATA subResource;
  subResource.pSysMem = image_data.data();
  subResource.SysMemPitch = desc.Width * 4;
  subResource.SysMemSlicePitch = 0;
  g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
  if (pTexture != nullptr) {
    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();
  } else {
    return false;
  }
  *out_width = image_width;
  *out_height = image_height;
  return true;
}

ImGuiIO UIUtils::setupImGuiContextAndStyle() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  auto &io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
  ImGui::StyleColorsDark();
  return io;
}

HWND UIUtils::createAndConfigureWindow(WNDCLASSEXW &wc, LPCWSTR className,
                                       LPCWSTR windowName) {
  HWND hwnd =
      ::CreateWindowW(wc.lpszClassName, windowName, WS_OVERLAPPEDWINDOW, 100,
                      100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);
  MONITORINFO monitor_info;
  monitor_info.cbSize = sizeof(monitor_info);
  GetMonitorInfo(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST),
                 &monitor_info);
  SetWindowPos(hwnd, NULL, 0, 0,
               monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
               monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  return hwnd;
}

void UIUtils::setupImGuiBackends(HWND hwnd, ID3D11Device *g_pd3dDevice,
                                 ID3D11DeviceContext *g_pd3dDeviceContext) {
  ImGui_ImplWin32_Init(hwnd);
  ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
}

void UIUtils::cleanupDirect3DDevice(ID3D11Device *g_pd3dDevice) {
  if (g_pd3dDevice) {
    g_pd3dDevice->Release();
    g_pd3dDevice = nullptr;
  }
}

void UIUtils::renderImGui(ID3D11DeviceContext *g_pd3dDeviceContext,
                          ID3D11RenderTargetView *g_mainRenderTargetView,
                          const ImVec4 &clear_color,
                          IDXGISwapChain *g_pSwapChain) {
  ImGui::Render();
  const float clear_color_with_alpha[4] = {
      clear_color.x * clear_color.w, clear_color.y * clear_color.w,
      clear_color.z * clear_color.w, clear_color.w};
  g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
  g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView,
                                             clear_color_with_alpha);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  g_pSwapChain->Present(1, 0); // Present with vsync
}

void UIUtils::shutdownImGui() {
  ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
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

} // namespace Masks

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
  auto imageDirectory = path + "//uiHelpImages//";

  for (const auto &entry :
       std::filesystem::directory_iterator(imageDirectory)) {
    if (entry.is_directory()) {
      continue; // Skip directories
    }
    // get the filename without extension
    auto filename = entry.path().filename().string();
    // remove the extension
    filename = filename.substr(0, filename.find_last_of('.'));
    auto image = Fwg::Gfx::Png::load(entry.path().string());
    auto texWidth = image.width();
    auto texHeight = image.height();
    ID3D11ShaderResourceView *texture = nullptr;
    if (texWidth > 0 && texHeight > 0) {
      getResourceView(image, &texture, &texWidth, &texHeight, device);
      advancedHelpTextures[filename] = texture;
      advancedHelpTexturesAspectRatio[filename] =
          static_cast<float>(texHeight) / static_cast<float>(texWidth);
    }
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
      ImGui::Image(advancedHelpTextures[key],
                   ImVec2(imageWidth,
                          imageWidth * advancedHelpTexturesAspectRatio[key]));
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + textWidth);
    RenderEmphasizedText(value, textWidth);
    // ImGui::TextUnformatted(value.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndGroup();

    ImGui::Separator();
    ImGui::End();            // Ends the "Extended Help" window
    ImGui::PopStyleColor(2); // WindowBg + Border
    ImGui::PopStyleVar();    // Border size
  }
}