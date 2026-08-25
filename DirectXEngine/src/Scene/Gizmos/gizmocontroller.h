#pragma once
#include "gizmo.h"
#include "gizmomode.h"
#include <memory>

// Owns one gizmo per transform mode and forwards attachment, picking, dragging
// and drawing to whichever one the current mode selects. Callers never need to
// know which concrete gizmo is active, so adding a mode (e.g. Rotate) only means
// adding a Gizmo subclass and registering it here.
class GizmoController
{
public:
	using Axis = Gizmo::Axis;

	GizmoController(Microsoft::WRL::ComPtr<ID3D11Device> device,
	                Microsoft::WRL::ComPtr<ID3D11DeviceContext> context);

	void SetMode(GizmoMode newMode);
	GizmoMode GetMode() const { return mode; }

	void AttachTo(std::shared_ptr<Object3D> target);
	void Detach();
	bool IsVisible() const;
	void SetGizmoScale(float scale);

	Axis PickAxis(const Ray& ray) const;
	void BeginDrag(Axis axis, const Ray& ray);
	void UpdateDrag(const Ray& ray);
	void EndDrag();
	bool IsDragging() const;

	void Draw(Camera camera, RenderMode renderMode);

private:
	Gizmo& Active() const;

	std::shared_ptr<Gizmo> moveGizmo;
	std::shared_ptr<Gizmo> scaleGizmo;
	GizmoMode mode = GizmoMode::Move;
};
