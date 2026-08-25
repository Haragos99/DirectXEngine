#include "gizmo.h"
#include <cfloat>

using namespace DirectX;

Gizmo::Gizmo(Microsoft::WRL::ComPtr<ID3D11Device> _device,
             Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
    : Object3D(_device, _context)
{
    wireframeEnabled = false;
}

void Gizmo::Build(const std::string& gizmoName)
{
    // Build merged mesh: X (red), Y (green), Z (blue).
    createTexturedVertex();
    createIndeces();

    // Shader helper still bind one, so use a neutral 1x1 white texture.
    texture->CreateSolidColorTexture(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

    shader->createVertexBuffer(vertices);
    shader->createInexxBuffer(indices);
    shader->createConstantBuffer();
    shader->creaetLightBuffer();
    shader->createRasterize();
    shader->LoadShaders(L"shaders\\GizmoVertexShader.hlsl",
                        L"shaders\\GizmoPixelShader.hlsl");

    name = gizmoName;

    // Depth-stencil state that lets the gizmo draw on top of the scene.
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable    = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
    device->CreateDepthStencilState(&dsDesc, &depthAlwaysOn);
}

void Gizmo::Update(float /*time*/)
{
    // Gizmo does not animate on its own; its world matrix is composed in Draw.
}

void Gizmo::AttachTo(std::shared_ptr<Object3D> newTarget)
{
    target = newTarget;
}

void Gizmo::Detach()
{
    target.reset();
}

bool Gizmo::IsVisible() const
{
    return !target.expired();
}


// Axis picking and dragging
XMVECTOR Gizmo::AxisVector(Axis axis)
{
    switch (axis)
    {
    case Axis::X: return XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    case Axis::Y: return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    case Axis::Z: return XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    default:      return XMVectorZero();
    }
}

float Gizmo::AxisParamForRay(Axis axis, const XMFLOAT3& anchor,
                             const Ray& ray) const
{
    // Closest point between the axis line (O + s*A) and the pick ray
    // (R + u*D). Returns s.
    const XMVECTOR A = AxisVector(axis);
    const XMVECTOR O = XMLoadFloat3(&anchor);
    const XMVECTOR R = XMLoadFloat3(&ray.origin);
    const XMVECTOR D = XMVector3Normalize(XMLoadFloat3(&ray.direction));

    const XMVECTOR w0 = XMVectorSubtract(O, R);
    const float b = XMVectorGetX(XMVector3Dot(A, D));
    const float d = XMVectorGetX(XMVector3Dot(A, w0));
    const float e = XMVectorGetX(XMVector3Dot(D, w0));
    const float denom = 1.0f - b * b;
    if (fabsf(denom) < 1e-6f)
    {
        return 0.0f; // ray parallel to axis
    }
    return (b * e - d) / denom;
}

Gizmo::Axis Gizmo::PickAxis(const Ray& ray) const
{
    auto liveTarget = target.lock();
    if (!liveTarget)
        return Axis::None;

    // The centre cube sits where all three shafts start, so it wins the pick.
    if (PickCentre(ray))
        return Axis::Center;

    const XMFLOAT3 posF = liveTarget->GetPosition();
    const XMVECTOR O = XMLoadFloat3(&posF);
    const XMVECTOR R = XMLoadFloat3(&ray.origin);
    const XMVECTOR D = XMVector3Normalize(XMLoadFloat3(&ray.direction));

    const float length    = GizmoGeometry::kHandleLength * gizmoScale;
    const float threshold = 0.12f * gizmoScale; // grab radius around the shaft

    Axis  best      = Axis::None;
    float bestRayT  = FLT_MAX;

    const Axis axes[3] = { Axis::X, Axis::Y, Axis::Z };
    for (Axis axis : axes)
    {
        const XMVECTOR A = AxisVector(axis);

        // Point on the axis segment closest to the ray, clamped to [0, length].
        float s = AxisParamForRay(axis, posF, ray);
        if (s < 0.0f)     s = 0.0f;
        if (s > length)   s = length;

        const XMVECTOR axisPoint = XMVectorAdd(O, XMVectorScale(A, s));

        // Parameter along the ray for that point (must be in front of camera).
        const float u = XMVectorGetX(XMVector3Dot(XMVectorSubtract(axisPoint, R), D));
        if (u < 0.0f)
            continue;

        const XMVECTOR rayPoint = XMVectorAdd(R, XMVectorScale(D, u));
        const float dist = XMVectorGetX(XMVector3Length(XMVectorSubtract(axisPoint, rayPoint)));

        if (dist <= threshold && u < bestRayT)
        {
            bestRayT = u;
            best     = axis;
        }
    }
    return best;
}

void Gizmo::BeginDrag(Axis axis, const Ray& ray)
{
    auto liveTarget = target.lock();
    if (!liveTarget || axis == Axis::None)
        return;

    dragging   = true;
    activeAxis = axis;
    dragAnchor = liveTarget->GetPosition();

    if (axis == Axis::Center)
    {
        // Drag inside the plane facing the viewer: the pick ray direction is a
        // good stand-in for the view direction.
        XMStoreFloat3(&dragPlaneNormal,
            XMVectorNegate(XMVector3Normalize(XMLoadFloat3(&ray.direction))));
        lastPlanePoint = PlaneHitForRay(ray);
        lastRadius = XMVectorGetX(XMVector3Length(
            XMVectorSubtract(XMLoadFloat3(&lastPlanePoint), XMLoadFloat3(&dragAnchor))));
        return;
    }

    lastParam = AxisParamForRay(axis, dragAnchor, ray);
}

void Gizmo::UpdateDrag(const Ray& ray)
{
    auto liveTarget = target.lock();
    if (!dragging || !liveTarget)
        return;

    Drag drag;
    drag.axis = activeAxis;

    if (activeAxis == Axis::Center)
    {
        const XMFLOAT3 hit = PlaneHitForRay(ray);
        const XMVECTOR hitVec = XMLoadFloat3(&hit);
        XMStoreFloat3(&drag.translation, XMVectorSubtract(hitVec, XMLoadFloat3(&lastPlanePoint)));

        // Radial movement away from the centre drives uniform scaling.
        const float radius = XMVectorGetX(XMVector3Length(
            XMVectorSubtract(hitVec, XMLoadFloat3(&dragAnchor))));
        drag.scalar = radius - lastRadius;

        lastPlanePoint = hit;
        lastRadius = radius;
    }
    else
    {
        const float param = AxisParamForRay(activeAxis, dragAnchor, ray);
        drag.scalar = param - lastParam;
        lastParam = param;
        XMStoreFloat3(&drag.translation, XMVectorScale(AxisVector(activeAxis), drag.scalar));
    }

    applyDrag(*liveTarget, drag);
}

bool Gizmo::PickCentre(const Ray& ray) const
{
    auto liveTarget = target.lock();
    if (!liveTarget)
        return false;

    const XMFLOAT3 posF = liveTarget->GetPosition();
    const XMVECTOR Postion = XMLoadFloat3(&posF);
    const XMVECTOR RayOrigin = XMLoadFloat3(&ray.origin);
    const XMVECTOR Distance = XMVector3Normalize(XMLoadFloat3(&ray.direction));

    const float u = XMVectorGetX(XMVector3Dot(XMVectorSubtract(Postion, RayOrigin), Distance));
    if (u < 0.0f)
    {
        return false; // behind the camera
    }

    const XMVECTOR closest = XMVectorAdd(RayOrigin, XMVectorScale(Distance, u));
    const float distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(Postion, closest)));
    return distance <= GizmoGeometry::kCentrePickRadius * gizmoScale;
}

