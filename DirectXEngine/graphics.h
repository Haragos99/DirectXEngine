#pragma once
#include <d3d11.h>
#include <wrl.h> // Microsoft::WRL::ComPtr
#include "camera.h"
#include "object3d.h"
#include "envcube.h"
#include "cube.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <memory>
class Graphics
{
public:
    Graphics(HWND hwnd, int width, int height);
    ~Graphics() = default;

    void Clear(float r, float g, float b, float a);
    void RenderFrame();

    ID3D11Device* GetDevice() const { return device.Get(); }
    ID3D11DeviceContext* GetContext() const { return context.Get(); }
    void changeWireFrame() { cube->wireframeEnabled = !cube->wireframeEnabled; }
    std::shared_ptr<Object3D> cube;
    std::shared_ptr<Object3D> plane;
    EnvCube envcube;
	Camera camera;
private:
    
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
};
