#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <d3d11_4.h>
#include "OBJLoader.h"

class MeshLoader
{
public:
	static int LoadMesh(const std::string& filePath, const std::string& nameIdentifier, ID3D11Device* device, bool invertTexCoords);
	static MeshData GetMesh(int meshID);
	static MeshData GetMesh(const std::string& nameIdentifier);
	static int GetMeshID(const std::string& nameIdentifier);

private:
	static std::vector<MeshData> m_meshCache;
	static std::unordered_map<std::string, int> m_nameToID;
	static int m_loadedMeshCount;
};

