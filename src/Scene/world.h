#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include "camera.h"
#include "object3d.h"
#include "envcube.h"
#include "raytracer.h"

class Renderer;

// The scene: the top level objects (each with its children), the camera, the
// environment cube and the render mode. It owns no editor state (selection,
// gizmos, import), so anything that only needs to show a scene can use it.
class World
{
public:
    World(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, float aspect);

    void Add(std::shared_ptr<Object3D> object);
    // Removes `object` from its parent, or from the top level.
    void RemoveObject(const std::shared_ptr<Object3D>& object);
    // Move `child` under `newParent`, or back to the top level when it is null.
    void Reparent(const std::shared_ptr<Object3D>& child, const std::shared_ptr<Object3D>& newParent);
    // Ray-cast against every object and return the nearest one hit (nullptr when
    // nothing is hit). When provided, outHitPoint receives the world-space hit.
    std::shared_ptr<Object3D> PickObject(const Ray& ray, DirectX::XMFLOAT3* outHitPoint = nullptr) const;

    void Update(float time);
    // Draws the environment and every object. The renderer's frame must be bound.
    void Draw(Renderer& renderer);

    // Cycles Solid -> WireframeOnly -> SolidWireframe.
    void CycleRenderMode();
    RenderMode GetRenderMode() const { return currentRenderMode; }

    const std::vector<std::shared_ptr<Object3D>>& Objects() const { return objects; }

    Camera camera;

private:
    // Unlinks a top level object and hands its ownership over.
    std::shared_ptr<Object3D> detachRoot(const Object3D* object);

    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    std::vector<std::shared_ptr<Object3D>> objects;
    EnvCube envcube;
    std::shared_ptr<Raytracer> raytracer;
    RenderMode currentRenderMode = RenderMode::Solid;
};
