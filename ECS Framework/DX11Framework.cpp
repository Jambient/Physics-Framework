#include "DX11Framework.h"
#include "Structures.h"
#include <string>

HRESULT DX11Framework::Initialise(HWND windowHandle)
{
    // set the default hresult value
    HRESULT hr = S_OK;

    // store the handle for later use
    m_windowHandle = windowHandle;

    // run the other initialise methods and check for fails
    hr = CreateD3DDevice();
    if (FAILED(hr)) return E_FAIL;

    hr = CreateSwapChainAndFrameBuffer();
    if (FAILED(hr)) return E_FAIL;

    hr = InitPipelineVariables();
    if (FAILED(hr)) return E_FAIL;

    hr = InitRunTimeData();
    if (FAILED(hr)) return E_FAIL;

    return hr;
}

HRESULT DX11Framework::CreateD3DDevice()
{
    // set the default hresult value
    HRESULT hr = S_OK;

    // set the targeted features
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
    };

    // create temporary base device and context variables so that the device and context pointers can be retrieved
    ID3D11Device* baseDevice;
    ID3D11DeviceContext* baseDeviceContext;

    // create the device
    DWORD createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | createDeviceFlags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &baseDevice, nullptr, &baseDeviceContext);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    hr = baseDevice->QueryInterface(__uuidof(ID3D11Device), reinterpret_cast<void**>(&m_device));
    hr = baseDeviceContext->QueryInterface(__uuidof(ID3D11DeviceContext), reinterpret_cast<void**>(&m_immediateContext));

    baseDevice->Release();
    baseDeviceContext->Release();

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // get the dxgi device
    hr = m_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&m_dxgiDevice));
    if (FAILED(hr)) return hr;

    // get the dxgi adapter and use it to retrieve the factory
    IDXGIAdapter* dxgiAdapter;
    hr = m_dxgiDevice->GetAdapter(&dxgiAdapter);
    hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&m_dxgiFactory));
    dxgiAdapter->Release();

    return S_OK;
}

HRESULT DX11Framework::CreateSwapChainAndFrameBuffer()
{
    // set the default hresult value
    HRESULT hr = S_OK;

    // create the description for the swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; // Defer to WindowWidth
    swapChainDesc.Height = 0; // Defer to WindowHeight
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //FLIP* modes don't support sRGB backbuffer
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    // build the swap chain
    hr = m_dxgiFactory->CreateSwapChainForHwnd(m_device, m_windowHandle, &swapChainDesc, nullptr, nullptr, &m_swapChain);
    if (FAILED(hr)) return hr;

    ///////////////////////////////////////////////////////////////////////////////////////////////

    // temporary frame buffer used to retrieve the frame buffer view and buffer description
    ID3D11Texture2D* frameBuffer = nullptr;

    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&frameBuffer));
    if (FAILED(hr)) return hr;

    D3D11_RENDER_TARGET_VIEW_DESC framebufferDesc = {};
    framebufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //sRGB render target enables hardware gamma correction
    framebufferDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    hr = m_device->CreateRenderTargetView(frameBuffer, &framebufferDesc, &m_frameBufferView);
    if (FAILED(hr)) return hr;

    D3D11_TEXTURE2D_DESC depthBufferDesc = {};
    frameBuffer->GetDesc(&depthBufferDesc);

    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = m_device->CreateTexture2D(&depthBufferDesc, nullptr, &m_depthStencilBuffer);
    if (FAILED(hr)) return hr;

    m_device->CreateDepthStencilView(m_depthStencilBuffer, nullptr, &m_depthStencilView);
    if (FAILED(hr)) return hr;

    frameBuffer->Release();

    return hr;
}

HRESULT DX11Framework::InitPipelineVariables()
{
    // create and set the viewport
    m_viewport = { 0.0f, 0.0f, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 0.0f, 1.0f };
    m_immediateContext->RSSetViewports(1, &m_viewport);

    return S_OK;
}

HRESULT DX11Framework::InitRunTimeData()
{
    // run the virtual initialise method so that the application can initialise
    HRESULT hr = Init();

    return hr;
}

DX11Framework::~DX11Framework()
{
    if (m_immediateContext)m_immediateContext->Release();
    if (m_device)m_device->Release();
    if (m_dxgiDevice)m_dxgiDevice->Release();
    if (m_dxgiFactory)m_dxgiFactory->Release();
    if (m_frameBufferView)m_frameBufferView->Release();
    if (m_swapChain)m_swapChain->Release();
    if (m_depthStencilBuffer)m_depthStencilBuffer->Release();
    if (m_depthStencilView)m_depthStencilView->Release();
}