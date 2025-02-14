#include "DX11App.h"
#include "ShaderManager.h"
#include "Components.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Collision.h"
#include "MeshLoader.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <sstream>
#include <format>
#include <variant>
#include "DDSTextureLoader.h"
#include "IntegratorSystem.h"
#include "ColliderUpdateSystem.h"
#include "BroadPhaseUpdateSystem.h"
#include "NarrowPhaseSystem.h"
#include <chrono>

#define ThrowIfFailed(x)  if (FAILED(x)) { throw new std::bad_exception;}

Ray DX11App::GetRayFromScreenPosition(int x, int y)
{
    // get inverted camera matrices
    XMMATRIX invertedProj = XMMatrixInverse(nullptr, m_camera->GetProjection());
    XMMATRIX invertedView = XMMatrixInverse(nullptr, m_camera->GetView());

    // convert to 3d nds
    float viewportX = (2.0f * x) / m_viewport.Width - 1.0f;
    float viewportY = 1.0f - (2.0f * y) / m_viewport.Height;
    XMFLOAT3 rayNDS = XMFLOAT3(viewportX, viewportY, 1.0f);

    // convert to homogenous clip coordinates
    XMVECTOR rayClip = XMVectorSet(rayNDS.x, rayNDS.y, 1.0f, 1.0f);

    // convert to eye coordinates
    XMVECTOR rayEye = XMVector4Transform(rayClip, invertedProj);
    //rayEye = XMVectorSetZ(rayEye, -1.0f);
    rayEye = XMVectorSetW(rayEye, 0.0f);

    // convert to world coordinates
    XMFLOAT3 rayDirection;
    XMStoreFloat3(&rayDirection, XMVector3Normalize(XMVector4Transform(rayEye, invertedView)));

    XMFLOAT3 cameraPosition = m_camera->GetPosition();
    Ray ray = Ray(Vector3(cameraPosition.x, cameraPosition.y, cameraPosition.z), Vector3(rayDirection.x, rayDirection.y, rayDirection.z));

    return ray;
}

DX11App::~DX11App()
{
    if (m_defaultRasterizerState)m_defaultRasterizerState->Release();
    if (m_defaultSamplerState)m_defaultSamplerState->Release();

    // delete the instance of the shader manager
    ShaderManager::ReleaseInstance();
}

