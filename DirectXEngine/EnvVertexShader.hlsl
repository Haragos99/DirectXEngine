cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
};

struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 worldNormal : NORMAL;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    float4 worldPos4 = mul(float4(input.pos, 1), world);
    output.worldPos = worldPos4.xyz;
    output.worldNormal = normalize(mul(float4(input.normal, 0), world).xyz);
    output.posH = mul(mul(worldPos4, view), proj);
    return output;
}
