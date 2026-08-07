#pragma once
#include "object3d.h"

// 3-axis translation gizmo: red = +X, green = +Y, blue = +Z.
// Not a scene object: it is a control widget owned by Graphics and drawn on
// top of the scene at the position of an attached target Object3D.
// The gizmo is hidden unless a live target is attached.
class GizmoArrow : public Object3D
{
public:
    // Which axis handle a pick/drag refers to.
    enum class Axis { None, X, Y, Z };

    GizmoArrow() = default;
    GizmoArrow(Microsoft::WRL::ComPtr<ID3D11Device> _device,
               Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
    ~GizmoArrow() = default;

    void Update(float time) override;

    // Selection binding. The gizmo tracks the target's world position each
    // frame and hides itself when no live target is attached.
    void AttachTo(std::shared_ptr<Object3D> target);
    void Detach();
    bool IsVisible() const;

    // Uniform world-space size of the gizmo.
    void SetGizmoScale(float scale) { gizmoScale = scale; }

    // Return the axis arrow under the given world-space ray, or Axis::None.
    Axis PickAxis(const Ray& ray) const;

    // Axis-constrained translation drag. BeginDrag locks the axis under the
    // ray; UpdateDrag slides the attached target along that axis; EndDrag
    // releases it.
    void BeginDrag(Axis axis, const Ray& ray);
    void UpdateDrag(const Ray& ray);
    void EndDrag();
    bool IsDragging() const { return dragging; }

    void Draw(Camera camera, RenderMode mode) override;

protected:
    void createTexturedVertex() override;
    void createIndeces() override;

private:
    void appendArrow(const DirectX::XMFLOAT3& axis,
                     const DirectX::XMFLOAT3& color,
                     UINT segments,
                     float shaftLength, float shaftRadius,
                     float headLength,  float headRadius);

    // Unit direction vector for an axis.
    static DirectX::XMVECTOR AxisVector(Axis axis);
    // Parameter of the point on the axis line (anchored at `anchor`) closest to
    // the given ray, measured in world units along the axis direction.
    float AxisParamForRay(Axis axis, const DirectX::XMFLOAT3& anchor,
                          const Ray& ray) const;

    std::weak_ptr<Object3D> target;
    float gizmoScale = 1.0f;

    // Active drag state.
    bool dragging = false;
    Axis activeAxis = Axis::None;
    DirectX::XMFLOAT3 dragAnchor{}; // object position at drag start (axis origin)
    float lastParam = 0.0f;        // axis parameter applied on the previous update

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthAlwaysOn;
};
