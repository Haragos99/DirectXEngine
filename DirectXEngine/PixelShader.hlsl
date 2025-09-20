struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

float4 PSMain(VS_OUT input) : SV_TARGET
{
    return float4(input.col, 1.0f); // RGB + alpha = 1
}
