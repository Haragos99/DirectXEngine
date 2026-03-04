#include "raytracer.h"
#include <d3dcompiler.h>
#include <stdexcept>
#include "../External/DDSTextureLoader.h"

using namespace DirectX;

Raytracer::Raytracer(Microsoft::WRL::ComPtr<ID3D11Device> _device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context) : device(_device), context(_context)
{
    shaderRay = std::make_unique<RaytraceShader>(device, context);
    // Fullscreen quad
    std::vector<VertexRT> vertices 
    {
        { {-1, -1, 0}, {0, 1} },
        { {-1,  1, 0}, {0, 0} },
        { { 1,  1, 0}, {1, 0} },
        { { 1, -1, 0}, {1, 1} },
    };
    createIndeces();
	shaderRay->LoadShaders(L"shaders\\RaytracerVS.hlsl", L"shaders\\RaytracerPS.hlsl");
    shaderRay->createVertexRTBuffer(vertices);
    shaderRay->createInexxBuffer(indices);
	shaderRay->createCameraBuffer();
    shaderRay->creatQuadricsBuffer();

	initSpheres();

}


void Raytracer::createIndeces()
{
    indices = {
        0,1,2, 0,2,3
	};
}
void Raytracer::Update(float time)
{
    for (auto& s : spheres)
    {
        //s.velocity.y -= 9.81f * time;
        s.position.x += s.velocity.x * time;
        s.position.y += s.velocity.y * time;
        s.position.z += s.velocity.z * time;

        // Optional gravity
        resolveFloorCollision(s);
    }

    resolveSphereCollisions();
}


void Raytracer::resolveSphereCollisions()
{
    for (size_t i = 0; i < spheres.size(); ++i)
    {
        for (size_t j = i + 1; j < spheres.size(); ++j)
        {
            auto& A = spheres[i];
            auto& B = spheres[j];

            XMVECTOR posA = XMLoadFloat3(&A.position);
            XMVECTOR posB = XMLoadFloat3(&B.position);

            XMVECTOR diff = posB - posA;
            float dist = XMVectorGetX(XMVector3Length(diff));
            float minDist = A.radius + B.radius;

            if (dist < minDist)
            {
                resolveCollision(A, B, diff, dist, minDist);
            }
        }
    }
}


void Raytracer::resolveCollision(
    SpherePhysics& A,
    SpherePhysics& B,
    DirectX::XMVECTOR diff,
    float dist,
    float minDist)
{
    XMVECTOR normal = XMVector3Normalize(diff);

    XMVECTOR velA = XMLoadFloat3(&A.velocity);
    XMVECTOR velB = XMLoadFloat3(&B.velocity);

    float m1 = A.mass;
    float m2 = B.mass;

    float v1 = XMVectorGetX(XMVector3Dot(velA, normal));
    float v2 = XMVectorGetX(XMVector3Dot(velB, normal));

    // 1D elastic collision formula
    float newV1 = (v1 * (m1 - m2) + 2 * m2 * v2) / (m1 + m2);
    float newV2 = (v2 * (m2 - m1) + 2 * m1 * v1) / (m1 + m2);

    velA += normal * (newV1 - v1);
    velB += normal * (newV2 - v2);

    XMStoreFloat3(&A.velocity, velA);
    XMStoreFloat3(&B.velocity, velB);

    // Positional correction (prevent sinking)
    float penetration = minDist - dist;
    XMVECTOR correction = normal * (penetration * 0.5f);

    XMStoreFloat3(&A.position, XMLoadFloat3(&A.position) - correction);
    XMStoreFloat3(&B.position, XMLoadFloat3(&B.position) + correction);
}





SphereBuffer Raytracer::initElemt()
{
    SphereBuffer sb = {};
    sb.sphereCount = spheres.size();

    for (int i = 0; i < spheres.size(); i++)
    {
        sb.spheres[i].center = spheres[i].position;
        sb.spheres[i].radius = spheres[i].radius;
    }

	return sb;
}


