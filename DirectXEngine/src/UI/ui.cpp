#include "ui.h"

#include <commdlg.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#pragma comment(lib, "comdlg32.lib")

UIPanel::~UIPanel()
{
    Shutdown();
}

void UIPanel::Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (initialized)
        return;

    this->hwnd = hwnd;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(device, context);

    initialized = true;
}

void UIPanel::Shutdown()
{
    if (!initialized)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    initialized = false;
}

void UIPanel::BeginFrame()
{
    if (!initialized)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

static std::string GetSceneObjectLabel(const std::shared_ptr<Object3D>& object)
{
    if (!object)
    {
        return "<null>";
    }

    return object->GetName();
}

void UIPanel::DrawTestPanel(const std::vector<std::shared_ptr<Object3D>>& sceneObjects)
{
    if (!initialized)
        return;

    // Drop stale selection if the scene shrank.
    if (selectedSceneObjectIndex >= static_cast<int>(sceneObjects.size()))
        selectedSceneObjectIndex = -1;

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
    if (ImGui::Button("Import OBJ model"))
        OpenModelDialog();

    if (!selectedModelPath.empty())
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Loaded");
        ImGui::TextWrapped("Path: %ls", selectedModelPath.c_str());
    }
    else
    {
        ImGui::SameLine();
        ImGui::TextDisabled("No file selected");
    }

    ImGui::Spacing();
    ImGui::Text("Scene objects");
    if (ImGui::BeginChild("SceneObjectList", ImVec2(0.0f, 140.0f), true))
    {
        if (sceneObjects.empty())
        {
            ImGui::TextDisabled("No scene objects yet");
        }
        else
        {
            for (size_t i = 0; i < sceneObjects.size(); ++i)
            {
                const std::string label = GetSceneObjectLabel(sceneObjects[i]) + " #" + std::to_string(i);
                const bool isSelected = (selectedSceneObjectIndex == static_cast<int>(i));
                if (ImGui::Selectable(label.c_str(), isSelected))
                    selectedSceneObjectIndex = static_cast<int>(i);
            }
        }
        ImGui::EndChild();
    }

    if (HasSelection() && selectedSceneObjectIndex < static_cast<int>(sceneObjects.size()))
    {
        ImGui::Text("Selected: %s", GetSceneObjectLabel(sceneObjects[selectedSceneObjectIndex]).c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear selection"))
            selectedSceneObjectIndex = -1;

        // Gizmo interaction mode: drag the axis arrows to move or to scale.
        ImGui::Text("Gizmo mode");
        int mode = static_cast<int>(gizmoMode);
        ImGui::RadioButton("Move", &mode, static_cast<int>(GizmoMode::Move));
        ImGui::SameLine();
        ImGui::RadioButton("Scale", &mode, static_cast<int>(GizmoMode::Scale));
        gizmoMode = static_cast<GizmoMode>(mode);
    }
    else
    {
        ImGui::TextDisabled("No object selected");
    }

    ImGui::Spacing();
    ImGui::ColorEdit3("Clear color", clearColor);

    ImGui::Spacing();
    ImGui::Checkbox("Show ImGui demo window", &showDemoWindow);

    ImGui::End();

    if (showDemoWindow)
        ImGui::ShowDemoWindow(&showDemoWindow);
}

void UIPanel::OpenModelDialog()
{
    if (!hwnd)
        return;

    OPENFILENAMEW ofn = {};
    wchar_t fileBuffer[MAX_PATH] = L"";

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Wavefront OBJ (*.obj)\0*.obj\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn))
    {
        selectedModelPath = fileBuffer;
        if (importModelCallback)
            importModelCallback(selectedModelPath);
    }
}

void UIPanel::EndFrame()
{
    if (!initialized)
        return;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
