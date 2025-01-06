#include "DX11App.h"
#include "ShaderManager.h"
#include "Components.h"
#include "PhysicsSystem.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <iostream>
#include <stdexcept>
#include <random>
#include <sstream>

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
        { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
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

    // debug shader:
    hr = sm->LoadVertexShader(m_windowHandle, "DebugBoxShaders", L"DebugBoxShaders.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->LoadPixelShader(m_windowHandle, "DebugBoxShaders", L"DebugBoxShaders.hlsl");
    if (FAILED(hr)) { return hr; }
    hr = sm->CreateInputLayout("DebugBoxShaders", defaultInputLayout, ARRAYSIZE(defaultInputLayout));
    if (FAILED(hr)) { return hr; }

    sm->RegisterConstantBuffer("DebugBoxShaders", "TransformBuffer", 0);

    // store the default ambient light into the global buffer
    m_globalBufferData.AmbientLight = XMFLOAT4(0.05f, 0.05f, 0.05f, 1.0f);

    // create camera
    float aspect = m_viewport.Width / m_viewport.Height;

    m_camera = new Camera();
    m_camera->SetLens(XMConvertToRadians(90), aspect, 0.1f, 500.0f);
    m_camera->SetPosition(XMFLOAT3(0.0f, 0.0f, -5.0f));

    // create lights
    Light directionalLight;
    directionalLight.Type = LightType::DIRECTIONAL;
    directionalLight.Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
    directionalLight.Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);

    m_lightBufferData.Lights[0] = directionalLight;
    m_lightBufferData.LightCount = 1;

    // initialise and build the scene
    m_scene.Init();

    m_scene.RegisterComponent<Particle>();
    m_scene.RegisterComponent<Gravity>();
    m_scene.RegisterComponent<RigidBody>();
    m_scene.RegisterComponent<Transform>();

    m_physicsSystem = m_scene.RegisterSystem<PhysicsSystem>();
    m_physicsSystem->m_scene = &m_scene;

    /*Entity e1 = m_scene.CreateEntity();
    m_scene.AddComponent(e1, Gravity{ Vector3::Down * 9.8f });
    m_scene.AddComponent(e1, RigidBody{ Vector3::One, Vector3::One });
    m_scene.AddComponent(e1, Transform{ Vector3::Left, Vector3::Zero, Vector3::Right });*/

    Signature objectSignature;
    objectSignature.set(m_scene.GetComponentType<Gravity>());
    objectSignature.set(m_scene.GetComponentType<RigidBody>());
    objectSignature.set(m_scene.GetComponentType<Transform>());
    m_scene.SetSystemSignature<PhysicsSystem>(objectSignature);

    std::vector<Entity> entities(MAX_ENTITIES);

    /*std::default_random_engine generator;
    std::uniform_real_distribution<float> velocityAxisOffset(-1.0f, 1.0f);

    for (Entity entity : entities)
    {
        entity = m_scene.CreateEntity();

        Vector3 velocity = Vector3(velocityAxisOffset(generator), 5.0f, velocityAxisOffset(generator)).normalized() * 5.0f;

        m_scene.AddComponent(
            entity,
            Particle{ Vector3::Zero, velocity, Vector3::Down * 9.8f, 0.99f, 1 / 2.0f }
        );
    }*/

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

    m_aabbTree.InsertLeaf(e1, AABB(Vector3::Left * 2.0f, Vector3::One));
    m_aabbTree.InsertLeaf(e2, AABB(Vector3::Right * 2.0f, Vector3::One));

    m_aabbTree.PrintTree(m_aabbTree.GetRootIndex());*/

    for (Entity entity : entities)
    {
        entity = m_scene.CreateEntity();
        Vector3 objectPosition = Vector3(randPosition(generator), randPosition(generator), randPosition(generator));
        Vector3 objectVelocity = Vector3(randVelocity(generator), randVelocity(generator), randVelocity(generator));

        m_scene.AddComponent(
            entity,
            Particle{ objectPosition, objectVelocity, Vector3::Zero, 0.99f, 1 / 2.0f}
        );

        m_aabbTree.InsertLeaf(entity, AABB::FromPositionScale(objectPosition, Vector3::One));

        //std::cout << "Adding entity " << entity << " to AABB tree" << std::endl;

        /*m_scene.AddComponent(
            entity, 
            Gravity{ Vector3(0.0f, randGravity(generator), 0.0f) });

        m_scene.AddComponent(
            entity,
            RigidBody{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } });

        Vector3 objectPosition = Vector3(randPosition(generator), randPosition(generator), randPosition(generator));
        Vector3 objectRotation = Vector3(randRotation(generator), randRotation(generator), randRotation(generator));
        Vector3 objectScale = Vector3(randScale(generator), randScale(generator), randScale(generator));

        m_scene.AddComponent(
            entity,
            Transform{ objectPosition, objectRotation, objectScale });*/
    }

    //m_aabbTree.PrintTree(m_aabbTree.GetRootIndex());
    m_instanceData.resize(MAX_ENTITIES);

    // create instance buffer
    D3D11_BUFFER_DESC instanceBufferDesc = {};
    instanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC; // Allows updating the buffer
    instanceBufferDesc.ByteWidth = sizeof(InstanceData) * MAX_ENTITIES;
    instanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA instanceDataResource = {};
    instanceDataResource.pSysMem = m_instanceData.data();

    m_device->CreateBuffer(&instanceBufferDesc, &instanceDataResource, &m_instanceBuffer);

    // load cube mesh
    m_cubeMeshData = OBJLoader::Load("Models/Cube.obj", m_device, false);

    // initialise ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(m_windowHandle);
    ImGui_ImplDX11_Init(m_device, m_immediateContext);

    return hr;
}

