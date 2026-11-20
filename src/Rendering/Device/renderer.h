#pragma once
#include <d3d11.h>
#include <wrl/client.h>

// Owns the D3D11 device, the swap chain and the back-buffer render targets.
// It knows nothing about the scene: callers bind the frame, draw, then Present.
class Renderer
{
public:
    Renderer(HWND hwnd, int width, int height);
    ~Renderer() = default;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void Clear(float r, float g, float b, float a);
    // Binds the back buffer, the depth buffer and the default depth state.
    void BeginFrame();
    // Re-binds the default depth state, for after a pass that changed it.
    void RestoreDepthState();
    void Present();
    // Recreates the back-buffer views and the viewport. Ignores a 0 size (minimized).
    void Resize(UINT width, UINT height);

    const Microsoft::WRL::ComPtr<ID3D11Device>& Device() const { return device; }
    const Microsoft::WRL::ComPtr<ID3D11DeviceContext>& Context() const { return context; }
    ID3D11Device* GetDevice() const { return device.Get(); }
    ID3D11DeviceContext* GetContext() const { return context.Get(); }

private:
    // Creates the render target view, depth buffer + view and viewport for the
    // current swap chain buffers. Used at startup and after every resize.
    void CreateBackBufferViews(UINT width, UINT height);

    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
};
