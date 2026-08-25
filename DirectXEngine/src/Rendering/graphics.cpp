#include "graphics.h"
#include <stdexcept>
#include <cfloat>
#include "meshmodel.h"


Graphics::Graphics(HWND hwnd, int width, int height) : camera(static_cast<float>(width) / static_cast<float>(height))
{
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
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
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;;

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



    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS; // smaller depth = closer

    
    device->CreateDepthStencilState(&dsDesc, &depthStencilState);

    // Bind state
    context->OMSetDepthStencilState(depthStencilState.Get(), 1);

    // Bind render target + depth buffer
    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

    // Setup viewport
    D3D11_VIEWPORT viewport = {};
    viewport.Width =(float)(width);
    viewport.Height = (float)(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    context->RSSetViewports(1, &viewport);

    raytracer = std::make_shared<Raytracer>(device, context);

    for (int i = 0; i < 2; ++i)
    {
        sceenObjects.push_back(std::make_shared<Cube>(device, context));
        sceenObjects[i]->SetPosition((float)(i) * 6.0f - 4.0f, 0.0f, (float)(i) * 6.0f - 4.0f);
    }

    auto teapot = std::make_shared<MeshModel>("..\\Resources\\Models\\teapot.obj", L"shaders\\VertexShader.hlsl", L"shaders\\MeshPixelShader.hlsl", device, context);
    teapot->SetPosition(0.0f, -1.0f, 0.0f);
    sceenObjects.push_back(teapot);

    auto plane = std::make_shared<Plane>(device, context);
	sceenObjects.push_back(plane);
    envcube = EnvCube(device,context);

    // Editor-only transform gizmos (3 colored axis handles). Not part of
    // sceenObjects: they are control widgets drawn on top of the scene.
    gizmo = std::make_shared<GizmoController>(device, context);
    gizmo->SetGizmoScale(1.0f);

	currentRenderMode = RenderMode::Solid;
}

void Graphics::Resize(UINT width, UINT height)
{
    if (!swapChain || !device || !context)
        return;
    if (width == 0 || height == 0)
        return; // minimized

    // Detach back-buffer views before resizing the swap chain buffers.
    ID3D11RenderTargetView* nullRTV[] = { nullptr };
    context->OMSetRenderTargets(1, nullRTV, nullptr);
    renderTargetView.Reset();
    depthStencilView.Reset();

    HRESULT hr = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
        throw std::runtime_error("Failed to resize swap chain buffers");

    // Recreate render target view from the resized back buffer.
    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    if (FAILED(hr))
        throw std::runtime_error("Failed to get back buffer after resize");
    hr = device->CreateRenderTargetView(backBuffer.Get(), nullptr, &renderTargetView);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create render target view after resize");

    // Recreate depth/stencil buffer + view at the new size.
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
    hr = device->CreateTexture2D(&depthDesc, nullptr, &depthStencilBuffer);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create depth stencil buffer after resize");

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;
    hr = device->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, &depthStencilView);
    if (FAILED(hr))
        throw std::runtime_error("Failed to create depth stencil view after resize");

    // Update viewport to match the new size.
    D3D11_VIEWPORT viewport = {};
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    context->RSSetViewports(1, &viewport);

    // Rebind so subsequent draws use the fresh views.
    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());
    context->OMSetDepthStencilState(depthStencilState.Get(), 1);

    // Keep the camera's projection matching the new aspect ratio.
    camera.SetAspect(static_cast<float>(width) / static_cast<float>(height));
}

void Graphics::Clear(float r, float g, float b, float a)
{
    const float color[4] = { r, g, b, a };
    context->ClearRenderTargetView(renderTargetView.Get(), color);
    context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

}

void Graphics::Update(float time)
{
	raytracer->Update(time);
    for (auto& object : sceenObjects)
    {
        object->Update(time);
    }
}

void Graphics::ImportModel(const std::wstring& path)
{
    if (path.empty())
        return;

    std::string modelPath(path.begin(), path.end());
    auto importedModel = std::make_shared<MeshModel>(modelPath, L"shaders\\VertexShader.hlsl", L"shaders\\MeshPixelShader.hlsl", device, context);
    importedModel->SetPosition(0.0f, -1.0f, 0.0f);
    sceenObjects.push_back(importedModel);
}

void Graphics::SetSelectedObject(int index)
{
    if (!gizmo)
        return;
    if (index < 0 || index >= static_cast<int>(sceenObjects.size()))
    {
        gizmo->Detach();
        return;
    }
    gizmo->AttachTo(sceenObjects[index]);
}

void Graphics::SetGizmoMode(GizmoMode mode)
{
    if (gizmo)
        gizmo->SetMode(mode);
}

int Graphics::PickObject(const Ray& ray, DirectX::XMFLOAT3* outHitPoint) const
{
    int nearestIndex = -1;
    float nearestDistance = FLT_MAX;

    for (size_t i = 0; i < sceenObjects.size(); ++i)
    {
        if (!sceenObjects[i])
            continue;

        float distance = 0.0f;
        if (sceenObjects[i]->Intersect(ray, distance) && distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestIndex = static_cast<int>(i);
        }
    }

    if (nearestIndex >= 0 && outHitPoint)
    {
        const DirectX::XMVECTOR origin = DirectX::XMLoadFloat3(&ray.origin);
        const DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&ray.direction);
        DirectX::XMStoreFloat3(outHitPoint, DirectX::XMVectorAdd(origin, DirectX::XMVectorScale(direction, nearestDistance)));
    }

    return nearestIndex;
}

void Graphics::changeRenderMode()
{
    if (currentRenderMode == RenderMode::Solid)
    {
        currentRenderMode = RenderMode::WireframeOnly;
    }
    else if (currentRenderMode == RenderMode::WireframeOnly)
    {
        currentRenderMode = RenderMode::SolidWireframe;
    }
    else
    {
        currentRenderMode = RenderMode::Solid;
    }
}

void Graphics::RenderFrame()
{
    // Bind render target + depth buffer
    context->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

    // Bind state
    context->OMSetDepthStencilState(depthStencilState.Get(), 1);
    
	//raytracer->Draw(camera);

    for (auto& object : sceenObjects)
    {
       
        object->Draw(camera,currentRenderMode);
    }
    envcube.Draw(context, camera);

    // Draw the control gizmo last so it appears on top of the scene.
    if (gizmo && gizmo->IsVisible())
    {
        gizmo->Draw(camera, currentRenderMode);
    }
}

