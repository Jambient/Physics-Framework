#include "Terrain.h"
#include "Material.h"
#include <fstream>

Terrain::~Terrain()
{
}

bool Terrain::Init(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& heightMapFile, int fileWidth, int fileHeight, int terrainWidth, int terrainDepth, int terrainScale)
{
	if (!loadHeightMap(heightMapFile, fileWidth, fileHeight, terrainScale))
	{
		return false;
	}

	// smooth height map
	/*for (int x = 0; x < fileWidth; x++)
	{
		for (int y = 0; y < fileHeight; y++)
		{
			SetHeightAt(x, y, Average(x, y));
		}
	}*/

	createGrid(terrainWidth, terrainDepth, fileWidth, fileHeight);

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(SimpleVertex) * m_vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &m_vertices[0];
	device->CreateBuffer(&vbd, &vinitData, &m_vertexBuffer);

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * m_indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &m_indices[0];
	device->CreateBuffer(&ibd, &iinitData, &m_indexBuffer);

	// create materials
	Material* rockMaterial = new Material();
	rockMaterial->LoadTexture(device, TextureType::ALBEDO, L"Textures\\PBR\\Rock\\COLOR.dds");
	rockMaterial->LoadTexture(device, TextureType::NORMAL, L"Textures\\PBR\\Rock\\NORMAL.dds");
	rockMaterial->LoadTexture(device, TextureType::AMBIENT, L"Textures\\PBR\\Rock\\AO.dds");
	rockMaterial->LoadTexture(device, TextureType::ROUGHNESS, L"Textures\\PBR\\Rock\\ROUGHNESS.dds");
	rockMaterial->LoadTexture(device, TextureType::METALLIC, L"Textures\\PBR\\Rock\\METALLIC.dds");
	rockMaterial->LoadTexture(device, TextureType::HEIGHT, L"Textures\\PBR\\Rock\\HEIGHT.dds");

	m_rockMaterialSRV = rockMaterial->GetMaterialAsTextureArray(device, context);

	Material* grassMaterial = new Material();
	grassMaterial->LoadTexture(device, TextureType::ALBEDO, L"Textures\\PBR\\Grass\\COLOR.dds");
	grassMaterial->LoadTexture(device, TextureType::NORMAL, L"Textures\\PBR\\Grass\\NORMAL.dds");
	grassMaterial->LoadTexture(device, TextureType::AMBIENT, L"Textures\\PBR\\Grass\\AO.dds");
	grassMaterial->LoadTexture(device, TextureType::ROUGHNESS, L"Textures\\PBR\\Grass\\ROUGHNESS.dds");
	grassMaterial->LoadTexture(device, TextureType::METALLIC, L"Textures\\PBR\\Grass\\METALLIC.dds");
	grassMaterial->LoadTexture(device, TextureType::HEIGHT, L"Textures\\PBR\\Grass\\HEIGHT.dds");

	m_grassMaterialSRV = grassMaterial->GetMaterialAsTextureArray(device, context);

    return true;
}

void Terrain::createGrid(int terrainWidth, int terrainDepth, int fileWidth, int fileHeight)
{
	UINT vertexCount = fileWidth * fileHeight;
	UINT faceCount = (fileWidth - 1) * (fileHeight - 1) * 2;

	// create the vertices
	float halfWidth = 0.5f * terrainWidth;
	float halfDepth = 0.5f * terrainDepth;

	float dx = (float)terrainWidth / (fileWidth - 1);
	float dz = (float)terrainDepth / (fileHeight - 1);

	m_vertices.resize(vertexCount);
	for (UINT i = 0; i < fileHeight; ++i)
	{
		float z = halfDepth - i * dz;
		for (UINT j = 0; j < fileWidth; ++j)
		{
			float x = -halfWidth + j * dx;
			UINT index = i * fileHeight + j;

			// Set position
			float mapHeight = getHeightAt(i, j);
			m_vertices[index].Pos = XMFLOAT3(x, mapHeight, z);

			// set normal
			XMFLOAT3 normal(0.0f, 0.0f, 0.0f);
			float heightL = getHeightAt(j - 1, i);
			float heightR = getHeightAt(j + 1, i);
			float heightU = getHeightAt(j, i + 1);
			float heightD = getHeightAt(j, i - 1);

			XMVECTOR tangentVec = XMVectorSet(2.0f * dx, heightL - heightR, 0.0f, 0.0f);
			XMVECTOR bitangentVec = XMVectorSet(0.0f, heightU - heightD, 2.0f * dz, 0.0f);
			XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(tangentVec, bitangentVec)));

			m_vertices[index].Normal = normal;

			// Compute and set tangent
			XMFLOAT3 tangent;
			XMStoreFloat3(&tangent, tangentVec);
			m_vertices[index].Tangent = XMFLOAT4(tangent.x, tangent.y, tangent.z, 1.0f);

			// Set texture coordinates
			m_vertices[index].TexC.x = j * dx;
			m_vertices[index].TexC.y = i * dz;
		}
	}

	m_indices.resize(faceCount * 3);
	UINT k = 0;
	for (UINT i = 0; i < fileHeight - 1; i++)
	{
		for (UINT j = 0; j < fileWidth - 1; j++)
		{
			m_indices[k] = i * fileWidth + j;
			m_indices[k + 1] = i * fileWidth + j + 1;
			m_indices[k + 2] = (i + 1) * fileWidth + j;

			m_indices[k + 3] = (i + 1) * fileWidth + j;
			m_indices[k + 4] = i * fileWidth + j + 1;
			m_indices[k + 5] = (i + 1) * fileWidth + j + 1;

			k += 6;
		}
	}
}

bool Terrain::loadHeightMap(const std::string& filePath, int fileWidth, int fileHeight, int terrainScale)
{
	int imageSize = fileWidth * fileHeight;
	std::vector<unsigned char> in(imageSize);

	std::ifstream inFile;
	inFile.open(filePath.c_str(), std::ios_base::binary);

	if (inFile)
	{
		inFile.read((char*)&in[0], (std::streamsize)in.size());
		inFile.close();
	}
	else
	{
		return false;
	}

	m_heightData.resize(imageSize);
	for (unsigned int i = 0; i < imageSize; i++)
	{
		m_heightData[i] = (in[i] / 255.0f) * terrainScale;
	}

	return true;
}

float Terrain::getHeightAt(int x, int y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= m_heightMapWidth) x = m_heightMapWidth - 1;
	if (y >= m_heightMapDepth) y = m_heightMapDepth - 1;

	return m_heightData[x * m_heightMapWidth + y];
}
