#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include "gizmocontroller.h"
#include "gizmomode.h"
#include "isosurfacejob.h"

class World;
class IVoxelSource;
class Object3D;

// Editor actions on top of a World: the transform gizmo, the starting scene,
// model import, new skeletons and background marching-cubes meshing.
class Editor
{
public:
    Editor(World& world, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

    // Turns a finished meshing job into a scene object, then updates the world.
    void Update(float time);
    // Draws the gizmo on top of the scene. Call after World::Draw.
    void DrawOverlay();

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
    // Select which gizmo (translate or scale) the viewport manipulates with.
    void SetGizmoMode(GizmoMode mode);
    GizmoController* Gizmo() const { return gizmo.get(); }

private:
    // Two cubes, the teapot and the ground plane.
    void CreateDefaultScene();
    void collectIsoSurface();

    World& world;
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    // Editor-only transform gizmos (3 colored axis handles). Not part of the
    // world: they are control widgets drawn on top of the scene.
    std::shared_ptr<GizmoController> gizmo;
    std::unique_ptr<IsoSurfaceJob> meshingJob;
};
