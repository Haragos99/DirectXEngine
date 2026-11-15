cbuffer LightBuffer : register(b1)
{
    float3 lightDirection;
    float  lightPadding;
    float4 lightColor;
};

struct PS_IN
{
    float4 pos      : SV_POSITION;
    float3 worldPos : POSITION;
    float3 color    : COLOR0;
};

static const float kAmbient = 0.55f;

float4 PSMain(PS_IN input) : SV_TARGET
{
    const float3 normal = normalize(cross(ddx(input.worldPos), ddy(input.worldPos)));
    const float3 toLight = normalize(-lightDirection);

    const float diffuse = kAmbient + (1.0f - kAmbient) * abs(dot(normal, toLight));

    return float4(input.color * diffuse, 1.0f);
}
