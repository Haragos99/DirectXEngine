// Gizmo pixel shader: emits the per-vertex color unlit.
struct VS_OUT
{
    float4 pos   : SV_POSITION;
    float3 color : COLOR0;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}
