#include "editor.h"
#include "world.h"
#include "cube.h"
#include "plane.h"
#include "meshmodel.h"
#include "modelimportservice.h"
#include "skeleton.h"
#include "isosurfacemodel.h"
#include "ivoxelsource.h"
#include "assetpaths.h"

Editor::Editor(World& world, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
    : world(world), device(device), context(context)
{
    CreateDefaultScene();

    gizmo = std::make_shared<GizmoController>(device, context);
    gizmo->SetGizmoScale(1.0f);
}

void Editor::CreateDefaultScene()
{
    for (int i = 0; i < 2; ++i)
    {
        auto cube = std::make_shared<Cube>(device, context);
        cube->SetPosition((float)(i) * 6.0f - 4.0f, 0.0f, (float)(i) * 6.0f - 4.0f);
        world.Add(cube);
    }

    auto teapot = std::make_shared<MeshModel>(AssetPaths::ResourceA(L"Models\\teapot.obj"), L"shaders\\VertexShader.hlsl", L"shaders\\MeshPixelShader.hlsl", device, context);
    teapot->SetPosition(0.0f, -1.0f, 0.0f);
    world.Add(teapot);

    world.Add(std::make_shared<Plane>(device, context));
}

void Editor::Update(float time)
{
    collectIsoSurface();
    world.Update(time);
}

void Editor::DrawOverlay()
{
    // Draw the control gizmo last so it appears on top of the scene.
    if (gizmo && gizmo->IsVisible())
    {
        gizmo->Draw(world.camera, world.GetRenderMode());
    }
}

void Editor::ImportModel(const std::wstring& path)
{
    // The service decides which loader runs and which scene object comes back,
    // so a new file format never touches this code.
    if (auto imported = ModelImportService::Instance().Import(path, device, context))
        world.Add(imported);
}

void Editor::AddSkeleton()
{
    world.Add(std::make_shared<Skeleton>(device, context));
}

void Editor::AddIsoSurface(const std::shared_ptr<IVoxelSource>& volume, int resolution, float isoLevel)
{
    // One job at a time; the panel disables its buttons while one is running.
    if (!volume || meshingJob)
        return;

    meshingJob = std::make_unique<IsoSurfaceJob>(volume, resolution, isoLevel);
}

// Runs on the thread that owns the device context, so the GPU buffers can be created here.
void Editor::collectIsoSurface()
{
    if (!meshingJob || !meshingJob->IsFinished())
        return;

    MeshData surface = meshingJob->TakeMesh();
    if (!surface.indices.empty())
    {
        auto mesh = std::make_shared<IsoSurfaceModel>(
            std::move(surface),
            meshingJob->GetMeshName(),
            L"shaders\\VertexShader.hlsl",
            L"shaders\\MeshPixelShader.hlsl",
            device, context);
        mesh->SetTransform(meshingJob->GetPlacement());
        world.Add(std::move(mesh));
    }

    meshingJob.reset();
}

void Editor::SetSelectedObject(const std::shared_ptr<Object3D>& object)
{
    if (!gizmo)
        return;

    if (object)
        gizmo->AttachTo(object);
    else
        gizmo->Detach();
}

void Editor::SetGizmoMode(GizmoMode mode)
{
    if (gizmo)
        gizmo->SetMode(mode);
}
