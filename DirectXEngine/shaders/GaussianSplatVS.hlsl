// 3D Gaussian Splatting (3DGS) vertex shader.
//
// Every splat is drawn as one instance of a 4 vertex quad. The instance stream
// only carries an index into the splat buffer, so the CPU can re-sort the draw
// order back to front without touching the splat payload. The 3D covariance
// built from the splat scale and rotation is projected into a 2D screen space
// covariance (EWA splatting), the quad is sized to cover 3 sigma of it and the
// inverse covariance ("conic") is handed to the pixel shader.

cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
};

cbuffer SplatFrameBuffer : register(b1)
{
    float2 focal;        // focal length in pixels
    float2 viewportSize; // render target size in pixels
    float splatScale;    // global size multiplier
    float alphaCutoff;
    float2 framePadding;
};

struct SplatData
{
    float3 center;
    float centerPadding;
    float3 scale;
    float scalePadding;
    float4 rot;   // (x, y, z, w)
    float4 color; // rgb + opacity
};

StructuredBuffer<SplatData> splats : register(t0);

struct VS_IN
{
    float2 corner : CORNER;    // per-vertex, in {-1, +1}
    uint index : SPLATINDEX;   // per-instance, back to front
};

struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR0;
    float3 conic : TEXCOORD0; // inverse 2D covariance (a, b, c)
    float2 delta : TEXCOORD1; // pixel offset from the splat centre
};

float3x3 QuaternionToMatrix(float4 q)
{
    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;

    return float3x3(
        1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y),
        2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x),
        2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y));
}

VS_OUT Culled()
{
    VS_OUT output;
    // x/w outside [-1, 1] so the rasteriser discards the whole quad.
    output.position = float4(2.0f, 2.0f, 2.0f, 1.0f);
    output.color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    output.conic = float3(1.0f, 0.0f, 1.0f);
    output.delta = float2(0.0f, 0.0f);
    return output;
}

VS_OUT VSMain(VS_IN input)
{
    SplatData splat = splats[input.index];

    float4 worldPos = mul(float4(splat.center, 1.0f), world);
    float4 viewPos = mul(worldPos, view);

    if (viewPos.z < 0.2f)
    {
        return Culled();
    }

    // 3D covariance: sigma = (R * S) * (R * S)^T, i.e. the rotation columns scaled.
    float3 s = max(splat.scale * splatScale, 1e-6f);
    float3x3 rotation = QuaternionToMatrix(normalize(splat.rot));
    float3x3 m = float3x3(
        rotation._m00 * s.x, rotation._m01 * s.y, rotation._m02 * s.z,
        rotation._m10 * s.x, rotation._m11 * s.y, rotation._m12 * s.z,
        rotation._m20 * s.x, rotation._m21 * s.y, rotation._m22 * s.z);
    float3x3 sigma = mul(m, transpose(m));

    // Engine matrices are row-vector; transpose to keep the covariance math in
    // the usual column-vector convention.
    float3x3 objectToView = transpose(mul((float3x3) world, (float3x3) view));

    // Jacobian of the perspective divide at this splat, expressed in pixels.
    float invZ = 1.0f / viewPos.z;
    float3x3 jacobian = float3x3(
        focal.x * invZ, 0.0f, -focal.x * viewPos.x * invZ * invZ,
        0.0f, focal.y * invZ, -focal.y * viewPos.y * invZ * invZ,
        0.0f, 0.0f, 0.0f);

    float3x3 t = mul(jacobian, objectToView);
    float3x3 screenCov = mul(t, mul(sigma, transpose(t)));

    // Low pass filter so sub-pixel splats stay at least one pixel wide.
    float a = screenCov._m00 + 0.3f;
    float b = screenCov._m01;
    float c = screenCov._m11 + 0.3f;

    float determinant = a * c - b * b;
    if (determinant <= 0.0f)
    {
        return Culled();
    }

    VS_OUT output;
    output.color = splat.color;
    output.conic = float3(c / determinant, -b / determinant, a / determinant);

    // Quad half size = 3 standard deviations along the major axis.
    float mid = 0.5f * (a + c);
    float majorEigenvalue = mid + sqrt(max(0.1f, mid * mid - determinant));
    float radius = 3.0f * sqrt(majorEigenvalue);

    float2 pixelOffset = input.corner * radius;
    output.delta = pixelOffset;

    float4 clipPos = mul(viewPos, proj);
    output.position = float4(clipPos.xy + pixelOffset * (2.0f / viewportSize) * clipPos.w, clipPos.zw);
    return output;
}