HRESULT DX11App::Init()
{
    // set the default state for the HRESULT value
    HRESULT hr = S_OK;

    // create the default rasterizer state
    D3D11_RASTERIZER_DESC rasterizerDesc = {};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;

    hr = m_device->CreateRasterizerState(&rasterizerDesc, &m_defaultRasterizerState);
    if (FAILED(hr)) return hr;

    // create the wireframe rasterizer state
    D3D11_RASTERIZER_DESC wireFrameRasterizerDesc = {};
    wireFrameRasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireFrameRasterizerDesc.CullMode = D3D11_CULL_NONE;

    hr = m_device->CreateRasterizerState(&wireFrameRasterizerDesc, &m_wireframeRasterizerState);
    if (FAILED(hr)) return hr;

    // create the default sampler state
    D3D11_SAMPLER_DESC bilinearSamplerDesc = {};
    bilinearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
    bilinearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
    bilinearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    bilinearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    bilinearSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    bilinearSamplerDesc.MinLOD = 0;

    hr = m_device->CreateSamplerState(&bilinearSamplerDesc, &m_defaultSamplerState);
    if (FAILED(hr)) return hr;

    const UINT maxVertices = maxDebugSprings * 2;

    D3D11_BUFFER_DESC vertexBufferDesc = {};
    vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth = sizeof(SimpleVertex) * maxVertices;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = m_device->CreateBuffer(&vertexBufferDesc, nullptr, &m_springVertexBuffer);
    if (FAILED(hr)) return hr;

    // create skybox depth buffer state
    D3D11_DEPTH_STENCIL_DESC dsDesc;
    ZeroMemory(&dsDesc, sizeof(dsDesc));
    dsDesc.DepthEnable = true;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    m_device->CreateDepthStencilState(&dsDesc, &m_skyboxDepthState);

    // create the layout for the default input used in most shaders
    D3D11_INPUT_ELEMENT_DESC defaultInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    D3D11_INPUT_ELEMENT_DESC instancedInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "INSTANCE_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_SCALE", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
    };

    // get an instance of the shader manager and initialize it
    ShaderManager* sm = ShaderManager::GetInstance();
    sm->Init(m_device, m_immediateContext);

    // create the constant buffers
    hr = sm->CreateConstantBuffer<TransformBuffer>("TransformBuffer");
    ThrowIfFailed(hr);
    hr = sm->CreateConstantBuffer<GlobalBuffer>("GlobalBuffer");
    ThrowIfFailed(hr);
    hr = sm->CreateConstantBuffer<LightBuffer>("LightBuffer");
    ThrowIfFailed(hr);
    hr = sm->CreateConstantBuffer<ObjectBuffer>("ObjectBuffer");
    ThrowIfFailed(hr);

    // load the shaders from file
    // simple shader:
    hr = sm->LoadVertexShader(m_windowHandle, "SimpleShaders", L"SimpleShaders.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->LoadPixelShader(m_windowHandle, "SimpleShaders", L"SimpleShaders.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->CreateInputLayout("SimpleShaders", instancedInputLayout, ARRAYSIZE(instancedInputLayout));
    if (FAILED(hr)) { return hr; }

    sm->RegisterConstantBuffer("SimpleShaders", "TransformBuffer", 0);
    sm->RegisterConstantBuffer("SimpleShaders", "GlobalBuffer", 1);
    sm->RegisterConstantBuffer("SimpleShaders", "LightBuffer", 2);
    sm->RegisterConstantBuffer("SimpleShaders", "ObjectBuffer", 3);

    // debug shader:
    hr = sm->LoadVertexShader(m_windowHandle, "DebugBoxShaders", L"DebugBoxShaders.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->LoadPixelShader(m_windowHandle, "DebugBoxShaders", L"DebugBoxShaders.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->CreateInputLayout("DebugBoxShaders", defaultInputLayout, ARRAYSIZE(defaultInputLayout));
    if (FAILED(hr)) { return hr; }

    sm->RegisterConstantBuffer("DebugBoxShaders", "TransformBuffer", 0);

    // skybox shader:
    hr = sm->LoadVertexShader(m_windowHandle, "SkyboxShader", L"Skybox.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->LoadPixelShader(m_windowHandle, "SkyboxShader", L"Skybox.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->CreateInputLayout("SkyboxShader", defaultInputLayout, ARRAYSIZE(defaultInputLayout));
    if (FAILED(hr)) { return hr; }

    sm->RegisterConstantBuffer("SkyboxShader", "TransformBuffer", 0);

    // terrain shader:
    hr = sm->LoadVertexShader(m_windowHandle, "TerrainShader", L"Terrain.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->LoadPixelShader(m_windowHandle, "TerrainShader", L"Terrain.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->CreateInputLayout("TerrainShader", defaultInputLayout, ARRAYSIZE(defaultInputLayout));
    if (FAILED(hr)) { return hr; }

    sm->RegisterConstantBuffer("TerrainShader", "TransformBuffer", 0);
    sm->RegisterConstantBuffer("TerrainShader", "GlobalBuffer", 1);
    sm->RegisterConstantBuffer("TerrainShader", "LightBuffer", 2);

    // store the default ambient light into the global buffer
    m_globalBufferData.AmbientLight = XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);

    // create camera
    float aspect = m_viewport.Width / m_viewport.Height;

    m_camera = new Camera();
    m_camera->SetLens(XMConvertToRadians(90), aspect, 0.1f, 500.0f);
    m_camera->SetPosition(XMFLOAT3(0.0f, 2.0f, -5.0f));

    // create lights
    Light directionalLight;
    directionalLight.Type = LightType::DIRECTIONAL;
    directionalLight.Color = XMFLOAT3(0.5f, 0.5f, 0.5f);
    directionalLight.Direction = XMFLOAT3(0.0f, -0.5f, 1.0f);

    Light spotLight;
    spotLight.Type = LightType::POINT;
    spotLight.Color = XMFLOAT3(0.5f, 0.5f, 0.5f);
    spotLight.Position = XMFLOAT3(0.0f, 3.0f, 0.0f);
    //spotLight.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);

    m_lightBufferData.Lights[0] = directionalLight;
    m_lightBufferData.Lights[1] = spotLight;
    m_lightBufferData.LightCount = 2;

    ObjectBuffer objectBuffer;
    objectBuffer.HasTexture = true;

    sm->SetConstantBuffer<ObjectBuffer>("ObjectBuffer", objectBuffer);

    // load meshes
    MeshLoader::LoadMesh("Models/Cube.obj", "Cube", m_device, false);
    MeshLoader::LoadMesh("Models/InvertedCube.obj", "InvertedCube", m_device, false);
    MeshLoader::LoadMesh("Models/Sphere.obj", "Sphere", m_device, false);

    // load textures
    Material* rockMaterial = new Material();
    rockMaterial->LoadTexture(m_device, TextureType::ALBEDO, L"Textures\\Materials\\Crate\\COLOR.dds");
    rockMaterial->LoadTexture(m_device, TextureType::NORMAL, L"Textures\\Materials\\Crate\\NORMAL.dds");
    rockMaterial->LoadTexture(m_device, TextureType::AMBIENT, L"Textures\\Materials\\Crate\\AO.dds");
    rockMaterial->LoadTexture(m_device, TextureType::ROUGHNESS, L"Textures\\Materials\\Crate\\ROUGHNESS.dds");
    rockMaterial->LoadTexture(m_device, TextureType::HEIGHT, L"Textures\\Materials\\Crate\\HEIGHT.dds");

    m_crateMaterialSRV = rockMaterial->GetMaterialAsTextureArray(m_device, m_immediateContext);
    DirectX::CreateDDSTextureFromFile(m_device, L"Textures\\Skyboxes\\MountainSkybox.dds", nullptr, &m_skyboxSRV);

    // initialise and build the scene
    m_scene.Init();
    Collision::Init();

    m_scene.RegisterComponent<Particle>();
    m_scene.RegisterComponent<Transform>();
    m_scene.RegisterComponent<RigidBody>();
    m_scene.RegisterComponent<Collider>();
    m_scene.RegisterComponent<Mesh>();
    m_scene.RegisterComponent<Spring>();
    
    m_scene.RegisterSystem(std::move(std::make_unique<IntegratorSystem>()));
    m_scene.RegisterSystem(std::move(std::make_unique<ColliderUpdateSystem>()));
    m_scene.RegisterSystem(std::move(std::make_unique<BroadPhaseUpdateSystem>(m_aabbTree)));
    m_scene.RegisterSystem(std::move(std::make_unique<NarrowPhaseSystem>(m_aabbTree, m_debugPoints)));

    std::vector<Entity> entities(MAX_ENTITIES);

    std::default_random_engine generator;
    std::uniform_real_distribution<float> randPosition(-30.0f, 30.0f);
    std::uniform_real_distribution<float> randRotation(0.0f, 3.0f);
    std::uniform_real_distribution<float> randScale(3.0f, 5.0f);
    std::uniform_real_distribution<float> randGravity(-10.0f, -1.0f);
    std::uniform_real_distribution<float> randVelocity(-1.5f, 1.5f);

    /* Entity e1 = m_scene.CreateEntity();
    m_scene.AddComponent(
        e1,
        Particle{ Vector3::Left * 2.0f, Vector3::Zero, Vector3::Down, 0.99f, 1 / 2.0f}
    );

    Entity e2 = m_scene.CreateEntity();
    m_scene.AddComponent(
        e2,
        Particle{ Vector3::Right * 2.0f, Vector3::Zero, Vector3::Down, 0.99f, 1 / 2.0f }
    );

    m_aabbTree.InsertEntity(e1, AABB(Vector3::Left * 2.0f, Vector3::One));
    m_aabbTree.InsertEntity(e2, AABB(Vector3::Right * 2.0f, Vector3::One));

    m_aabbTree.PrintTree(m_aabbTree.GetRootIndex());*/

    entities[0] = m_scene.CreateEntity();
    entities[1] = m_scene.CreateEntity();
    entities[2] = m_scene.CreateEntity();
    entities[3] = m_scene.CreateEntity();
    entities[4] = m_scene.CreateEntity();

    Vector3 inverseCubeInertia = Vector3(
        (1.0f / 12.0f) * 2.0f * (1.0f + 1.0f),
        (1.0f / 12.0f) * 2.0f * (1.0f + 1.0f),
        (1.0f / 12.0f) * 2.0f * (1.0f + 1.0f)
    ).reciprocal();

    m_scene.AddComponent(
        entities[0],
        Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 0.0f, 0.6f }
    );
    m_scene.AddComponent(
        entities[0],
        Transform{ Vector3::Zero, Quaternion(), Vector3(10.0f, 1.0f, 10.0f) }
    );
    m_scene.AddComponent(
        entities[0],
        RigidBody{ Vector3::Zero, Vector3::Zero, Vector3::Zero, Matrix3(Vector3::Zero) }
    );
    m_scene.AddComponent(
        entities[0],
        Collider{ OBB(Vector3::Zero, Vector3(10.0f, 1.0f, 10.0f), Quaternion()) }
        //Collider{ AABB::FromPositionScale(Vector3::Zero, Vector3(10.0f, 1.0f, 10.0f)) }
    );
    m_scene.AddComponent(
        entities[0],
        Mesh{ MeshLoader::GetMeshID("Cube") }
    );
    m_aabbTree.InsertEntity(entities[0], AABB::FromPositionScale(Vector3::Zero, Vector3(10.0f, 1.0f, 10.0f)), true);

    // walls
    m_scene.AddComponent(
        entities[2],
        Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 0.0f, 0.15f }
    );
    m_scene.AddComponent(
        entities[2],
        Transform{ Vector3(0.0f, 3.1f, 5.6f), Quaternion(), Vector3(10.0f, 5.0f, 1.0f)}
    );
    m_scene.AddComponent(
        entities[2],
        RigidBody{ Vector3::Zero, Vector3::Zero, Vector3::Zero, Matrix3(Vector3::Zero) }
    );
    m_scene.AddComponent(
        entities[2],
        //Collider{ AABB::FromPositionScale(Vector3(0.0f, 3.1f, 5.6f), Vector3(10.0f, 5.0f, 1.0f)) }
        Collider{ OBB(Vector3(0.0f, 3.1f, 5.6f), Vector3(10.0f, 5.0f, 1.0f), Quaternion()) }
    );
    m_scene.AddComponent(
        entities[2],
        Mesh{ MeshLoader::GetMeshID("Cube") }
    );
    m_aabbTree.InsertEntity(entities[2], AABB::FromPositionScale(Vector3(0.0f, 3.1f, 5.6f), Vector3(10.0f, 5.0f, 1.0f)), true);

    // cube
    //m_scene.AddComponent(
    //    entities[1],
    //    Particle{ Vector3::Zero, Vector3::Zero, 1 / 2.0f, 0.8f }
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    Transform{ Vector3::Up * 5, Quaternion(), Vector3(1.0f, 1.0f, 1.0f)}
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    RigidBody{ Vector3::Zero, Vector3::Zero, inverseCubeInertia, Matrix3(inverseCubeInertia) }
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    Collider{ OBB(Vector3::Up * 5.0f, Vector3::One, Quaternion()) }
    //    //Collider{ AABB::FromPositionScale(Vector3::Up * 5, Vector3::One) }
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    Mesh{ MeshLoader::GetMeshID("Cube") }
    //);
    //m_aabbTree.InsertEntity(entities[1], AABB::FromPositionScale(Vector3::Up * 5.0f, Vector3::One));

    Vector3 testInverseInertia = Vector3(
        (1.0f / 12.0f) * 2.0f * (1.0f + 3.0f),
        (1.0f / 12.0f) * 2.0f * (3.0f + 3.0f),
        (1.0f / 12.0f) * 2.0f * (3.0f + 1.0f)
    ).reciprocal();

    //Vector3 testObjectPosition = Vector3::Up * 5.0f + Vector3::Right * 1.5f;
    //Quaternion testObjectRotation = Quaternion::FromEulerAngles(Vector3(0.0f, 0.0, XMConvertToRadians(30.0f)));
    //m_scene.AddComponent(
    //    entities[1],
    //    Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 1.0f / 2.0f, 0.0f }
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    Transform{ testObjectPosition, testObjectRotation, Vector3(3.0f, 1.0f, 3.0f)}
    //    //Transform{ testObjectPosition, Quaternion(), Vector3(3.0f, 1.0f, 3.0f) }
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    RigidBody{ Vector3::Zero, Vector3::Zero, testInverseInertia, Matrix3(testInverseInertia) }
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    Collider{ OBB(testObjectPosition, Vector3::One, testObjectRotation) }
    //);
    //m_scene.AddComponent(
    //    entities[1],
    //    Mesh{ MeshLoader::GetMeshID("Cube") }
    //);
    //m_aabbTree.InsertEntity(entities[1], AABB::FromPositionScale(testObjectPosition, Vector3::One));

    // CLOTH
    /*m_scene.AddComponent(
        entities[3],
        Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 1 / 1.5f, 0.8f }
    );
    m_scene.AddComponent(
        entities[3],
        Transform{ Vector3::Up * 3, Quaternion(), Vector3(1.0f, 1.0f, 1.0f)}
    );
    m_scene.AddComponent(
        entities[3],
        Mesh{ MeshLoader::GetMeshID("Cube") }
    );
    m_aabbTree.InsertEntity(entities[3], AABB::FromPositionScale(Vector3::Up * 3.0f, Vector3::One));

    m_scene.AddComponent(
        entities[4],
        Spring{ entities[1], entities[3], 3.0f, 0.5f }
    );*/

    /*Entity currentEntityNum = m_scene.CreateEntity();
    m_scene.AddComponent(
        currentEntityNum,
        Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 1 / 0.1f, 0.2f }
    );
    m_scene.AddComponent(
        currentEntityNum,
        Transform{ Vector3::Up * 10.0f, Quaternion(), Vector3(0.05f, 0.05f, 0.05f) }
    );
    m_scene.AddComponent(
        currentEntityNum,
        RigidBody{ Vector3::Zero, Vector3::Zero, Vector3::Zero, Matrix3(Vector3::Zero) }
    );
    m_scene.AddComponent(
        currentEntityNum,
        Collider{ Point(Vector3::Up * 10.0f) }
    );
    m_scene.AddComponent(
        currentEntityNum,
        Mesh{ MeshLoader::GetMeshID("Sphere") }
    );
    m_aabbTree.InsertEntity(currentEntityNum, AABB::FromPositionScale(Vector3::Up * 10.0f, Vector3(0.1f, 0.1f, 0.1f)));*/

    Vector3 startPosition = Vector3(-5.0f, 10.0f, 0.0f);
    float startEntity = m_scene.GetEntityCount() - 1;
    float currentEntityNum = 0;
    constexpr float rows = 30;
    constexpr float cols = 30;
    float spacing = 0.15f;
    float springStiffness = 3.0f;
    constexpr unsigned int pointCount = rows * cols;

    std::array<Entity, pointCount> clothEntities = {};

    for (float y = 0; y < rows; y++)
    {
        for (float x = 0; x < cols; x++)
        {
            Vector3 pointPosition = startPosition + Vector3::Right * x * spacing + Vector3::Forward * y * spacing;
            float anchored = y == 0 || y == rows - 1 || x == 0 || x == cols - 1;
            float mass = anchored ? 0.0f : 1 / 0.05f;
            //float mass = 1.0f / 0.05f;

            currentEntityNum = m_scene.CreateEntity();
            m_scene.AddComponent(
                currentEntityNum,
                Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, mass, 0.2f }
            );
            m_scene.AddComponent(
                currentEntityNum,
                Transform{ pointPosition, Quaternion(), Vector3(0.05f, 0.05f, 0.05f) }
            );
            m_scene.AddComponent(
                currentEntityNum,
                RigidBody{ Vector3::Zero, Vector3::Zero, Vector3::Zero, Matrix3(Vector3::Zero) }
            );
            m_scene.AddComponent(
                currentEntityNum,
                Collider{ Point(pointPosition) }
            );
            m_scene.AddComponent(
                currentEntityNum,
                Mesh{ MeshLoader::GetMeshID("Sphere") }
            );

            m_aabbTree.InsertEntity(currentEntityNum, AABB::FromPositionScale(pointPosition, Vector3(0.1f, 0.1f, 0.1f)), anchored);
            clothEntities[x + y * cols] = currentEntityNum;

            if (x > 0 && y > 0)
            {
                Entity a = clothEntities[x + y * cols];
                Entity b = clothEntities[(x - 1) + y * cols];
                Entity c = clothEntities[x + (y - 1) * cols];
                
                Entity spring1 = m_scene.CreateEntity();
                m_scene.AddComponent(
                    spring1,
                    Spring{ a, b, spacing, springStiffness }
                );

                Entity spring2 = m_scene.CreateEntity();
                m_scene.AddComponent(
                    spring2,
                    Spring{ a, c, spacing, springStiffness }
                );
            }
            if (x == 0 && y > 0)
            {
                Entity a = clothEntities[x + y * cols];
                Entity b = clothEntities[x + (y - 1) * cols];

                Entity spring1 = m_scene.CreateEntity();
                m_scene.AddComponent(
                    spring1,
                    Spring{ a, b, spacing, springStiffness }
                );
            }
            if (x > 0 && y == 0)
            {
                Entity a = clothEntities[x + y * cols];
                Entity b = clothEntities[(x - 1) + y * cols];

                Entity spring1 = m_scene.CreateEntity();
                m_scene.AddComponent(
                    spring1,
                    Spring{ a, b, spacing, springStiffness }
                );
            }
        }
    }

    ////////////////////////////////////////

    // create terrain
    m_terrain = new Terrain();
    m_terrain->Init(m_device, m_immediateContext, "Textures/HeightMaps/TestHeightMap.raw", 100, 100, 150, 150, 10);
    m_terrain->BuildCollision(&m_scene, &m_aabbTree);

    // initialise ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.IniFilename = "ImGui/imgui.ini";

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_windowHandle);
    ImGui_ImplDX11_Init(m_device, m_immediateContext);

    return hr;
}