void Raytracer::Draw(Camera camera)
{
	auto view = camera.GetViewMatrix();
	auto proj = camera.GetProjectionMatrix();
	auto camPos = camera.getPos();

    // Compute inverse view-projection
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    CameraCB cb;
    XMStoreFloat3(&cb.camPos, XMLoadFloat3(&camPos));
    cb.invViewProj = XMMatrixTranspose(invViewProj);

	SphereBuffer sb = initElemt();

	shaderRay->renderRaytraceDraw(cb, sb);

    context->DrawIndexed(6, 0, 0);
}



void Raytracer::initSpheres()
{
    spheres.clear();
    spheres.resize(16);

    // -------- Row 1 --------
    spheres[0] = { {0.0f, 12.0f, 2.0f},  { 0.5f, -1.0f, 0.0f }, 1.0f,  1.0f };
    spheres[1] = { {2.0f, -10.0f, 4.0f},  {-0.8f,  0.0f, 0.0f }, 0.5f,  1.0f };
    spheres[2] = { {-2.0f,-1.0f, 3.0f},  { 0.7f,  0.0f, 0.0f }, 0.75f, 1.0f };
    spheres[3] = { {1.5f,-1.0f, 5.0f},   {-0.4f,  0.0f, 0.0f }, 1.25f, 1.5f };

    // -------- Row 2 --------
    spheres[4] = { {0.0f,-2.5f, 6.0f},   { 0.6f,  0.0f, 0.0f }, 0.9f,  1.2f };
    spheres[5] = { {-3.0f,-1.5f,2.0f},   { 0.9f,  0.0f, 0.0f }, 1.1f,  1.4f };
    spheres[6] = { {3.0f,-1.0f, 7.0f},   {-0.5f,  0.0f, 0.0f }, 0.6f,  0.8f };
    spheres[7] = { {1.0f,-3.0f, 4.0f},   { 0.3f,  0.0f, 0.0f }, 1.4f,  1.8f };

    // -------- Row 3 --------
    spheres[8] = { {-1.0f,-2.0f,5.0f},   {-0.6f,  0.0f, 0.0f }, 0.8f,  1.0f };
    spheres[9] = { {2.5f,-0.5f, 3.5f},   { 0.4f,  1.0f, 0.0f }, 1.2f,  1.5f };
    spheres[10] = { {-2.5f,-2.0f,6.5f},   { 0.2f,  0.0f, 0.0f }, 0.7f,  0.9f };
    spheres[11] = { {0.5f,-3.0f, 2.5f},   {-0.7f,  0.0f, 0.0f }, 1.3f,  1.6f };

    // -------- Row 4 --------
    spheres[12] = { {3.5f, -1.0f, 5.5f},   {-0.3f,  0.0f, 0.0f }, 0.95f, 1.1f };
    spheres[13] = { {-1.5f,-1.5f,3.0f},   { 0.5f,  -1.0f, 0.0f }, 1.05f, 1.3f };
    spheres[14] = { {2.0f,-2.0f, 6.0f},   {-0.2f,  0.0f, 0.0f }, 0.85f, 1.0f };
    spheres[15] = { {0.0f,-4.0f, 8.0f},   { 0.0f,  0.0f, 0.0f }, 1.5f,  2.0f };
}
void Raytracer::resolveFloorCollision(SpherePhysics& s)
{

    const float FLOOR_Y = 1.0f;
    const float RESTITUTION = 0.f;   // bounciness [0..1]
    const float FRICTION = 0.999f;  // velocity damping on floor contact
    const float SLEEP_EPSILON = 0.01f;  // below this speed, stop bouncing
    float floorContact =  s.radius;


    if (s.position.y - s.radius <= FLOOR_Y)
    {
        // Push sphere above floor
        s.position.y = -(-FLOOR_Y + s.radius);
        // Reflect vertical velocity with restitution
        if (s.velocity.y < 0.0f)
        {
            s.velocity.y *= -FRICTION*10;
        }

        // Apply friction to horizontal movement while on floor
        s.velocity.x *= FRICTION;
        s.velocity.z *= FRICTION;
    }
}