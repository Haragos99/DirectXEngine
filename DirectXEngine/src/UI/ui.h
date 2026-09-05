#pragma once
#include <windows.h>
#include <d3d11.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ipanelsection.h"
#include "uistate.h"
#include "Sections/marchingcubessection.h"
#include "Sections/sceneoutlinersection.h"
#include "Sections/splatmeshingsection.h"

class ImportSection;
class SkeletonSection;

// Owns the Dear ImGui lifecycle (Win32 + DX11 backends) and composes the control
// panel out of IPanelSections. It holds no renderer resources and draws no
// widgets itself: the sections do that, and UIState carries what they share.
class UIPanel
{
public:
    UIPanel();
    ~UIPanel();

    using ImportModelCallback = std::function<void(const std::wstring&)>;
    using CreateSkeletonCallback = std::function<void()>;

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
    void SetCreateSkeletonCallback(CreateSkeletonCallback callback);
    void SetCreateIsoSurfaceCallback(CreateIsoSurfaceCallback callback);
    void SetReparentCallback(SceneOutlinerSection::ReparentCallback callback);

    // Drives the meshing progress popup. Pushed in every frame by the engine.
    void SetMeshingStatus(bool active, float progress, const std::string& label);

    // Selection query. Returns nullptr when nothing is selected.
    std::shared_ptr<Object3D> GetSelectedObject() const { return state.SelectedObject(); }
    bool HasSelection() const { return state.HasSelection(); }
    // Set the selection (e.g. from viewport picking). Pass nullptr to clear.
    void SetSelectedObject(const std::shared_ptr<Object3D>& object) { state.Select(object); }

    // Transform mode chosen in the main panel (Move or Scale). It is always
    // available, independently of whether an object is currently selected.
    GizmoMode GetGizmoMode() const { return state.gizmoMode; }

private:
    void BuildSections();
    // Inspector for the selected object, shown next to the control panel.
    void DrawPropertiesPanel();
    // Modal shown while a mesh is being built in the background.
    void DrawMeshingProgress();

    bool initialized = false;
    HWND hwnd = nullptr;
    UIState state;
    std::vector<std::unique_ptr<IPanelSection>> sections;
    std::unique_ptr<IPanelSection> propertiesSection; // own window, not part of `sections`
    ImportSection* importSection = nullptr; // owned by `sections`
    SkeletonSection* skeletonSection = nullptr; // owned by `sections`
    MarchingCubesSection* marchingCubesSection = nullptr; // owned by `sections`
    SplatMeshingSection* splatMeshingSection = nullptr; // owned by `sections`
    SceneOutlinerSection* outlinerSection = nullptr; // owned by `sections`
};
