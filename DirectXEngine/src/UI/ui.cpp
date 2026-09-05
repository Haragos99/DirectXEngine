#include "ui.h"

#include "Sections/importsection.h"
#include "Sections/marchingcubessection.h"
#include "Sections/objectpropertiessection.h"
#include "Sections/sceneoutlinersection.h"
#include "Sections/skeletonsection.h"
#include "Sections/splatsettingssection.h"
#include "Sections/splatmeshingsection.h"
#include "Sections/transformmodesection.h"
#include "Sections/viewportsection.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

UIPanel::UIPanel()
{
    BuildSections();
}

UIPanel::~UIPanel()
{
    Shutdown();
}

void UIPanel::BuildSections()
{
    auto import = std::make_unique<ImportSection>();
    importSection = import.get();

    auto skeleton = std::make_unique<SkeletonSection>();
    skeletonSection = skeleton.get();

    auto outliner = std::make_unique<SceneOutlinerSection>();
    outlinerSection = outliner.get();

    auto marchingCubes = std::make_unique<MarchingCubesSection>();
    marchingCubesSection = marchingCubes.get();

    auto splatMeshing = std::make_unique<SplatMeshingSection>();
    splatMeshingSection = splatMeshing.get();

    sections.push_back(std::move(import));
    sections.push_back(std::make_unique<TransformModeSection>());
    sections.push_back(std::move(outliner));
    sections.push_back(std::move(skeleton));
    sections.push_back(std::move(marchingCubes));
    sections.push_back(std::make_unique<SplatSettingsSection>());
    sections.push_back(std::move(splatMeshing));
    sections.push_back(std::make_unique<ViewportSection>());

    propertiesSection = std::make_unique<ObjectPropertiesSection>();
}

void UIPanel::SetImportModelCallback(ImportModelCallback callback)
{
    importSection->SetCallback(std::move(callback));
}

void UIPanel::SetCreateSkeletonCallback(CreateSkeletonCallback callback)
{
    skeletonSection->SetCreateCallback(std::move(callback));
}

void UIPanel::SetCreateIsoSurfaceCallback(CreateIsoSurfaceCallback callback)
{
    marchingCubesSection->SetCreateCallback(callback);
    splatMeshingSection->SetCreateCallback(std::move(callback));
}

void UIPanel::SetReparentCallback(SceneOutlinerSection::ReparentCallback callback)
{
    outlinerSection->SetReparentCallback(std::move(callback));
}

void UIPanel::SetMeshingStatus(bool active, float progress, const std::string& label)
{
    state.meshingActive = active;
    state.meshingProgress = progress;
    state.meshingLabel = label;
}

void UIPanel::Init(HWND _hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    if (initialized)
        return;

    hwnd = _hwnd;
    importSection->SetOwnerWindow(hwnd);

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

void UIPanel::DrawTestPanel(const std::vector<std::shared_ptr<Object3D>>& sceneObjects)
{
    if (!initialized)
        return;

    // Borrowed for this frame only, so sections never outlive the scene list.
    state.sceneObjects = &sceneObjects;

    ImGui::Begin("Control Panel");

    for (const std::unique_ptr<IPanelSection>& section : sections)
    {
        if (!section->IsVisible(state))
            continue;

        ImGui::SeparatorText(section->GetTitle());
        section->Draw(state);
        ImGui::Spacing();
    }

    ImGui::End();

    DrawPropertiesPanel();
    DrawMeshingProgress();

    state.sceneObjects = nullptr;
}

void UIPanel::DrawPropertiesPanel()
{
    if (!propertiesSection->IsVisible(state))
        return;

    ImGui::Begin(propertiesSection->GetTitle());
    propertiesSection->Draw(state);
    ImGui::End();
}

void UIPanel::DrawMeshingProgress()
{
    static constexpr const char* kPopupId = "Building mesh";

    if (state.meshingActive && !ImGui::IsPopupOpen(kPopupId))
        ImGui::OpenPopup(kPopupId);

    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal(kPopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        return;

    ImGui::TextUnformatted(state.meshingLabel.c_str());
    ImGui::ProgressBar(state.meshingProgress, ImVec2(280.0f, 0.0f));

    // The job is collected by the renderer, so the popup just waits it out.
    if (!state.meshingActive)
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void UIPanel::EndFrame()
{
    if (!initialized)
        return;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
