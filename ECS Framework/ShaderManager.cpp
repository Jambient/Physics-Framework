#include "ShaderManager.h"
#include <d3dcompiler.h>

ShaderManager* ShaderManager::_instance = nullptr;

ShaderManager* ShaderManager::GetInstance()
{
    if (_instance == nullptr) {
        _instance = new ShaderManager();
    }
    return _instance;
}

void ShaderManager::ReleaseInstance()
{
    delete _instance;
    _instance = nullptr;
}

void ShaderManager::Init(ID3D11Device* device, ID3D11DeviceContext* context)
{
    _device = device;
    _context = context;
}

HRESULT ShaderManager::LoadVertexShader(HWND windowHandle, const std::string& shaderName, const std::wstring& filePath, const std::string& entryPoint)
{
    HRESULT hr = S_OK;
    ID3DBlob* errorBlob;
    ID3DBlob* vsBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    hr = D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), "vs_5_0", dwShaderFlags, 0, &vsBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    ID3D11VertexShader* vertexShader;
    hr = _device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vertexShader);
    if (FAILED(hr)) return hr;

    _shaders[shaderName].VertexShader = vertexShader;
    _shaders[shaderName].VSBlob = vsBlob;

    return hr;
}

HRESULT ShaderManager::LoadPixelShader(HWND windowHandle, const std::string& shaderName, const std::wstring& filePath, const std::string& entryPoint)
{
    HRESULT hr = S_OK;
    ID3DBlob* errorBlob;
    ID3DBlob* psBlob;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

    hr = D3DCompileFromFile(filePath.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint.c_str(), "ps_5_0", dwShaderFlags, 0, &psBlob, &errorBlob);
    if (FAILED(hr))
    {
        MessageBoxA(windowHandle, (char*)errorBlob->GetBufferPointer(), nullptr, ERROR);
        errorBlob->Release();
        return hr;
    }

    ID3D11PixelShader* pixelShader;
    hr = _device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pixelShader);
    if (FAILED(hr)) return hr;

    psBlob->Release();

    _shaders[shaderName].PixelShader = pixelShader;

    return hr;
}

HRESULT ShaderManager::CreateInputLayout(const std::string& shaderName, const D3D11_INPUT_ELEMENT_DESC* inputElementDesc, UINT numElements)
{
    HRESULT hr = S_OK;
    ID3DBlob* vsBlob = _shaders[shaderName].VSBlob;

    hr = _device->CreateInputLayout(inputElementDesc, numElements, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &_shaders[shaderName].InputLayout);
    if (FAILED(hr)) return hr;

    return hr;
}

void ShaderManager::SetActiveShader(const std::string& shaderName)
{
    Shader& shader = _shaders[shaderName];
    _context->IASetInputLayout(shader.InputLayout);

    // set constant buffers
    for (UINT i = 0; i < shader.Buffers.size(); i++)
    {
        std::string bufferName = shader.Buffers[i];
        _context->VSSetConstantBuffers(i, 1, &_buffers[bufferName]);
        _context->PSSetConstantBuffers(i, 1, &_buffers[bufferName]);
    }

    // set shaders
    _context->VSSetShader(shader.VertexShader, nullptr, 0);
    _context->PSSetShader(shader.PixelShader, nullptr, 0);
}

void ShaderManager::RegisterConstantBuffer(const std::string& shaderName, const std::string& bufferName, UINT slot)
{
    if (_shaders.find(shaderName) == _shaders.end()) {
        return;
    }

    Shader& shader = _shaders[shaderName];
    if (slot >= shader.Buffers.size()) {
        shader.Buffers.resize(slot + 1);
    }

    shader.Buffers[slot] = bufferName;
}

ShaderManager::ShaderManager()
{
    _device = nullptr;
    _context = nullptr;
}

ShaderManager::~ShaderManager()
{
    for (auto& kvp : _shaders) {
        if (kvp.second.VertexShader) {
            kvp.second.VertexShader->Release();
            kvp.second.VertexShader = nullptr;
        }
        if (kvp.second.VSBlob) {
            kvp.second.VSBlob->Release();
            kvp.second.VSBlob = nullptr;
        }
        if (kvp.second.PixelShader) {
            kvp.second.PixelShader->Release();
            kvp.second.PixelShader = nullptr;
        }
        if (kvp.second.InputLayout) {
            kvp.second.InputLayout->Release();
            kvp.second.InputLayout = nullptr;
        }
        kvp.second.Buffers.clear();
    }
    _shaders.clear();

    for (auto& kvp : _buffers) {
        if (kvp.second) {
            kvp.second->Release();
            kvp.second = nullptr;
        }
    }
    _buffers.clear();

    for (auto& kvp : _bufferData)
    {
        delete[] static_cast<char*>(kvp.second);
    }
}
