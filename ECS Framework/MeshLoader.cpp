#include "MeshLoader.h"

std::vector<MeshData> MeshLoader::m_meshCache = {};
std::unordered_map<std::string, int> MeshLoader::m_nameToID = {};
int MeshLoader::m_loadedMeshCount = 0;

int MeshLoader::LoadMesh(const std::string& filePath, const std::string& nameIdentifier, ID3D11Device* device, bool invertTexCoords)
{
    m_meshCache.push_back(OBJLoader::Load(filePath.c_str(), device, invertTexCoords));
    m_nameToID[nameIdentifier] = m_loadedMeshCount++;
    return m_loadedMeshCount - 1;
}

MeshData MeshLoader::GetMesh(int meshID)
{
    if (meshID >= m_loadedMeshCount)
    {
        throw std::runtime_error("Invalid mesh ID");
    }

    return m_meshCache[meshID];
}

MeshData MeshLoader::GetMesh(const std::string& nameIdentifier)
{
    return m_meshCache[GetMeshID(nameIdentifier)];
}

int MeshLoader::GetMeshID(const std::string& nameIdentifier)
{
    if (m_nameToID.find(nameIdentifier) == m_nameToID.end())
    {
        throw std::runtime_error("Invalid mesh name");
    }

    return m_nameToID[nameIdentifier];
}
