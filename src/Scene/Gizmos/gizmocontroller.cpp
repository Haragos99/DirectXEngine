#include "gizmocontroller.h"
#include "movegizmo.h"
#include "scalegizmo.h"

GizmoController::GizmoController(Microsoft::WRL::ComPtr<ID3D11Device> device,
                                 Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
	: moveGizmo(std::make_shared<MoveGizmo>(device, context)),
	  scaleGizmo(std::make_shared<ScaleGizmo>(device, context))
{
}

Gizmo& GizmoController::Active() const
{
	return mode == GizmoMode::Scale ? *scaleGizmo : *moveGizmo;
}

void GizmoController::SetMode(GizmoMode newMode)
{
	if (newMode == mode)
		return;

	// Never carry a half-finished drag across a mode switch.
	Active().EndDrag();
	mode = newMode;
}

void GizmoController::AttachTo(std::shared_ptr<Object3D> target)
{
	// Both gizmos track the same target so switching mode needs no rebinding.
	moveGizmo->AttachTo(target);
	scaleGizmo->AttachTo(target);
}

void GizmoController::Detach()
{
	moveGizmo->Detach();
	scaleGizmo->Detach();
}

bool GizmoController::IsVisible() const
{
	return Active().IsVisible();
}

void GizmoController::SetGizmoScale(float scale)
{
	moveGizmo->SetGizmoScale(scale);
	scaleGizmo->SetGizmoScale(scale);
}

GizmoController::Axis GizmoController::PickAxis(const Ray& ray) const
{
	return Active().PickAxis(ray);
}

void GizmoController::BeginDrag(Axis axis, const Ray& ray)
{
	Active().BeginDrag(axis, ray);
}

void GizmoController::UpdateDrag(const Ray& ray)
{
	Active().UpdateDrag(ray);
}

void GizmoController::EndDrag()
{
	Active().EndDrag();
}

bool GizmoController::IsDragging() const
{
	return Active().IsDragging();
}

void GizmoController::Draw(Camera camera, RenderMode renderMode)
{
	Active().Draw(camera, renderMode);
}
