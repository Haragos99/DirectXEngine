#include "ui.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

UI::~UI()
{
    Shutdown();
}

void UI::Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (initialized)
        return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

    initialized = true;
}

void UI::Shutdown()
{
    if (!initialized)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

void UI::BeginFrame()
{
    if (!initialized)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UI::DrawTestPanel()
{
    if (!initialized)
        return;

    ImGui::Begin("Control Panel");

    ImGui::Text("DirectX Engine - ImGui test panel");
    ImGui::Separator();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Frame time: %.3f ms (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    ImGui::Spacing();
    ImGui::Text("Button clicks: %d", buttonClicks);
    if (ImGui::Button("Click me"))
        ++buttonClicks;
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
        buttonClicks = 0;

    ImGui::Spacing();
    if (ImGui::Button("Cycle render mode"))
        renderModeChangeRequested = true;

    ImGui::Spacing();
    ImGui::ColorEdit3("Clear color", clearColor);

    ImGui::Spacing();
    ImGui::Checkbox("Show ImGui demo window", &showDemoWindow);

    ImGui::End();

    if (showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);
}

void UI::EndFrame()
{
    if (!initialized)
        return;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
