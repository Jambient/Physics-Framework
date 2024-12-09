#pragma once
#include <string>
#include <unordered_map>
#include <d3d11_4.h>
#include <vector>

struct Shader
{
	ID3D11VertexShader* VertexShader;
	ID3DBlob* VSBlob;
	ID3D11PixelShader* PixelShader;
	ID3D11InputLayout* InputLayout;
	std::vector<std::string> Buffers;
};

class ShaderManager
{
public:
	ShaderManager(ShaderManager& other) = delete;
	void operator=(const ShaderManager&) = delete;
	static ShaderManager* GetInstance();
	static void ReleaseInstance();

	void Init(ID3D11Device* device, ID3D11DeviceContext* context);

	HRESULT LoadVertexShader(HWND windowHandle, const std::string& shaderName, const std::wstring& filePath, const std::string& entryPoint = "VS_main");
	HRESULT LoadPixelShader(HWND windowHandle, const std::string& shaderName, const std::wstring& filePath, const std::string& entryPoint = "PS_main");
	HRESULT CreateInputLayout(const std::string& shaderName, const D3D11_INPUT_ELEMENT_DESC* inputElementDesc, UINT numElements);

	void SetActiveShader(const std::string& shaderName);

	template<typename T>
	HRESULT CreateConstantBuffer(const std::string& bufferName);

	template<typename T>
	void SetConstantBuffer(const std::string& bufferName, T& bufferData);

	template<typename T>
	bool GetConstantBufferData(const std::string& bufferName, T& outputData);

	void RegisterConstantBuffer(const std::string& shaderName, const std::string& bufferName, UINT slot);

private:
	ShaderManager();
	~ShaderManager();
	static ShaderManager* _instance;

	ID3D11Device* _device;
	ID3D11DeviceContext* _context;

	std::unordered_map<std::string, Shader> _shaders;
	std::unordered_map<std::string, ID3D11Buffer*> _buffers;
	std::unordered_map<std::string, void*> _bufferData;
};

template<typename T>
inline HRESULT ShaderManager::CreateConstantBuffer(const std::string& bufferName)
{
	D3D11_BUFFER_DESC constantBufferDesc = {};
	constantBufferDesc.ByteWidth = sizeof(T);
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* buffer;
	HRESULT hr = _device->CreateBuffer(&constantBufferDesc, nullptr, &buffer);
	if (FAILED(hr)) { return hr; }

	_buffers[bufferName] = buffer;

	return S_OK;
}

template<typename T>
inline void ShaderManager::SetConstantBuffer(const std::string& bufferName, T& bufferData)
{
	auto bufferIt = _buffers.find(bufferName);
	if (bufferIt == _buffers.end()) {
		return; // Buffer not found
	}

	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	_context->Map(_buffers[bufferName], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
	memcpy(mappedSubresource.pData, &bufferData, sizeof(bufferData));
	_context->Unmap(_buffers[bufferName], 0);

	auto dataIt = _bufferData.find(bufferName);
	if (dataIt != _bufferData.end()) {
		delete[] static_cast<char*>(dataIt->second);
	}
	void* newData = new char[sizeof(T)];
	memcpy(newData, &bufferData, sizeof(T));
	_bufferData[bufferName] = newData;
}

template<typename T>
inline bool ShaderManager::GetConstantBufferData(const std::string& bufferName, T& outputData)
{
	auto it = _bufferData.find(bufferName);
	if (it == _bufferData.end()) {
		return false; // Data not found
	}

	// Copy the raw data into the output buffer
	memcpy(&outputData, it->second, sizeof(T));
	return true;
}
