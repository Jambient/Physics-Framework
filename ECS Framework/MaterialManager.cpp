#include "MaterialManager.h"
#include <stdexcept>

std::vector<ID3D11ShaderResourceView*> MaterialManager::m_materialSRVCache = {};
std::unordered_map<std::string, int> MaterialManager::m_nameToID = {};
int MaterialManager::m_loadedMaterialCount = 0;

int MaterialManager::AddMaterialSRV(const std::string& nameIdentifier, ID3D11ShaderResourceView* srv)
{
    m_materialSRVCache.push_back(srv);
    m_nameToID[nameIdentifier] = m_loadedMaterialCount++;
    return m_loadedMaterialCount - 1;
}

ID3D11ShaderResourceView* MaterialManager::GetMaterialSRV(int meshID)
{
    if (meshID >= m_loadedMaterialCount)
    {
        throw std::runtime_error("Invalid mesh ID");
    }

    return m_materialSRVCache[meshID];
}

ID3D11ShaderResourceView* MaterialManager::GetMaterialSRV(const std::string& nameIdentifier)
{
    return m_materialSRVCache[GetMaterialID(nameIdentifier)];
}

int MaterialManager::GetMaterialID(const std::string& nameIdentifier)
{
    if (m_nameToID.find(nameIdentifier) == m_nameToID.end())
    {
        throw std::runtime_error("Invalid mesh name");
    }

    return m_nameToID[nameIdentifier];
}
