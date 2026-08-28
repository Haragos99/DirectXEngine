#include "movegizmo.h"

using namespace DirectX;

MoveGizmo::MoveGizmo(Microsoft::WRL::ComPtr<ID3D11Device> _device,
                     Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
    : Gizmo(_device, _context)
{
    Build("MoveGizmo");
}

// Cone from kShaftLength to kShaftLength + kHeadLength, closed with a base cap.
void MoveGizmo::appendHead(const AxisFrame& frame, const XMFLOAT3& color)
{
    using namespace GizmoGeometry;

    const UINT baseRingStart = static_cast<UINT>(vertices.size());
    for (UINT i = 0; i < kSegments; ++i)
    {
        const float theta = XM_2PI * static_cast<float>(i) / static_cast<float>(kSegments);
        appendVertex(framePoint(frame, kHeadRadius, kShaftLength, theta),
                     color, XMFLOAT2(0.0f, 0.0f));
    }

    const UINT apexIndex = static_cast<UINT>(vertices.size());
    appendVertex(framePoint(frame, 0.0f, kShaftLength + kHeadLength, 0.0f),
                 color, XMFLOAT2(0.5f, 1.0f));

    for (UINT i = 0; i < kSegments; ++i)
    {
        const UINT i0 = baseRingStart + i;
        const UINT i1 = baseRingStart + (i + 1) % kSegments;
        indices.push_back(i0);
        indices.push_back(apexIndex);
        indices.push_back(i1);
    }

    const UINT capCenterIndex = static_cast<UINT>(vertices.size());
    appendVertex(framePoint(frame, 0.0f, kShaftLength, 0.0f),
                 color, XMFLOAT2(0.5f, 0.5f));

    for (UINT i = 0; i < kSegments; ++i)
    {
        const UINT i0 = baseRingStart + i;
        const UINT i1 = baseRingStart + (i + 1) % kSegments;
        indices.push_back(capCenterIndex);
        indices.push_back(i1);
        indices.push_back(i0);
    }
}

void MoveGizmo::applyDrag(Object3D& target, const Drag& drag)
{
    // The base class already constrained the movement to the locked axis, or
    // left it free for the centre handle.
    target.AdjustPosition(drag.translation.x, drag.translation.y, drag.translation.z);
}