XMFLOAT3 Gizmo::PlaneHitForRay(const Ray& ray) const
{
    const XMVECTOR Normal = XMLoadFloat3(&dragPlaneNormal);
    const XMVECTOR RayOrigin = XMLoadFloat3(&ray.origin);
    const XMVECTOR Distance = XMVector3Normalize(XMLoadFloat3(&ray.direction));

    const float denom = XMVectorGetX(XMVector3Dot(Normal, Distance));
    if (fabsf(denom) < 1e-6f)
        return lastPlanePoint; // ray parallel to the drag plane

    const XMVECTOR anchor = XMLoadFloat3(&dragAnchor);
    const float t = XMVectorGetX(XMVector3Dot(Normal, XMVectorSubtract(anchor, RayOrigin))) / denom;

    XMFLOAT3 hit;
    XMStoreFloat3(&hit, XMVectorAdd(RayOrigin, XMVectorScale(Distance, t)));
    return hit;
}

void Gizmo::EndDrag()
{
    dragging   = false;
    activeAxis = Axis::None;
}

// Geometry generation
void Gizmo::createTexturedVertex()
{
    vertices.clear();
    indices.clear();

    appendCentreHandle();
    appendAxis(XMFLOAT3(1, 0, 0), XMFLOAT3(1, 0, 0)); // X - red
    appendAxis(XMFLOAT3(0, 1, 0), XMFLOAT3(0, 1, 0)); // Y - green
    appendAxis(XMFLOAT3(0, 0, 1), XMFLOAT3(0, 0, 1)); // Z - blue
}

void Gizmo::createIndeces()
{
    // Indices are appended alongside vertices while building the axes.
    // Keep this a no-op so we don't clear what we just built.
}

Gizmo::AxisFrame Gizmo::makeAxisFrame(const XMFLOAT3& axis)
{
    AxisFrame frame;
    frame.w = XMVector3Normalize(XMLoadFloat3(&axis));
    const XMVECTOR up = (fabsf(axis.y) < 0.9f)
        ? XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        : XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    frame.u = XMVector3Normalize(XMVector3Cross(up, frame.w));
    frame.v = XMVector3Cross(frame.w, frame.u);
    return frame;
}

