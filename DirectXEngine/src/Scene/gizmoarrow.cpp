#include "gizmoarrow.h"

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
