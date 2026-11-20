#include "scalegizmo.h"

using namespace DirectX;

ScaleGizmo::ScaleGizmo(Microsoft::WRL::ComPtr<ID3D11Device> _device,
                       Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
    : Gizmo(_device, _context)
{
    Build("ScaleGizmo");
}

// Cube centred on the axis, spanning kShaftLength .. kShaftLength + kHeadLength.
void ScaleGizmo::appendHead(const AxisFrame& frame, const XMFLOAT3& color)
{
    using namespace GizmoGeometry;

    appendBox(frame, color, kHeadRadius,
              kShaftLength + kHeadLength * 0.5f, kHeadLength * 0.5f);
}

void ScaleGizmo::applyDrag(Object3D& target, const Drag& drag)
{
    // Dragging a handle away from the centre grows the object, inward shrinks it.
    float factor = 1.0f + drag.scalar;
    if (factor < 0.01f)
        factor = 0.01f;

    // The centre cube scales all three axes at once.
    const bool uniform = drag.axis == Axis::Center;
    const float sx = (uniform || drag.axis == Axis::X) ? factor : 1.0f;
    const float sy = (uniform || drag.axis == Axis::Y) ? factor : 1.0f;
    const float sz = (uniform || drag.axis == Axis::Z) ? factor : 1.0f;

    // The transform composes scale before translation, so this already scales
    // around the object centre.
    target.Scale(sx, sy, sz);
}
