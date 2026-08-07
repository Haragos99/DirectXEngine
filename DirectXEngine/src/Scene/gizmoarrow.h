#pragma once
#include "object3d.h"

// 3-axis translation gizmo: red = +X, green = +Y, blue = +Z.
// Not a scene object: it is a control widget owned by Graphics and drawn on
// top of the scene at the position of an attached target Object3D.
// The gizmo is hidden unless a live target is attached.
class GizmoArrow : public Object3D
{
public:
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

    std::weak_ptr<Object3D> target;
    float gizmoScale = 1.0f;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthAlwaysOn;
};