void DX11App::Update()
{
    double deltaTime = m_timer.GetDeltaTime();

    // update window text
    m_timeSinceLastTitleUpdate += deltaTime;
    m_framesSinceLastTitleUpdate++;

    // Update FPS once every second
    if (m_timeSinceLastTitleUpdate >= 1.0f) {
        float avgFrameTime = m_timeSinceLastTitleUpdate / m_framesSinceLastTitleUpdate;
        float fps = 1.0f / avgFrameTime;

        // Create a new title
        std::wostringstream oss;
        oss.precision(2);
        oss << std::fixed;
        oss << L"ECS Framework - FPS: " << fps
            << L" - MS TIME: " << avgFrameTime * 1000;

        // Update window title
        SetWindowText(m_windowHandle, oss.str().c_str());

        // Reset counters but preserve leftover time
        m_timeSinceLastTitleUpdate -= 1.0f;
        m_framesSinceLastTitleUpdate = 0;
    }

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
        m_physicsSystem->Update(FPS60);
        m_physicsAccumulator -= FPS60;

        //std::cout << "Deepest Tree Level: " << m_aabbTree.GetDeepestLevel() << std::endl;
        // also need to have individual elapsed time for system
    }

    // update instance data
    /*m_scene.ForEach<Transform>([&](Entity entity, Transform* transform) {
        m_instanceData[entity].Position = transform->Position;
        m_instanceData[entity].Scale = transform->Scale;
    });*/

    m_scene.ForEach<Particle>([&](Entity entity, Particle* particle) {
        m_instanceData[entity].Position = particle->Position;
        m_instanceData[entity].Scale = Vector3::One;

        m_aabbTree.Update(entity, AABB::FromPositionScale(particle->Position, Vector3::One));
    });

    for (Entity entity = 0; entity < MAX_ENTITIES; entity++)
    {
        m_instanceData[entity].Color = Vector3(0.05f, 0.05f, 0.05f);
    }

    /*for (const auto [entity1, entity2] : m_aabbTree.GetPotentialIntersections())
    {
        m_instanceData[entity1].Color = Vector3(1.0f, 0.2f, 0.2f);
        m_instanceData[entity2].Color = Vector3(1.0f, 0.2f, 0.2f);
    }*/

    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = m_immediateContext->Map(m_instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    ThrowIfFailed(hr);
    memcpy(mappedResource.pData, m_instanceData.data(), sizeof(InstanceData) * MAX_ENTITIES);
    m_immediateContext->Unmap(m_instanceBuffer, 0);

    // start imgui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

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

    // set the bilienar sampler state as the default sampler for shaders
    m_immediateContext->PSSetSamplers(0, 1, &m_defaultSamplerState);

    sm->SetActiveShader("SimpleShaders");

    // draw cube
    UINT strides[] = { m_cubeMeshData.VBStride, sizeof(InstanceData) };
    UINT offsets[] = { m_cubeMeshData.VBOffset, 0 };
    ID3D11Buffer* buffers[] = { m_cubeMeshData.VertexBuffer, m_instanceBuffer };

    m_immediateContext->IASetVertexBuffers(0, 2, buffers, strides, offsets);
    m_immediateContext->IASetIndexBuffer(m_cubeMeshData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    m_immediateContext->DrawInstanced(m_cubeMeshData.VerticesCount, MAX_ENTITIES, 0, 0);

    sm->SetActiveShader("DebugBoxShaders");
    m_immediateContext->RSSetState(m_wireframeRasterizerState);

    TransformBuffer transformData;
    sm->GetConstantBufferData<TransformBuffer>("TransformBuffer", transformData);

    //for (const Node& node : m_aabbTree.GetNodes())
    //{
    //    if (true)// m_aabbTree.GetNode(node.child1).isLeaf || m_aabbTree.GetNode(node.child2).isLeaf)
    //    {
    //        Vector3 boxPos = node.box.getPosition();
    //        Vector3 boxSize = node.box.getSize();

    //        XMMATRIX transform = XMMatrixScaling(boxSize.x, boxSize.y, boxSize.z) * XMMatrixTranslation(boxPos.x, boxPos.y, boxPos.z);

    //        transformData.World = XMMatrixTranspose(transform);
    //        sm->SetConstantBuffer<TransformBuffer>("TransformBuffer", transformData);

    //        UINT stride = m_cubeMeshData.VBStride;
    //        UINT offset = m_cubeMeshData.VBOffset;

    //        m_immediateContext->IASetVertexBuffers(0, 1, &m_cubeMeshData.VertexBuffer, &stride, &offset);
    //        m_immediateContext->IASetIndexBuffer(m_cubeMeshData.IndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    //        m_immediateContext->DrawIndexed(m_cubeMeshData.IndexCount, 0, 0);
    //    }
    //}

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
    // check if the left mouse button was the button that was clicked
    if ((pressedBtn & MK_LBUTTON) != 0)
    {
        // build a ray from the current mouse position
        Ray ray = GetRayFromScreenPosition(x, y);

        float intersectDistance;
        Entity entity = m_aabbTree.Intersect(ray, intersectDistance);

        if (entity != INVALID_ENTITY)
        {
            std::cout << "Clicked on entity with ID " << entity << " and distance " << intersectDistance << std::endl;
        }
    }
}

void DX11App::OnMouseWheel(WPARAM wheelDelta, int x, int y)
{
    // move the camera forwards or backwards based on the scroll wheel movement.
    m_camera->Move(XMFLOAT3(0, 0, (int)wheelDelta * 0.02f));
}
