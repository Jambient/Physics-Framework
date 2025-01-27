#pragma once
#include <d3d11_4.h>
#include <string>

enum class TextureType
{
    ALBEDO,
    NORMAL,
    AMBIENT,
    ROUGHNESS,
    METALLIC,
    HEIGHT,
};

class Material
{
public:
	Material();
    ~Material();

    HRESULT LoadTexture(ID3D11Device* device, TextureType type, const std::wstring& filePath);
    void SetTexture(TextureType type, ID3D11ShaderResourceView* resource);
    ID3D11ShaderResourceView* GetTexture(TextureType type);
    ID3D11ShaderResourceView* GetMaterialAsTextureArray(ID3D11Device* device, ID3D11DeviceContext* context);

private:
    ID3D11ShaderResourceView* _textures[6];
};

