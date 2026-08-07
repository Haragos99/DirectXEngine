#pragma once
#include "object3d.h"

// 3-axis translation gizmo: red = +X, green = +Y, blue = +Z.
// Not a scene object — owned separately by Graphics and drawn on top of the
// scene at a configurable target position (e.g. the selected object's origin).
class GizmoArrow : public Object3D
{
public:
    GizmoArrow() = default;
    GizmoArrow(Microsoft::WRL::ComPtr<ID3D11Device> _device,
               Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
    ~GizmoArrow() = default;

    void Update(float time) override;

    // Move the gizmo to a world-space point (typically the selected object).
    void SetTargetPosition(float x, float y, float z);

    // Uniform scale in world units so the gizmo stays a fixed visual size.
    void SetGizmoScale(float scale) { gizmoScale = scale; }

    // Draws all 3 arrows on top of the scene (depth test disabled).
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

    DirectX::XMFLOAT3 targetPosition{ 0.0f, 0.0f, 0.0f };
    float gizmoScale = 1.0f;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthAlwaysOn;
};
