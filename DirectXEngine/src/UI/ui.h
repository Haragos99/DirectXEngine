#pragma once
#include <windows.h>
#include <d3d11.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ipanelsection.h"
#include "uistate.h"

class ImportSection;

// Owns the Dear ImGui lifecycle (Win32 + DX11 backends) and composes the control
// panel out of IPanelSections. It holds no renderer resources and draws no
// widgets itself: the sections do that, and UIState carries what they share.
class UIPanel
{
public:
    UIPanel();
    ~UIPanel();

    using ImportModelCallback = std::function<void(const std::wstring&)>;

    void Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Call once per frame BEFORE any ImGui::* calls.
    void BeginFrame();
    // Draws every visible section of the control panel.
    void DrawTestPanel(const std::vector<std::shared_ptr<Object3D>>& sceneObjects);
    // Renders ImGui draw data to the currently bound render target.
    // Call AFTER the scene has been drawn, BEFORE SwapChain->Present().
    void EndFrame();

    // Panel state (read from Graphics/Engine).
    bool  RequestedRenderModeChange() { bool v = state.renderModeChangeRequested; state.renderModeChangeRequested = false; return v; }
    const float* GetClearColor() const { return state.clearColor; }
    void SetImportModelCallback(ImportModelCallback callback);

    // Selection query. Returns -1 when no scene object is selected.
    int  GetSelectedIndex() const { return state.selectedIndex; }
    bool HasSelection() const { return state.selectedIndex >= 0; }
    // Set the selected object (e.g. from viewport picking). Pass -1 to clear.
    void SetSelectedIndex(int index) { state.selectedIndex = index; }

    // Transform mode chosen in the main panel (Move or Scale). It is always
    // available, independently of whether an object is currently selected.
    GizmoMode GetGizmoMode() const { return state.gizmoMode; }

private:
    void BuildSections();
    // Inspector for the selected object, shown next to the control panel.
    void DrawPropertiesPanel();

    bool initialized = false;
    HWND hwnd = nullptr;
    UIState state;
    std::vector<std::unique_ptr<IPanelSection>> sections;
    std::unique_ptr<IPanelSection> propertiesSection; // own window, not part of `sections`
    ImportSection* importSection = nullptr; // owned by `sections`
};
