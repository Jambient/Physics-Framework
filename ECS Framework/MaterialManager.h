#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <d3d11_4.h>
#include "Material.h"

class MaterialManager
{
public:
	static int AddMaterialSRV(const std::string& nameIdentifier, ID3D11ShaderResourceView* srv);
	static ID3D11ShaderResourceView* GetMaterialSRV(int meshID);
	static ID3D11ShaderResourceView* GetMaterialSRV(const std::string& nameIdentifier);
	static int GetMaterialID(const std::string& nameIdentifier);

private:
	static std::vector<ID3D11ShaderResourceView*> m_materialSRVCache;
	static std::unordered_map<std::string, int> m_nameToID;
	static int m_loadedMaterialCount;
};