void DX11App::Update()
{
    double deltaTime = m_timer.GetDeltaTime();

    // store the elapsed time in the global buffer
    m_globalBufferData.TimeStamp = (float)m_timer.GetElapsedTime();

    // handle input for moving the camera
    XMFLOAT3 movementInput = XMFLOAT3(0.0f, 0.0f, 0.0f);
    if (GetAsyncKeyState('W') & 0x8000)
        movementInput.z = 1.0f;
    if (GetAsyncKeyState('S') & 0x8000)
        movementInput.z = -1.0f;
    if (GetAsyncKeyState('A') & 0x8000)
        movementInput.x = -1.0f;
    if (GetAsyncKeyState('D') & 0x8000)
        movementInput.x = 1.0f;
    if (GetAsyncKeyState('E') & 0x8000)
        movementInput.y = 1.0f;
    if (GetAsyncKeyState('Q') & 0x8000)
        movementInput.y = -1.0f;

    // move the camera using the provided input
    XMStoreFloat3(&movementInput, XMVectorScale(XMLoadFloat3(&movementInput), 5.0f * deltaTime));
    m_camera->Move(movementInput);

    // update the cameras position in the global buffer
    XMFLOAT3 camPosition = m_camera->GetPosition();
    m_globalBufferData.CameraPosition = camPosition;

    // update the cameras view matrix
    m_camera->UpdateViewMatrix();

    // update the tranform buffer with the new camera matrices and a default world matrix.
    m_transformBufferData.World = XMMatrixTranspose(XMMatrixIdentity());
    m_transformBufferData.View = XMMatrixTranspose(m_camera->GetView());
    m_transformBufferData.Projection = XMMatrixTranspose(m_camera->GetProjection());

    // update systems
    m_physicsAccumulator += deltaTime;

    while (m_physicsAccumulator >= FPS60)
    {
        m_physicsAccumulator -= FPS60;

        auto start = std::chrono::high_resolution_clock::now();
        m_scene.UpdateSystems(FPS60);
        auto stop = std::chrono::high_resolution_clock::now();

        m_physicsDuration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000.0f;
    }

    // start imgui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Entity Editor");
    if (ImGui::Button("Add Sphere Entity"))
    {
        if (!m_scene.ReachedEntityCap())
        {
            XMFLOAT3 camPos = m_camera->GetPosition();
            XMFLOAT3 camDirection = m_camera->GetLook();

            Entity newEntity = m_scene.CreateEntity();
            m_scene.AddComponent(
                newEntity,
                Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 1 / 2.0f, 0.8f}
            );
            m_scene.AddComponent(
                newEntity,
                Transform{ Vector3(camPos.x, camPos.y, camPos.z), Quaternion(), Vector3(1.0f, 1.0f, 1.0f) }
            );

            float intertiaValue = (2.0f / 5.0f) * 2.0f * (1.0f * 1.0f);
            Vector3 inverseSphereInertia = Vector3(
                intertiaValue,
                intertiaValue,
                intertiaValue
            ).reciprocal();

            m_scene.AddComponent(
                newEntity,
                RigidBody{ Vector3::Zero, Vector3::Zero, inverseSphereInertia, Matrix3(inverseSphereInertia) }
            );

            m_scene.AddComponent(
                newEntity,
                Collider{ Sphere(Vector3(camPos.x, camPos.y, camPos.z), 1.0f) }
            );
            m_scene.AddComponent(
                newEntity,
                Mesh{ MeshLoader::GetMeshID("Sphere") }
            );

            m_aabbTree.InsertEntity(newEntity, AABB::FromPositionScale(Vector3(camPos.x, camPos.y, camPos.z), Vector3::One));

            m_scene.GetComponent<Particle>(newEntity)->ApplyLinearImpulse(Vector3(camDirection.x, camDirection.y, camDirection.z) * 10.0f);
        }
    }
    if (ImGui::Button("Add Box Entity"))
    {
        if (!m_scene.ReachedEntityCap())
        {
            XMFLOAT3 camPos = m_camera->GetPosition();
            XMFLOAT3 camDirection = m_camera->GetLook();

            Entity newEntity = m_scene.CreateEntity();
            m_scene.AddComponent(
                newEntity,
                Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 1 / 2.0f, 0.8f }
            );
            m_scene.AddComponent(
                newEntity,
                Transform{ Vector3(camPos.x, camPos.y, camPos.z), Quaternion(), Vector3::One }
            );

            Vector3 inverseCubeInertia = Vector3(
                (1.0f / 12.0f) * 2.0f * (1.0f + 1.0f),
                (1.0f / 12.0f) * 2.0f * (1.0f + 1.0f),
                (1.0f / 12.0f) * 2.0f * (1.0f + 1.0f)
            ).reciprocal();

            m_scene.AddComponent(
                newEntity,
                RigidBody{ Vector3::Zero, Vector3::Zero, inverseCubeInertia, Matrix3(inverseCubeInertia) }
            );

            m_scene.AddComponent(
                newEntity,
                Collider{ OBB(Vector3(camPos.x, camPos.y, camPos.z), Vector3::One, Quaternion()) }
            );
            m_scene.AddComponent(
                newEntity,
                Mesh{ MeshLoader::GetMeshID("Cube") }
            );

            m_aabbTree.InsertEntity(newEntity, AABB::FromPositionScale(Vector3(camPos.x, camPos.y, camPos.z), Vector3::One));

            m_scene.GetComponent<Particle>(newEntity)->ApplyLinearImpulse(Vector3(camDirection.x, camDirection.y, camDirection.z) * 10.0f);
        }
    }

    if (io.MouseDown[0] && m_currentClickAction == ClickAction::DRAG && m_draggingEntity != INVALID_ENTITY)
    {
        Ray ray = GetRayFromScreenPosition(io.MousePos.x, io.MousePos.y);
        Vector3 newDragPosition = ray.GetOrigin() + ray.GetDirection() * m_draggingDistance;

        Transform* transform = m_scene.GetComponent<Transform>(m_draggingEntity);
        Particle* particle = m_scene.GetComponent<Particle>(m_draggingEntity);

        Vector3 delta = newDragPosition - transform->Position;
        Vector3 velocity = particle->LinearVelocity;
        float stiffness = 10.0f;
        float damping = 2.0f;

        Vector3 force = (delta * stiffness) - (velocity * damping);
        particle->ApplyLinearImpulse(force);
    }

    if (m_selectedEntity != INVALID_ENTITY)
    {
        ImGui::Text(std::format("The current selected entity has ID: {}", m_selectedEntity).c_str());

        if (m_scene.HasComponent<Transform>(m_selectedEntity))
        {
            Transform* transformComponent = m_scene.GetComponent<Transform>(m_selectedEntity);

            ImGui::Text("Transform Component:");
            ImGui::InputFloat3("Position", &transformComponent->Position.x);
            ImGui::InputFloat3("Scale", &transformComponent->Scale.x);

            Quaternion& rotation = transformComponent->Rotation;
            ImGui::SliderFloat4("Rotation (Quaternion)", &rotation.r, -1.0f, 1.0f);
            rotation.normalize();

            Vector3 eulerRotation = rotation.toEulerAngles();
            eulerRotation = Vector3(XMConvertToDegrees(eulerRotation.x),
                XMConvertToDegrees(eulerRotation.y),
                XMConvertToDegrees(eulerRotation.z));
            ImGui::Text("Euler Rotation: %.1f, %.1f, %.1f", eulerRotation.x, eulerRotation.y, eulerRotation.z);
        }

        if (m_scene.HasComponent<Particle>(m_selectedEntity))
        {
            Particle* particleComponent = m_scene.GetComponent<Particle>(m_selectedEntity);

            ImGui::Text("Particle Component:");
            ImGui::InputFloat3("LinearVelocity", &particleComponent->LinearVelocity.x);
            ImGui::InputFloat3("Force", &particleComponent->Force.x);
            ImGui::Text("Mass: %.1f", 1.0f / particleComponent->InverseMass);
        }

        if (m_scene.HasComponent<RigidBody>(m_selectedEntity))
        {
            RigidBody* rigidBodyComponent = m_scene.GetComponent<RigidBody>(m_selectedEntity);
            ImGui::Text("RigidBody Component:");
            ImGui::InputFloat3("AngularVelocity", &rigidBodyComponent->AngularVelocity.x);
            ImGui::InputFloat3("Torque", &rigidBodyComponent->Torque.x);
        }

        if (ImGui::Button("Remove Entity"))
        {
            m_scene.DestroyEntity(m_selectedEntity);
            m_aabbTree.RemoveEntity(m_selectedEntity);
            m_selectedEntity = INVALID_ENTITY;
        }
    }
    else
    {
        ImGui::Text("No entity selected.");
    }
    ImGui::End();

    ImGui::Begin("Application Stats");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    ImGui::Text("Entity Count: %d of %d", m_scene.GetEntityCount(), MAX_ENTITIES);
    ImGui::Text("Physics Computation Time: %.3f ms", m_physicsDuration);
    ImGui::End();

    ImGui::Begin("Click Options");
    ImGui::Text("Choose click action:");
    if (ImGui::Button("Select Mode"))
    {
        m_currentClickAction = ClickAction::SELECT;
    }
    ImGui::SameLine();
    if (ImGui::Button("Push Mode"))
    {
        m_currentClickAction = ClickAction::PUSH;
    }
    ImGui::SameLine();
    if (ImGui::Button("Drag Mode"))
    {
        m_currentClickAction = ClickAction::DRAG;
    }
    ImGui::End();

    m_timer.Tick();
}

