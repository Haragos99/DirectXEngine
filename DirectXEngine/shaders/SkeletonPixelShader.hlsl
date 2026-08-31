// Skeleton pixel shader.
// The bone geometry carries no normals, so the face normal is reconstructed
// from the screen space derivatives of the world position. That flat shading is
// exactly what a joint/bone widget wants: every facet of the octahedral bones
// reads as a distinct plane.
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

static const float kAmbient = 0.45f;

float4 PSMain(PS_IN input) : SV_TARGET
{
    float3 normal = normalize(cross(ddx(input.worldPos), ddy(input.worldPos)));
    float3 toLight = normalize(-lightDirection);

    // Bones are drawn without backface culling, so the reconstructed normal can
    // point either way; the absolute value keeps both sides lit.
    float diffuse = kAmbient + (1.0f - kAmbient) * abs(dot(normal, toLight));

    return float4(input.color * lightColor.rgb * diffuse, 1.0f);
}
