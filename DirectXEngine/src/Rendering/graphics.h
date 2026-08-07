#pragma once
#include <d3d11.h>
#include <wrl.h> // Microsoft::WRL::ComPtr
#include "camera.h"
#include "object3d.h"
#include "envcube.h"
#include "cube.h"
#include "plane.h"
#include "gizmoarrow.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <memory>
#include <string>
#include "raytracer.h"
#include "ui.h"
class Graphics
{
public:
    Graphics(HWND hwnd, int width, int height);
    ~Graphics() = default;

    void Clear(float r, float g, float b, float a);
    void RenderFrame();
	void Update(float time);
    void Resize(UINT width, UINT height);
    void ImportModel(const std::wstring& path);
    ID3D11Device* GetDevice() const { return device.Get(); }
    ID3D11DeviceContext* GetContext() const { return context.Get(); }
    void changeRenderMode();
	void swapChainPresent() { swapChain->Present(1, 0); }
    std::vector< std::shared_ptr<Object3D>> sceenObjects;
    EnvCube envcube;
    std::shared_ptr<Raytracer> raytracer;
    std::shared_ptr<GizmoArrow> gizmo;
	Camera camera;
private:
    RenderMode currentRenderMode;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
};
