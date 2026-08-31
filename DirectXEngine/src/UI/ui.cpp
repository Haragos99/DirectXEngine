#include "ui.h"

#include "Sections/importsection.h"
#include "Sections/objectpropertiessection.h"
#include "Sections/sceneoutlinersection.h"
#include "Sections/skeletonsection.h"
#include "Sections/splatsettingssection.h"
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

    sections.push_back(std::move(import));
    sections.push_back(std::make_unique<TransformModeSection>());
    sections.push_back(std::move(outliner));
    sections.push_back(std::move(skeleton));
    sections.push_back(std::make_unique<SplatSettingsSection>());
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

void UIPanel::SetReparentCallback(SceneOutlinerSection::ReparentCallback callback)
{
    outlinerSection->SetReparentCallback(std::move(callback));
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

void UIPanel::EndFrame()
{
    if (!initialized)
        return;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
