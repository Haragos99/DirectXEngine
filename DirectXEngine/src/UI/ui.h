#pragma once
#include <windows.h>
#include <d3d11.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "object3d.h"

// Lightweight wrapper around Dear ImGui (Win32 + DX11 backends).
// Owns nothing renderer-side; borrows the device/context from Graphics.
class UIPanel
{
public:
    UIPanel() = default;
    ~UIPanel();

    using ImportModelCallback = std::function<void(const std::wstring&)>;

    void Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Call once per frame BEFORE any ImGui::* calls.
    void BeginFrame();
    // Builds the demo/test control panel.
    void DrawTestPanel(const std::vector<std::shared_ptr<Object3D>>& sceneObjects);
    // Renders ImGui draw data to the currently bound render target.
    // Call AFTER the scene has been drawn, BEFORE SwapChain->Present().
    void EndFrame();

    // Panel state (read from Graphics/Engine).
    bool  RequestedRenderModeChange() { bool v = renderModeChangeRequested; renderModeChangeRequested = false; return v; }
    const float* GetClearColor() const { return clearColor; }
    void SetImportModelCallback(ImportModelCallback callback) { importModelCallback = std::move(callback); }

    // Selection query. Returns -1 when no scene object is selected.
    int  GetSelectedIndex() const { return selectedSceneObjectIndex; }
    bool HasSelection() const { return selectedSceneObjectIndex >= 0; }
    // Set the selected object (e.g. from viewport picking). Pass -1 to clear.
    void SetSelectedIndex(int index) { selectedSceneObjectIndex = index; }

    // Gizmo interaction mode chosen in the panel (Move or Scale).
    GizmoMode GetGizmoMode() const { return gizmoMode; }

private:
    void OpenModelDialog();

    bool  initialized = false;
    HWND hwnd = nullptr;
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    int   buttonClicks = 0;
    bool  showDemoWindow = false;
    bool  renderModeChangeRequested = false;
    int   selectedSceneObjectIndex = -1;
    GizmoMode gizmoMode = GizmoMode::Move;
    std::wstring selectedModelPath;
    ImportModelCallback importModelCallback;
};
