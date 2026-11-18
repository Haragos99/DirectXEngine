#include "world.h"
#include <cfloat>
#include "renderer.h"

World::World(Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, float aspect)
    : camera(aspect), context(context)
{
    raytracer = std::make_shared<Raytracer>(device, context);
    envcube = EnvCube(device, context);
}

void World::Add(std::shared_ptr<Object3D> object)
{
    if (object)
        objects.push_back(std::move(object));
}

void World::Update(float time)
{
    raytracer->Update(time);
    for (auto& object : objects)
    {
        object->UpdateHierarchy(time);
    }
}

void World::Draw(Renderer& renderer)
{
    //raytracer->Draw(camera);

    // Background first: Gaussian splats blend without writing depth, so anything
    // drawn after them would cover them wherever the depth buffer is still empty.
    envcube.Draw(context, camera);

    // The env cube binds a depth state without depth writes; restore the scene
    // one so solid objects occlude each other instead of showing their interior.
    renderer.RestoreDepthState();

    for (auto& object : objects)
    {
        object->DrawHierarchy(camera, currentRenderMode);
    }
}

void World::Reparent(const std::shared_ptr<Object3D>& child, const std::shared_ptr<Object3D>& newParent)
{
    // Dropping an object on itself or on one of its own descendants would cut
    // the branch off the scene.
    if (!child || child == newParent || (newParent && newParent->IsDescendantOf(child.get())))
        return;

    std::shared_ptr<Object3D> detached;
    if (const std::shared_ptr<Object3D> currentParent = child->GetParent())
        detached = currentParent->DetachChild(child.get());
    else
        detached = detachRoot(child.get());

    if (!detached)
        return;

    if (newParent)
        newParent->AttachChild(detached);
    else
        objects.push_back(detached);
}

std::shared_ptr<Object3D> World::detachRoot(const Object3D* object)
{
    for (auto it = objects.begin(); it != objects.end(); ++it)
    {
        if (it->get() != object)
            continue;

        std::shared_ptr<Object3D> detached = *it;
        objects.erase(it);
        return detached;
    }
    return nullptr;
}

void World::RemoveObject(const std::shared_ptr<Object3D>& object)
{
    if (!object)
        return;

    if (const std::shared_ptr<Object3D> owner = object->GetParent())
        owner->DetachChild(object.get());
    else
        detachRoot(object.get());
}

namespace
{
    // Nearest hit among `object` and everything parented to it.
    void PickInHierarchy(const std::shared_ptr<Object3D>& object, const Ray& ray,
                         std::shared_ptr<Object3D>& nearest, float& nearestDistance)
    {
        if (!object)
            return;

        float distance = 0.0f;
        if (object->Intersect(ray, distance) && distance < nearestDistance)
        {
            nearestDistance = distance;
            nearest = object;
        }

        for (const std::shared_ptr<Object3D>& child : object->GetChildren())
            PickInHierarchy(child, ray, nearest, nearestDistance);
    }
}

std::shared_ptr<Object3D> World::PickObject(const Ray& ray, DirectX::XMFLOAT3* outHitPoint) const
{
    std::shared_ptr<Object3D> nearest;
    float nearestDistance = FLT_MAX;

    for (const std::shared_ptr<Object3D>& object : objects)
        PickInHierarchy(object, ray, nearest, nearestDistance);

    if (nearest && outHitPoint)
    {
        const DirectX::XMVECTOR origin = DirectX::XMLoadFloat3(&ray.origin);
        const DirectX::XMVECTOR direction = DirectX::XMLoadFloat3(&ray.direction);
        DirectX::XMStoreFloat3(outHitPoint, DirectX::XMVectorAdd(origin, DirectX::XMVectorScale(direction, nearestDistance)));
    }

    return nearest;
}

void World::CycleRenderMode()
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
