#pragma once
#include <windows.h>
#include <d3d11.h>

// Lightweight wrapper around Dear ImGui (Win32 + DX11 backends).
// Owns nothing renderer-side; borrows the device/context from Graphics.
class UI
{
public:
    UI() = default;
    ~UI();

    void Init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Shutdown();

    // Call once per frame BEFORE any ImGui::* calls.
    void BeginFrame();
    // Builds the demo/test control panel.
    void DrawTestPanel();
    // Renders ImGui draw data to the currently bound render target.
    // Call AFTER the scene has been drawn, BEFORE SwapChain->Present().
    void EndFrame();

    // Panel state (read from Graphics/Engine).
    bool  RequestedRenderModeChange() { bool v = renderModeChangeRequested; renderModeChangeRequested = false; return v; }
    const float* GetClearColor() const { return clearColor; }

private:
    bool  initialized = false;
    float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
    int   buttonClicks = 0;
    bool  showDemoWindow = false;
    bool  renderModeChangeRequested = false;
};
