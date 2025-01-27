#include "Material.h"
#include "DDSTextureLoader.h"

Material::Material()
{
    for (int i = 0; i < 6; i++) {
        _textures[i] = nullptr;
    }
}

Material::~Material()
{
    for (int i = 0; i < 6; i++) {
        if (_textures[i] != nullptr)
        {
            _textures[i]->Release();
            _textures[i] = nullptr;
        }
    }
}

HRESULT Material::LoadTexture(ID3D11Device* device, TextureType type, const std::wstring& filePath)
{
    HRESULT hr = S_OK;

    ID3D11ShaderResourceView* texture;
    hr = DirectX::CreateDDSTextureFromFile(device, filePath.c_str(), nullptr, &texture);
    if (FAILED(hr)) { return hr; }

    SetTexture(type, texture);

    return hr;
}

void Material::SetTexture(TextureType type, ID3D11ShaderResourceView* resource)
{
	_textures[(unsigned int)type] = resource;
}

ID3D11ShaderResourceView* Material::GetTexture(TextureType type)
{
    return _textures[(unsigned int)type];
}

ID3D11ShaderResourceView* Material::GetMaterialAsTextureArray(ID3D11Device* device, ID3D11DeviceContext* context)
{
    // Validate textures
    int textureCount = sizeof(_textures) / sizeof(ID3D11ShaderResourceView*);
    if (textureCount == 0)
        return nullptr;

    // Retrieve the first texture description for reference
    ID3D11Resource* resource = nullptr;
    D3D11_TEXTURE2D_DESC textureDesc = {};
    _textures[0]->GetResource(&resource);
    if (!resource)
        return nullptr;

    // Get the texture details
    ID3D11Texture2D* texture2D = nullptr;
    HRESULT hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texture2D);
    if (FAILED(hr))
        return nullptr;

    texture2D->GetDesc(&textureDesc);
    texture2D->Release();
    resource->Release();

    // Modify the description for a Texture2DArray
    textureDesc.ArraySize = textureCount;   // Number of textures in the array
    textureDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
    textureDesc.MiscFlags = 0;

    // Create the Texture2DArray
    ID3D11Texture2D* textureArray = nullptr;
    hr = device->CreateTexture2D(&textureDesc, nullptr, &textureArray);
    if (FAILED(hr))
        return nullptr;

    for (int i = 0; i < textureCount; ++i)
    {
        if (_textures[i] != nullptr)
        {
            ID3D11Resource* srcResource = nullptr;
            _textures[i]->GetResource(&srcResource);

            for (UINT mipLevel = 0; mipLevel < textureDesc.MipLevels; ++mipLevel)
            {
                context->CopySubresourceRegion(
                    textureArray,
                    D3D11CalcSubresource(mipLevel, i, textureDesc.MipLevels), // MipLevel, ArraySlice
                    0, 0, 0,                                                 // Start at (0, 0, 0)
                    srcResource,
                    mipLevel,                                                // Source mip level
                    nullptr                                                  // Entire resource
                );
            }

            srcResource->Release();
        }
    }

    // Create a Shader Resource View for the Texture2DArray
    ID3D11ShaderResourceView* textureArraySRV = nullptr;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Texture2DArray.MostDetailedMip = 0;
    srvDesc.Texture2DArray.MipLevels = -1;
    srvDesc.Texture2DArray.FirstArraySlice = 0;
    srvDesc.Texture2DArray.ArraySize = textureCount;

    hr = device->CreateShaderResourceView(textureArray, &srvDesc, &textureArraySRV);
    textureArray->Release();

    if (FAILED(hr))
        return nullptr;

    return textureArraySRV;
}
