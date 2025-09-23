// World constant buffer
cbuffer WorldBuffer : register(b0)
{
    matrix cModelViewProjection;
}

struct VS_IN
{
    float3 position : POSITION;
};

struct VS_OUT
{
    float4 positionClipSpace : SV_POSITION;
    float3 position : POSITION;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;
    output.positionClipSpace = mul(float4(input.position, 1.0f), cModelViewProjection).xyww;;
    output.position = input.position;
    return output;
}
