#pragma once
#include <d3d11.h>
#include <wrl.h> // Microsoft::WRL::ComPtr
#include "camera.h"
#include "object3d.h"
#include "envcube.h"
#include "cube.h"
#include "plane.h"
#include "gizmocontroller.h"
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#include <memory>
#include <string>
#include "raytracer.h"
#include "ui.h"
#include "isosurfacejob.h"
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
    // Drop a new skeleton, a bare root joint, into the scene.
    void AddSkeleton();
    // Mesh `volume` with marching cubes and add the extracted surface to the
    // scene. `resolution` is the sample count along the volume's longest axis.
    // The work runs in the background; the mesh appears once it finishes.
    void AddIsoSurface(const std::shared_ptr<IVoxelSource>& volume, int resolution, float isoLevel);
    bool IsMeshing() const { return meshingJob != nullptr; }
    float GetMeshingProgress() const { return meshingJob ? meshingJob->GetProgress() : 0.0f; }
    std::string GetMeshingLabel() const { return meshingJob ? meshingJob->GetLabel() : std::string(); }
    // Attach the gizmo to an object, or hide it when the object is null.
    void SetSelectedObject(const std::shared_ptr<Object3D>& object);
    // Move `child` under `newParent`, or back to the scene root when it is null.
    void Reparent(const std::shared_ptr<Object3D>& child, const std::shared_ptr<Object3D>& newParent);
    // Select which gizmo (translate or scale) the viewport manipulates with.
    void SetGizmoMode(GizmoMode mode);
    // Ray-cast against every scene object and return the nearest one hit
    // (nullptr when nothing is hit). When provided, outHitPoint receives the
    // world-space position of the hit.
    std::shared_ptr<Object3D> PickObject(const Ray& ray, DirectX::XMFLOAT3* outHitPoint = nullptr) const;
    void RemoveObject(const std::shared_ptr<Object3D>& object);
    ID3D11Device* GetDevice() const { return device.Get(); }
    ID3D11DeviceContext* GetContext() const { return context.Get(); }
    void changeRenderMode();
	void swapChainPresent() { swapChain->Present(1, 0); }
    std::vector< std::shared_ptr<Object3D>> sceenObjects;
    EnvCube envcube;
    std::shared_ptr<Raytracer> raytracer;
    std::shared_ptr<GizmoController> gizmo;
	Camera camera;
private:
    // Unlinks a top level object from the scene and hands its ownership over.
    std::shared_ptr<Object3D> detachRoot(const Object3D* object);
    // Turns a finished meshing job into a scene object, on the thread that owns
    // the device context.
    void collectIsoSurface();
    std::unique_ptr<IsoSurfaceJob> meshingJob;
    RenderMode currentRenderMode;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;
};
