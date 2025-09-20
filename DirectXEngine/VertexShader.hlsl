cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
};

struct VS_IN
{
    float3 pos : POSITION;
    float3 col : COLOR;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT output;

    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    float4 viewPos = mul(worldPos, view);
    output.pos = mul(viewPos, proj);

    output.col = input.col; // just pass color along
    return output;
}