XMFLOAT3 Gizmo::framePoint(const AxisFrame& frame, float radius,
                           float axialOffset, float theta)
{
    const XMVECTOR p = XMVectorScale(frame.u, radius * cosf(theta))
                     + XMVectorScale(frame.v, radius * sinf(theta))
                     + XMVectorScale(frame.w, axialOffset);
    XMFLOAT3 out;
    XMStoreFloat3(&out, p);
    return out;
}

void Gizmo::appendVertex(const XMFLOAT3& position, const XMFLOAT3& color,
                         const XMFLOAT2& uv)
{
    // The gizmo shaders read the per-vertex colour out of the normal slot.
    VertexData vertex;
    vertex.position = position;
    vertex.normal   = color;
    vertex.texcoord = uv;
    vertices.push_back(vertex);
}

// Build one axis handle: the shared shaft plus the subclass-specific head.
void Gizmo::appendAxis(const XMFLOAT3& axis, const XMFLOAT3& color)
{
    const AxisFrame frame = makeAxisFrame(axis);
    appendShaft(frame, color);
    appendHead(frame, color);
}

// Cube at the gizmo origin. Dragging it moves or scales on all axes at once.
void Gizmo::appendCentreHandle()
{
    const AxisFrame frame = makeAxisFrame(XMFLOAT3(0, 0, 1));
    appendBox(frame, XMFLOAT3(0.85f, 0.85f, 0.85f),
              GizmoGeometry::kCentreHalfExtent, 0.0f, GizmoGeometry::kCentreHalfExtent);
}

void Gizmo::appendBox(const AxisFrame& frame, const XMFLOAT3& color,
                      float halfExtent, float centre, float halfLength)
{
    // Corners: (-u,-v,-w) (+u,-v,-w) (+u,+v,-w) (-u,+v,-w), then the same at +w.
    static constexpr float signsU[8] = { -1, 1, 1, -1, -1, 1, 1, -1 };
    static constexpr float signsV[8] = { -1, -1, 1, 1, -1, -1, 1, 1 };
    static constexpr float signsW[8] = { -1, -1, -1, -1, 1, 1, 1, 1 };
    static constexpr UINT triangles[36] =
    {
        0, 1, 2,  0, 2, 3, // -w face
        4, 6, 5,  4, 7, 6, // +w face
        0, 4, 5,  0, 5, 1, // -v face
        3, 2, 6,  3, 6, 7, // +v face
        0, 3, 7,  0, 7, 4, // -u face
        1, 5, 6,  1, 6, 2  // +u face
    };

    const UINT baseVertex = static_cast<UINT>(vertices.size());

    for (UINT corner = 0; corner < 8; ++corner)
    {
        const XMVECTOR p = XMVectorScale(frame.u, signsU[corner] * halfExtent)
                         + XMVectorScale(frame.v, signsV[corner] * halfExtent)
                         + XMVectorScale(frame.w, centre + signsW[corner] * halfLength);
        XMFLOAT3 position;
        XMStoreFloat3(&position, p);
        appendVertex(position, color, XMFLOAT2(0.0f, 0.0f));
    }

    for (UINT cornerIndex : triangles)
    {
        indices.push_back(baseVertex + cornerIndex);
    }
}

// Cylinder from 0 to kShaftLength along the frame axis.
void Gizmo::appendShaft(const AxisFrame& frame, const XMFLOAT3& color)
{
    using namespace GizmoGeometry;

    const UINT baseVertex = static_cast<UINT>(vertices.size());

    for (UINT ring = 0; ring < 2; ++ring)
    {
        const float axialOffset = (ring == 0) ? 0.0f : kShaftLength;
        for (UINT i = 0; i < kSegments; ++i)
        {
            const float theta = XM_2PI * static_cast<float>(i) / static_cast<float>(kSegments);
            appendVertex(framePoint(frame, kShaftRadius, axialOffset, theta),
                         color, XMFLOAT2(0.0f, static_cast<float>(ring)));
        }
    }

    for (UINT i = 0; i < kSegments; ++i)
    {
        const UINT i0 = baseVertex + i;
        const UINT i1 = baseVertex + (i + 1) % kSegments;
        const UINT i2 = baseVertex + kSegments + i;
        const UINT i3 = baseVertex + kSegments + (i + 1) % kSegments;

        indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
        indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
    }
}

// Rendering
void Gizmo::Draw(Camera camera, RenderMode /*mode*/)
{
    auto liveTarget = target.lock();
    if (!liveTarget)
        return;

    const XMFLOAT3 pos = liveTarget->GetPosition();
    world = XMMatrixScaling(gizmoScale, gizmoScale, gizmoScale)
          * XMMatrixTranslation(pos.x, pos.y, pos.z);

    // Save the current depth state, swap in the always-on-top one, restore after.
    ID3D11DepthStencilState* previousDS = nullptr;
    UINT previousStencilRef = 0;
    context->OMGetDepthStencilState(&previousDS, &previousStencilRef);
    context->OMSetDepthStencilState(depthAlwaysOn.Get(), 0);

    solidOverlay(camera);
    context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);

    context->OMSetDepthStencilState(previousDS, previousStencilRef);
    if (previousDS) previousDS->Release();
}
