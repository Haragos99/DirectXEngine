#include "gizmoarrow.h"
#include <cfloat>

using namespace DirectX;

GizmoArrow::GizmoArrow(Microsoft::WRL::ComPtr<ID3D11Device> _device,
                       Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context)
    : Object3D(_device, _context)
{
    // Build merged mesh: X (red), Y (green), Z (blue).
    createTexturedVertex();
    createIndeces();

    // We do not sample a texture in the gizmo shaders, but Object3D and the
    // Shader helper still bind one, so use a neutral 1x1 white texture.
    texture->CreateSolidColorTexture(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

    shader->createVertexBuffer(vertices);
    shader->createInexxBuffer(indices);
    shader->createConstantBuffer();
    shader->creaetLightBuffer();
    shader->createRasterize();
    shader->LoadShaders(L"shaders\\GizmoVertexShader.hlsl",
                        L"shaders\\GizmoPixelShader.hlsl");

    wireframeEnabled = false;
    name = "GizmoArrow";

    // Depth-stencil state that lets the gizmo draw on top of the scene.
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable    = FALSE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
    device->CreateDepthStencilState(&dsDesc, &depthAlwaysOn);
}

void GizmoArrow::Update(float /*time*/)
{
    // Gizmo does not animate on its own; its world matrix is composed in Draw.
}

void GizmoArrow::AttachTo(std::shared_ptr<Object3D> newTarget)
{
    target = newTarget;
}

void GizmoArrow::Detach()
{
    target.reset();
}

bool GizmoArrow::IsVisible() const
{
    return !target.expired();
}

// ---------------------------------------------------------------------------
// Axis picking and dragging
// ---------------------------------------------------------------------------

// Total local length of an arrow (shaft + head), matching createTexturedVertex.
static constexpr float kArrowLength = 0.8f + 0.25f;

XMVECTOR GizmoArrow::AxisVector(Axis axis)
{
    switch (axis)
    {
    case Axis::X: return XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    case Axis::Y: return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    case Axis::Z: return XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    default:      return XMVectorZero();
    }
}

float GizmoArrow::AxisParamForRay(Axis axis, const XMFLOAT3& anchor,
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
        return 0.0f; // ray parallel to axis
    return (b * e - d) / denom;
}

GizmoArrow::Axis GizmoArrow::PickAxis(const Ray& ray) const
{
    auto liveTarget = target.lock();
    if (!liveTarget)
        return Axis::None;

    const XMFLOAT3 posF = liveTarget->GetPosition();
    const XMVECTOR O = XMLoadFloat3(&posF);
    const XMVECTOR R = XMLoadFloat3(&ray.origin);
    const XMVECTOR D = XMVector3Normalize(XMLoadFloat3(&ray.direction));

    const float length    = kArrowLength * gizmoScale;
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

void GizmoArrow::BeginDrag(Axis axis, const Ray& ray)
{
    auto liveTarget = target.lock();
    if (!liveTarget || axis == Axis::None)
        return;

    dragging   = true;
    activeAxis = axis;
    dragAnchor = liveTarget->GetPosition();
    lastParam  = AxisParamForRay(axis, dragAnchor, ray);
}

void GizmoArrow::UpdateDrag(const Ray& ray)
{
    auto liveTarget = target.lock();
    if (!dragging || !liveTarget)
        return;

    const float param = AxisParamForRay(activeAxis, dragAnchor, ray);
    const float delta = param - lastParam;
    lastParam = param;

    if (mode == GizmoMode::Move)
    {
        // SetPosition applies an incremental world-space translation, so move by
        // the per-frame delta along the locked axis only.
        const XMVECTOR step = XMVectorScale(AxisVector(activeAxis), delta);
        XMFLOAT3 s;
        XMStoreFloat3(&s, step);
        liveTarget->SetPosition(s.x, s.y, s.z);
    }
    else // GizmoMode::Scale
    {
        // Scale along the locked axis about the object's own centre. Dragging the
        // arrow outward grows the object, inward shrinks it.
        float factor = 1.0f + delta;
        if (factor < 0.01f)
            factor = 0.01f;

        const float sx = (activeAxis == Axis::X) ? factor : 1.0f;
        const float sy = (activeAxis == Axis::Y) ? factor : 1.0f;
        const float sz = (activeAxis == Axis::Z) ? factor : 1.0f;

        // Scale about the centre: translate to origin, scale, translate back.
        const XMFLOAT3 c = liveTarget->GetPosition();
        liveTarget->SetPosition(-c.x, -c.y, -c.z);
        liveTarget->Scale(sx, sy, sz);
        liveTarget->SetPosition(c.x, c.y, c.z);
    }
}

void GizmoArrow::EndDrag()
{
    dragging   = false;
    activeAxis = Axis::None;
}

// ---------------------------------------------------------------------------
// Geometry generation
// ---------------------------------------------------------------------------

void GizmoArrow::createTexturedVertex()
{
    vertices.clear();

    const UINT  segments      = 12;
    const float shaftLength   = 0.8f;
    const float shaftRadius   = 0.03f;
    const float headLength    = 0.25f;
    const float headRadius    = 0.09f;

    // X axis - red
    appendArrow(XMFLOAT3(1, 0, 0), XMFLOAT3(1, 0, 0),
                segments, shaftLength, shaftRadius, headLength, headRadius);
    // Y axis - green
    appendArrow(XMFLOAT3(0, 1, 0), XMFLOAT3(0, 1, 0),
                segments, shaftLength, shaftRadius, headLength, headRadius);
    // Z axis - blue
    appendArrow(XMFLOAT3(0, 0, 1), XMFLOAT3(0, 0, 1),
                segments, shaftLength, shaftRadius, headLength, headRadius);
}

void GizmoArrow::createIndeces()
{
    // Indices are appended alongside vertices in appendArrow.
    // Keep the base method a no-op so we don't clear what we just built.
}

// Build one arrow (shaft cylinder + head cone) aligned with `axis`.
// `axis` must be one of the unit basis vectors (+X, +Y, or +Z).
void GizmoArrow::appendArrow(const XMFLOAT3& axis,
                             const XMFLOAT3& color,
                             UINT segments,
                             float shaftLength, float shaftRadius,
                             float headLength,  float headRadius)
{
    // Build a local frame {u, v, w} where w = axis and (u,v) span the circle.
    XMVECTOR w = XMVector3Normalize(XMLoadFloat3(&axis));
    XMVECTOR up = (fabsf(axis.y) < 0.9f)
        ? XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
        : XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    XMVECTOR u = XMVector3Normalize(XMVector3Cross(up, w));
    XMVECTOR v = XMVector3Cross(w, u);

    auto circlePoint = [&](float radius, float axialOffset, float theta) {
        float c = cosf(theta);
        float s = sinf(theta);
        XMVECTOR p = XMVectorScale(u, radius * c)
                   + XMVectorScale(v, radius * s)
                   + XMVectorScale(w, axialOffset);
        XMFLOAT3 out;
        XMStoreFloat3(&out, p);
        return out;
    };

    const UINT baseVertex = static_cast<UINT>(vertices.size());

    // ---- Shaft: cylinder from 0 to shaftLength along w ----
    //   Ring 0 (bottom): [baseVertex, baseVertex+segments)
    //   Ring 1 (top):    [baseVertex+segments, baseVertex+2*segments)
    for (UINT i = 0; i < segments; ++i)
    {
        float theta = XM_2PI * (float)i / (float)segments;
        VertexData v0;
        v0.position = circlePoint(shaftRadius, 0.0f, theta);
        v0.normal   = color;
        v0.texcoord = XMFLOAT2(0.0f, 0.0f);
        vertices.push_back(v0);
    }
    for (UINT i = 0; i < segments; ++i)
    {
        float theta = XM_2PI * (float)i / (float)segments;
        VertexData v1;
        v1.position = circlePoint(shaftRadius, shaftLength, theta);
        v1.normal   = color;
        v1.texcoord = XMFLOAT2(0.0f, 1.0f);
        vertices.push_back(v1);
    }

    // Side faces
    for (UINT i = 0; i < segments; ++i)
    {
        UINT i0 = baseVertex + i;
        UINT i1 = baseVertex + (i + 1) % segments;
        UINT i2 = baseVertex + segments + i;
        UINT i3 = baseVertex + segments + (i + 1) % segments;

        indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
        indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
    }

    // ---- Head: cone from shaftLength to shaftLength+headLength ----
    const UINT coneBaseStart = static_cast<UINT>(vertices.size());
    // Cone base ring (wide)
    for (UINT i = 0; i < segments; ++i)
    {
        float theta = XM_2PI * (float)i / (float)segments;
        VertexData vb;
        vb.position = circlePoint(headRadius, shaftLength, theta);
        vb.normal   = color;
        vb.texcoord = XMFLOAT2(0.0f, 0.0f);
        vertices.push_back(vb);
    }
    // Cone apex
    const UINT apexIndex = static_cast<UINT>(vertices.size());
    {
        VertexData va;
        va.position = circlePoint(0.0f, shaftLength + headLength, 0.0f);
        va.normal   = color;
        va.texcoord = XMFLOAT2(0.5f, 1.0f);
        vertices.push_back(va);
    }

    // Cone side triangles
    for (UINT i = 0; i < segments; ++i)
    {
        UINT i0 = coneBaseStart + i;
        UINT i1 = coneBaseStart + (i + 1) % segments;
        indices.push_back(i0);
        indices.push_back(apexIndex);
        indices.push_back(i1);
    }

    // Cone base cap (so it looks closed from behind)
    const UINT capCenterIndex = static_cast<UINT>(vertices.size());
    {
        VertexData vc;
        vc.position = circlePoint(0.0f, shaftLength, 0.0f);
        vc.normal   = color;
        vc.texcoord = XMFLOAT2(0.5f, 0.5f);
        vertices.push_back(vc);
    }
    for (UINT i = 0; i < segments; ++i)
    {
        UINT i0 = coneBaseStart + i;
        UINT i1 = coneBaseStart + (i + 1) % segments;
        indices.push_back(capCenterIndex);
        indices.push_back(i1);
        indices.push_back(i0);
    }
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------

void GizmoArrow::Draw(Camera camera, RenderMode /*mode*/)
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
