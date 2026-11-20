// 3D Gaussian Splatting (3DGS) pixel shader.
// Evaluates the screen space Gaussian handed over by the vertex shader and
// outputs premultiplied alpha, which pairs with an ONE / INV_SRC_ALPHA blend.

cbuffer SplatFrameBuffer : register(b0)
{
    float2 focal;
    float2 viewportSize;
    float splatScale;
    float alphaCutoff;
    float2 framePadding;
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float3 conic : TEXCOORD0;
    float2 delta : TEXCOORD1;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    // -0.5 * d^T * conic * d
    float power = -0.5f * (input.conic.x * input.delta.x * input.delta.x
                         + input.conic.z * input.delta.y * input.delta.y)
                  - input.conic.y * input.delta.x * input.delta.y;

    if (power > 0.0f)
    {
        power = 0.0f;
    }

    float alpha = min(0.99f, input.color.a * exp(power));
    clip(alpha - alphaCutoff);

    return float4(input.color.rgb * alpha, alpha);
}
