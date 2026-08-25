#pragma once
#include "gizmo.h"

// Scale gizmo: each axis ends in a cube handle (instead of the move gizmo's
// cone) and dragging it scales the attached object along that axis. Dragging
// the centre cube scales uniformly on all three axes.
class ScaleGizmo : public Gizmo
{
public:
    ScaleGizmo(Microsoft::WRL::ComPtr<ID3D11Device> _device,
               Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
    ~ScaleGizmo() override = default;

protected:
    void appendHead(const AxisFrame& frame, const DirectX::XMFLOAT3& color) override;
    void applyDrag(Object3D& target, const Drag& drag) override;
};
