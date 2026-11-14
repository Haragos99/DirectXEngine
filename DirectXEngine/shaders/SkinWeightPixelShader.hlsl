// Skin weight view, pixel stage. The colour arrives per vertex already blended
// from the bones' own colours (or as a heat ramp for a single bone), so skin
// and skeleton read as one picture. The NORMAL slot is taken by that colour,
// so the facet normal is rebuilt from the world position: the view is flat shaded.
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

    // Lit on both sides, so an inside-out or open mesh still reads.
    const float diffuse = kAmbient + (1.0f - kAmbient) * abs(dot(normal, toLight));

    // Not tinted by the light colour: the weight colours have to stay comparable
    // with the bones' and with the weight table.
    return float4(input.color * diffuse, 1.0f);
}
