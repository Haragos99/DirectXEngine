#pragma target ps_3_0



struct Sphere
{
    float3 center;
    float radius;
};





cbuffer CameraCB : register(b0)
{
    matrix invViewProj;
    float3 camPos;
    float pad;
};


// Define a constant buffer to pass spheres from CPU to GPU
cbuffer SphereBuffer : register(b1)
{
    Sphere spheres[16]; // up to 16 spheres
    int sphereCount; // actual number of spheres
    float3 _padding;
};

struct PSInput
{
   float4 pos : SV_POSITION;
   float2 uv : TEXCOORD;
};



const int maxdepth = 5;


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




Hit intersectSphere(Ray ray, Hit current_best_hit, Sphere sphere)
{
    // Calculate Qudratic equation of sphere 
    float3 rayOriginToCenter = ray.start - sphere.center;
    float b = dot(rayOriginToCenter, ray.dir);
    float c_const = dot(rayOriginToCenter, rayOriginToCenter) - sphere.radius * sphere.radius;
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
            // Update the closest hit information
            current_best_hit.t = t_closest;
            current_best_hit.position = camPos + ray.dir * current_best_hit.t;
            current_best_hit.normal = normalize(current_best_hit.position - sphere.center);
            current_best_hit.mat = 0; 
        }

    }
    return current_best_hit;

}


Hit intersectPlane(Ray ray, Hit current_best_hit)
{
    float3 planeNormal = float3(0, 1, 0); // Y-up plane (horizontal)
    float planeD = -1.0f; // Plane is at y = -1.0,
    
    float denom = dot(ray.dir, planeNormal);

    // Check if the ray is not parallel to the plane (denominator is close to zero)
    if (abs(denom) > 0.0001f)
    {
        // Calculate intersection distance t
        float t_plane = -(dot(camPos, planeNormal) + planeD) / denom;
        
        if (t_plane > 0.001f && t_plane < current_best_hit.t)
        {
            current_best_hit.t = t_plane;
            current_best_hit.normal = planeNormal; // The normal is constant for a plane
            current_best_hit.position = ray.start + ray.dir * current_best_hit.t;
            current_best_hit.mat = 1; // Assign a material index for the plane
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
    for (int i = 0; i < sphereCount; ++i)
    {
        bestHit = intersectSphere(ray, bestHit, spheres[i]);
    }   
    if (dot(ray.dir, bestHit.normal) > 0)
    {
        bestHit.normal = bestHit.normal * (-1);
    }
    // If no intersection was found, set t to -1
    if (bestHit.t >= 9999.0f)
    {
        bestHit.t = -1.0f;
    }
    return bestHit;
    
}

float  Fresnel(float3 F0, float cosTheta)
{
    return F0+ (float3(1, 1, 1) - F0) * pow(cosTheta, 5);
}

float3 trace(Ray ray)
{
    // Material properties
    float epsilon = 0.01f;
    float3 ks = float3(1, 1, 1);
    float3 kd = float3(1.5f, 0.6f, 0.5f);
    float3 ka = float3(0.5f, 0.5f, 0.5f);
    float shininess = 70.0f;
    float3 La = float3(0.5f, 0.6f, 0.6f);
    float3 Le = float3(0.9f, 0.9f, 0.9f);
    float3 lightPositions[5] =
    {
    float3(0.87f, -3.87f, 0.25f),
    float3(1.5f, -2.0f, -0.5f),
    float3(2.0f, -1.0f, 3.0f),
    float3(-2.0f, -1.0f, 3.0f),
    float3(1.0f, -1.0f, -3.0f)
    };
    float3 outRadiance = float3(0, 0, 0);
    for (int i = 0; i < 5; ++i)
    {
        Hit hit = firstIntersect(ray);
        if (hit.t < 0)
            break;
        // Direct illumination
        if (hit.mat == 0)
        {
            float3 lightdir = normalize(lightPositions[i] - hit.position);
            float cosTheta = dot(hit.normal, lightdir);
            if (cosTheta > 0)
            {
                float3 LeIn = Le / dot(lightPositions[i] - hit.position, lightPositions[i] - hit.position);
                outRadiance += ray.weight * LeIn * kd * cosTheta;
                float3 halfway = normalize(-ray.dir + lightdir);
                float cosDelta = dot(hit.normal, halfway);
                if (cosDelta > 0)
                {
                    outRadiance += ray.weight * LeIn * ks * pow(cosDelta, shininess);
                }
            }
        }
        // Refflected Ambient term
        if (hit.mat == 1)
        {
            ray.weight *= Fresnel(float3(0.2,0.4,0.2), dot(-ray.dir, hit.normal));
            ray.start = hit.position + hit.normal * epsilon;
            ray.dir = reflect(ray.dir, hit.normal);
        }

    }
    outRadiance += ray.weight * La;
    return outRadiance;
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
    
    return float4(trace(ray), 1);
}