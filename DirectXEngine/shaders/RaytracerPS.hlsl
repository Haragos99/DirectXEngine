cbuffer CameraCB : register(b0)
{
    matrix invViewProj;
    float3 camPos;
    float pad;
};

struct PSInput
{
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};



struct Hit
{
    float t;
    float3 position, normal;
    int mat; // material index
};

struct Ray
{
    float3 start, dir, weight;
};

Hit intersectSphere(Ray ray, Hit current_best_hit)
{
    // Calculate Qudratic equation of sphere 
    float3 center = float3(0, 0, 0);
    float radius = 1.0f;
    float3 rayOriginToCenter = ray.start - center;
    float b = dot(rayOriginToCenter, ray.dir);
    float c_const = dot(rayOriginToCenter, rayOriginToCenter) - radius * radius;
    float disc = b * b - c_const;
    if (disc >= 0.0f) // If the discriminant is negative, there is no intersection
    {
        disc = sqrt(disc);
        float t0 = -b - disc;
        float t1 = -b + disc;

        float t_closest = 9999.0f; // Temporary variable for this sphere's hit

        if(t0 > 0.001f) // Use a small epsilon to avoid self-intersection/fireflies
        {
            t_closest = t0;
        }
        else
        if (t1 > 0.001f)
        {
            t_closest = t1;
        }
        
        if (t_closest < current_best_hit.t)
        {
            current_best_hit.t = t_closest;
            current_best_hit.position = camPos + ray.dir * current_best_hit.t;
            current_best_hit.normal = normalize(current_best_hit.position - center);
        }

    }
    return current_best_hit;

}


Hit intersectPlane(Ray ray, Hit current_best_hit)
{
    float3 planeNormal = float3(0, 1, 0); // Y-up plane (horizontal)
    float planeD = -2.0f; // Plane is at y = -2.0, so Ax+By+Cz+D = 0*x + 1*y + 0*z - 2 = 0
    
    // Ray-Plane Intersection Formula
    float denom = dot(ray.dir, planeNormal);

    // Check if the ray is not parallel to the plane (denominator is close to zero)
    if (abs(denom) > 0.0001f)
    {
        float t_plane = -(dot(camPos, planeNormal) + planeD) / denom;
        
        if (t_plane > 0.001f && t_plane < current_best_hit.t)
        {
            current_best_hit.t = t_plane;
            current_best_hit.normal = planeNormal; // The normal is constant for a plane
            current_best_hit.position = ray.start + ray.dir * current_best_hit.t;
        
            // Simple checkerboard pattern on the plane
            float s = 0.5f;
            float check_x = floor(current_best_hit.position.x / s);
            float check_z = floor(current_best_hit.position.z / s);
            float check_val = fmod(check_x + check_z, 2.0f);
            // Adjust the plane's normal slightly for the pattern color
            if (check_val == 0.0f)
            {
                current_best_hit.normal *= 0.8f; // Darker square
            }
        }

        
    }    
    return current_best_hit;
}


Hit firstIntersect(Ray ray)
{
    Hit bestHit = (Hit)0;
    bestHit.t = 9999.0f;
    bestHit.normal = float3(0, 0, 0); // Default normal for a miss
    bestHit = intersectPlane(ray, bestHit);
    bestHit = intersectSphere(ray, bestHit);

    if (bestHit.t >= 9999.0f)
    {
        bestHit.t = -1.0f;
    }
    return bestHit;
    
}



float3 trace(Ray ray)
{

    Hit hit = firstIntersect(ray);
    // Check if the intersection is in front of the camera
    if (hit.t < 0)
    {
        hit.normal = float3(0.2, 0.2, 0.3);

    }
    return hit.normal * 0.5f + 0.5f;
}


float4 PSMain(PSInput input) : SV_TARGET
{
    // Reconstruct ray direction
    float2 ndc = input.uv * 2.0f - 1.0f; //Normalized Device Coordinates
    float4 clip = float4(ndc, 0, 1);
    float4 worldPos = mul(clip, invViewProj);
    worldPos /= worldPos.w; // perspective divide

    Ray ray;
    ray.start = camPos;
    ray.dir = normalize(worldPos.xyz - camPos);;
    ray.weight = float3(1, 1, 1);
    
    // Use the final normal for shading
    return float4(trace(ray), 1);
}