void DX11App::Draw()
{
    // get an instance of the shader manager
    ShaderManager* sm = ShaderManager::GetInstance();

    // rebind the render target as it would have been unbinded, then clear it and the depth buffer.
    float backgroundColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
    m_immediateContext->OMSetRenderTargets(1, &m_frameBufferView, m_depthStencilView);
    m_immediateContext->ClearRenderTargetView(m_frameBufferView, backgroundColor);
    m_immediateContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    m_immediateContext->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_immediateContext->RSSetState(m_defaultRasterizerState);

    // update all of the constant buffers
    sm->SetConstantBuffer<LightBuffer>("LightBuffer", m_lightBufferData);
    sm->SetConstantBuffer<TransformBuffer>("TransformBuffer", m_transformBufferData);
    sm->SetConstantBuffer<GlobalBuffer>("GlobalBuffer", m_globalBufferData);

    TransformBuffer transformData;
    sm->GetConstantBufferData<TransformBuffer>("TransformBuffer", transformData);

    // set the bilienar sampler state as the default sampler for shaders
    m_immediateContext->PSSetSamplers(0, 1, &m_defaultSamplerState);

    sm->SetActiveShader("SimpleShaders");

    m_scene.ForEach<Transform, Mesh>([&](Entity entity, Transform* transform, Mesh* mesh) {
        XMMATRIX matrixTransform = XMMatrixScaling(transform->Scale.x, transform->Scale.y, transform->Scale.z) * XMMATRIX(transform->Rotation.toRotationMatrix()) * XMMatrixTranslation(transform->Position.x, transform->Position.y, transform->Position.z);
        transformData.World = XMMatrixTranspose(matrixTransform);
        sm->SetConstantBuffer<TransformBuffer>("TransformBuffer", transformData);

        m_immediateContext->PSSetShaderResources(0, 1, &m_crateMaterialSRV);

        MeshData meshData = MeshLoader::GetMesh(mesh->MeshId);

        UINT stride = meshData.VBStride;
        UINT offset = meshData.VBOffset;

        m_immediateContext->IASetVertexBuffers(0, 1, &meshData.VertexBuffer, &stride, &offset);
        m_immediateContext->IASetIndexBuffer(meshData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

        m_immediateContext->DrawIndexed(meshData.IndexCount, 0, 0);
    });

    m_terrain->Draw(m_immediateContext);

    ////////// DEBUG BOX RENDERING ////////////

    sm->SetActiveShader("DebugBoxShaders");
    m_immediateContext->RSSetState(m_wireframeRasterizerState);

    MeshData cubeMeshData = MeshLoader::GetMesh("Cube");
    MeshData sphereMeshData = MeshLoader::GetMesh("Sphere");

    //XMFLOAT3 camPosDX = m_camera->GetPosition();
    //Vector3 camPos = Vector3(camPosDX.x, camPosDX.y, camPosDX.z);
    //for (const Node& node : m_aabbTree.GetNodes())
    //{
    //    Vector3 boxPos = node.box.getPosition();
    //    if (node.isLeaf && (boxPos - camPos).magnitude() <= 10.0f)// m_aabbTree.GetNode(node.child1).isLeaf || m_aabbTree.GetNode(node.child2).isLeaf)
    //    {
    //        Vector3 boxSize = node.box.getSize();

    //        XMMATRIX transform = XMMatrixScaling(boxSize.x, boxSize.y, boxSize.z) * XMMatrixTranslation(boxPos.x, boxPos.y, boxPos.z);

    //        transformData.World = XMMatrixTranspose(transform);
    //        sm->SetConstantBuffer<TransformBuffer>("TransformBuffer", transformData);

    //        UINT stride = cubeMeshData.VBStride;
    //        UINT offset = cubeMeshData.VBOffset;

    //        m_immediateContext->IASetVertexBuffers(0, 1, &cubeMeshData.VertexBuffer, &stride, &offset);
    //        m_immediateContext->IASetIndexBuffer(cubeMeshData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    //        m_immediateContext->DrawIndexed(cubeMeshData.IndexCount, 0, 0);
    //    }
    //}

    if (m_showDebugPoints)
    {
        for (const Vector3& point : m_debugPoints)
        {
            XMMATRIX transform = XMMatrixScaling(0.1f, 0.1f, 0.1f) * XMMatrixTranslation(point.x, point.y, point.z);
            transformData.World = XMMatrixTranspose(transform);
            sm->SetConstantBuffer<TransformBuffer>("TransformBuffer", transformData);

            UINT stride = sphereMeshData.VBStride;
            UINT offset = sphereMeshData.VBOffset;

            m_immediateContext->IASetVertexBuffers(0, 1, &sphereMeshData.VertexBuffer, &stride, &offset);
            m_immediateContext->IASetIndexBuffer(sphereMeshData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

            m_immediateContext->DrawIndexed(sphereMeshData.IndexCount, 0, 0);
        }
    }

    // draw springs
    transformData.World = XMMatrixTranspose(XMMatrixIdentity());
    sm->SetConstantBuffer<TransformBuffer>("TransformBuffer", transformData);

    std::vector<SimpleVertex> springVertices;

    m_scene.ForEach<Spring>([&](Entity entity, Spring* spring) {

        Vector3 start = m_scene.GetComponent<Transform>(spring->Entity1)->Position;
        Vector3 end = m_scene.GetComponent<Transform>(spring->Entity2)->Position;

        springVertices.push_back({ XMFLOAT3(start.x, start.y, start.z), XMFLOAT3(), XMFLOAT2(), XMFLOAT4() });
        springVertices.push_back({ XMFLOAT3(end.x, end.y, end.z), XMFLOAT3(), XMFLOAT2(), XMFLOAT4() });
    });

    UINT vertexCount = static_cast<UINT>(springVertices.size());

    // Update the dynamic vertex buffer
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    m_immediateContext->Map(m_springVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    memcpy(mappedResource.pData, springVertices.data(), vertexCount * sizeof(SimpleVertex));
    m_immediateContext->Unmap(m_springVertexBuffer, 0);

    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    m_immediateContext->IASetVertexBuffers(0, 1, &m_springVertexBuffer, &stride, &offset);
    m_immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    m_immediateContext->Draw(vertexCount, 0);

    //////// SKYBOX RENDERING ////////
    ID3D11DepthStencilState* pOldDepthState = nullptr;
    UINT oldStencilRef;
    m_immediateContext->OMGetDepthStencilState(&pOldDepthState, &oldStencilRef);

    m_immediateContext->RSSetState(m_defaultRasterizerState);
    m_immediateContext->OMSetDepthStencilState(m_skyboxDepthState, 1);
    MeshData skyboxMeshData = MeshLoader::GetMesh("InvertedCube");

    sm->SetActiveShader("SkyboxShader");

    stride = skyboxMeshData.VBStride;
    offset = skyboxMeshData.VBOffset;

    m_immediateContext->IASetVertexBuffers(0, 1, &skyboxMeshData.VertexBuffer, &stride, &offset);
    m_immediateContext->IASetIndexBuffer(skyboxMeshData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_immediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_immediateContext->PSSetShaderResources(0, 1, &m_skyboxSRV);

    m_immediateContext->DrawIndexed(skyboxMeshData.IndexCount, 0, 0);

    m_immediateContext->OMSetDepthStencilState(pOldDepthState, oldStencilRef);

    // Release the old depth state reference to avoid memory leak
    if (pOldDepthState) pOldDepthState->Release();

    // draw imgui
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    // present the backbuffer to the screen
    m_swapChain->Present(1, 0);
}

void DX11App::OnMouseMove(WPARAM btnState, int x, int y)
{
    // check if the right mouse button is being held down
    if ((btnState & MK_RBUTTON) != 0)
    {
        // get the delta movement of the mouse since last call of this method
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - m_lastMousePos.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - m_lastMousePos.y));

        // rotate the camera using the delta rotation calculated
        m_camera->Rotate(dy, dx);
    }
	
    // store the mouse position so that delta movement can be calculated
    m_lastMousePos.x = x;
    m_lastMousePos.y = y;
}

void DX11App::OnMouseClick(WPARAM pressedBtn, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    // check if the left mouse button was the button that was clicked
    if ((pressedBtn & MK_LBUTTON) != 0)
    {
        // build a ray from the current mouse position
        Ray ray = GetRayFromScreenPosition(x, y);

        float intersectDistance;
        Entity entity = m_aabbTree.Intersect(ray, intersectDistance);
        
        if (m_currentClickAction == ClickAction::SELECT)
        {
            m_selectedEntity = entity;
        }
        else if (m_currentClickAction == ClickAction::PUSH && entity != INVALID_ENTITY)
        {
            Vector3 force = ray.GetDirection() * 2.5f;
            if (m_scene.HasComponent<Particle>(entity))
            {
                m_scene.GetComponent<Particle>(entity)->ApplyLinearImpulse(force);
            }
            if (m_scene.HasComponent<RigidBody>(entity))
            {
                Vector3 intersectionPosition = ray.GetOrigin() + ray.GetDirection() * intersectDistance;
                Vector3 localPosition = intersectionPosition - m_scene.GetComponent<Transform>(entity)->Position;
                Vector3 torque = Vector3::Cross(localPosition, force);

                m_scene.GetComponent<RigidBody>(entity)->ApplyAngularImpuse(torque);
            }
        }
        else if (m_currentClickAction == ClickAction::DRAG)
        {
            m_draggingEntity = entity;
            if (entity != INVALID_ENTITY && m_scene.HasComponent<Transform>(entity))
            {
                m_draggingDistance = (m_scene.GetComponent<Transform>(entity)->Position - ray.GetOrigin()).magnitude();
            }
        }
    }
}

void DX11App::OnMouseRelease(WPARAM releasedBtn, int x, int y)
{
    m_draggingEntity = INVALID_ENTITY;
}

void DX11App::OnMouseWheel(WPARAM wheelDelta, int x, int y)
{
    // move the camera forwards or backwards based on the scroll wheel movement.
    m_camera->Move(XMFLOAT3(0, 0, (int)wheelDelta * 0.02f));
}
