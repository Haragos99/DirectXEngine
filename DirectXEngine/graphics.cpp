#include "Graphics.h"
#include <stdexcept>

Graphics::Graphics(HWND hwnd, int width, int height) : camera(static_cast<float>(width) / static_cast<float>(height))
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;  // no MSAA for now
    scd.SampleDesc.Quality = 0;
    scd.Windowed = TRUE;

    UINT createDeviceFlags = 0;
#if defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &scd,
        &swapChain,
        &device,
        &featureLevel,
        &context
    );

    if (FAILED(hr))
        throw std::runtime_error("Failed to create device and swap chain");

    // Get back buffer
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to get back buffer");

    // Create render target view
    hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create render target view");

    // Create depth/stencil buffer
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthDesc.CPUAccessFlags = 0;
    depthDesc.MiscFlags = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
    hr = device->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create depth stencil buffer");

    // Create depth/stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = device->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, &depthStencilView);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create depth stencil view");

    // Bind render target + depth buffer
    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

    // Setup viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    context->RSSetViewports(1, &viewport);
	cube = Object3D(device);
}

void Graphics::Clear(float r, float g, float b, float a)
{
    const float color[4] = { r, g, b, a };
    context->ClearRenderTargetView(renderTargetView.Get(), color);
    context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

}

void Graphics::RenderFrame()
{
	cube.Draw(context, camera);
    swapChain->Present(1, 0); // vsync on
}