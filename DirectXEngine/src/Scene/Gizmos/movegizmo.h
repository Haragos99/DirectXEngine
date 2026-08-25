#pragma once
#include "gizmo.h"

// Translation gizmo: each axis ends in a cone head and dragging it slides the
// attached object along that axis. Dragging the centre cube moves the object
// freely in the view plane.
class MoveGizmo : public Gizmo
{
public:
    MoveGizmo(Microsoft::WRL::ComPtr<ID3D11Device> _device,
              Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context);
    ~MoveGizmo() override = default;

protected:
    void appendHead(const AxisFrame& frame, const DirectX::XMFLOAT3& color) override;
    void applyDrag(Object3D& target, const Drag& drag) override;
};
