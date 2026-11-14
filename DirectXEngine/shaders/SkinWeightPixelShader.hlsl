// Colours a skinned mesh by the bone that drives each vertex. The skinning
// dispatch puts (dominant influence, its weight) in the texcoord while this is
// in use, so no extra vertex attribute is needed.
//
// Influence i is bone i, so the palette must stay in step with BoneColors::For
// in bone.cpp. A rig without bones has one root influence, drawn in its colour 0.

struct VS_OUT
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float3 worldPos : TEXCOORD0;
    float2 tex : TEXTURE;
};

static const float3 kPalette[24] =
{
    float3(0.0f, 1.0f, 1.0f), float3(1.0f, 1.0f, 0.0f), float3(1.0f, 0.0f, 1.0f),
    float3(0.5f, 1.0f, 0.5f), float3(1.0f, 0.5f, 0.5f), float3(0.5f, 0.5f, 1.0f),
    float3(0.2f, 0.4f, 0.4f), float3(0.7f, 0.3f, 0.0f), float3(0.0f, 0.3f, 0.7f),
    float3(0.0f, 0.7f, 0.3f), float3(0.7f, 0.0f, 0.3f), float3(0.3f, 0.0f, 0.7f),
    float3(0.3f, 0.7f, 0.0f), float3(0.7f, 0.0f, 0.0f), float3(0.0f, 0.7f, 0.0f),
    float3(0.0f, 0.0f, 0.7f), float3(0.7f, 0.7f, 0.7f), float3(0.5f, 1.0f, 0.2f),
    float3(1.0f, 0.6f, 0.2f), float3(0.4f, 0.5f, 1.0f), float3(0.1f, 0.5f, 0.5f),
    float3(0.5f, 0.3f, 0.0f), float3(0.1f, 0.3f, 0.7f), float3(0.1f, 0.7f, 0.3f),
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    const uint influence = (uint) (input.tex.x + 0.5f);
    const float weight = saturate(input.tex.y);

    const float3 color = kPalette[influence % 24];

    // A little shading keeps the silhouette readable against flat colour.
    const float3 N = normalize(input.normal);
    const float3 L = normalize(float3(0.4f, 0.8f, -0.4f));
    const float shade = 0.6f + 0.4f * saturate(dot(N, L));

    return float4(color * weight * shade, 1.0f);
}
