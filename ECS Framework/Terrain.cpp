#include "Terrain.h"
#include "Material.h"
#include "ShaderManager.h"
#include <fstream>

Terrain::~Terrain()
{
}

bool Terrain::Init(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& heightMapFile, int fileWidth, int fileHeight, int terrainWidth, int terrainDepth, int heightScale)
{
	if (!loadHeightMap(heightMapFile, fileWidth, fileHeight, heightScale))
	{
		return false;
	}

	m_heightMapWidth = terrainWidth;
	m_heightMapDepth = terrainDepth;

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
	rockMaterial->LoadTexture(device, TextureType::ALBEDO, L"Textures\\Materials\\Rock\\COLOR.dds");
	rockMaterial->LoadTexture(device, TextureType::NORMAL, L"Textures\\Materials\\Rock\\NORMAL.dds");
	rockMaterial->LoadTexture(device, TextureType::AMBIENT, L"Textures\\Materials\\Rock\\AO.dds");
	rockMaterial->LoadTexture(device, TextureType::ROUGHNESS, L"Textures\\Materials\\Rock\\ROUGHNESS.dds");
	rockMaterial->LoadTexture(device, TextureType::METALLIC, L"Textures\\Materials\\Rock\\METALLIC.dds");
	rockMaterial->LoadTexture(device, TextureType::HEIGHT, L"Textures\\Materials\\Rock\\HEIGHT.dds");

	m_rockMaterialSRV = rockMaterial->GetMaterialAsTextureArray(device, context);

	Material* grassMaterial = new Material();
	grassMaterial->LoadTexture(device, TextureType::ALBEDO, L"Textures\\Materials\\Grass\\COLOR.dds");
	grassMaterial->LoadTexture(device, TextureType::NORMAL, L"Textures\\Materials\\Grass\\NORMAL.dds");
	grassMaterial->LoadTexture(device, TextureType::AMBIENT, L"Textures\\Materials\\Grass\\AO.dds");
	grassMaterial->LoadTexture(device, TextureType::ROUGHNESS, L"Textures\\Materials\\Grass\\ROUGHNESS.dds");
	grassMaterial->LoadTexture(device, TextureType::METALLIC, L"Textures\\Materials\\Grass\\METALLIC.dds");
	grassMaterial->LoadTexture(device, TextureType::HEIGHT, L"Textures\\Materials\\Grass\\HEIGHT.dds");

	m_grassMaterialSRV = grassMaterial->GetMaterialAsTextureArray(device, context);

    return true;
}

void Terrain::BuildCollision(ECSScene* scene)
{

}

void Terrain::Draw(ID3D11DeviceContext* context)
{
	ShaderManager* sm = ShaderManager::GetInstance();
	TransformBuffer transformBuffer;
	
	if (sm->GetConstantBufferData<TransformBuffer>("TransformBuffer", transformBuffer))
	{
		transformBuffer.World = XMMatrixTranspose(XMMatrixIdentity());
		sm->SetConstantBuffer<TransformBuffer>("TransformBuffer", transformBuffer);
	}

	context->PSSetShaderResources(0, 1, &m_grassMaterialSRV);
	context->PSSetShaderResources(1, 1, &m_rockMaterialSRV);
	ShaderManager::GetInstance()->SetActiveShader("TerrainShader");

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;

	context->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

	context->DrawIndexed(m_indices.size(), 0, 0);
}

void Terrain::createGrid(int terrainWidth, int terrainDepth, int fileWidth, int fileHeight)
{
	// create the vertices
	float halfWidth = 0.5f * terrainWidth;
	float halfDepth = 0.5f * terrainDepth;

	float dx = (float)terrainWidth / (fileWidth - 1);
	float dz = (float)terrainDepth / (fileHeight - 1);

	m_vertices.resize(fileWidth * fileHeight);
	for (UINT y = 0; y < fileHeight; y++)
	{
		float zPos = halfDepth - y * dz;
		for (UINT x = 0; x < fileWidth; x++)
		{
			float xPos = -halfWidth + x * dx;
			UINT index = y * fileWidth + x;

			// Set position
			float mapHeight = getHeightAt(x, y);
			m_vertices[index].Pos = XMFLOAT3(xPos, mapHeight, zPos);

			// set normal
			XMFLOAT3 normal(0.0f, 1.0f, 0.0f);
			XMVECTOR tangentVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			/*float heightL = getHeightAt(j - 1, i);
			float heightR = getHeightAt(j + 1, i);
			float heightU = getHeightAt(j, i + 1);
			float heightD = getHeightAt(j, i - 1);

			XMVECTOR tangentVec = XMVectorSet(2.0f * dx, heightL - heightR, 0.0f, 0.0f);
			XMVECTOR bitangentVec = XMVectorSet(0.0f, heightU - heightD, 2.0f * dz, 0.0f);
			XMStoreFloat3(&normal, XMVector3Normalize(XMVector3Cross(tangentVec, bitangentVec)));*/

			m_vertices[index].Normal = normal;

			// Compute and set tangent
			XMFLOAT3 tangent;
			XMStoreFloat3(&tangent, tangentVec);
			m_vertices[index].Tangent = XMFLOAT4(tangent.x, tangent.y, tangent.z, 1.0f);

			// Set texture coordinates
			m_vertices[index].TexC.x = x * dx;
			m_vertices[index].TexC.y = y * dz;
		}
	}

	UINT faceCount = (fileWidth - 1) * (fileHeight - 1) * 2;
	m_indices.resize(faceCount * 3);
	UINT k = 0;
	for (UINT y = 0; y < fileHeight - 1; y++)
	{
		for (UINT x = 0; x < fileWidth - 1; x++)
		{
			m_indices[k] = y * fileWidth + x;
			m_indices[k + 1] = (y + 1) * fileWidth + x;
			m_indices[k + 2] = y * fileWidth + (x + 1);

			m_indices[k + 3] = y * fileWidth + (x + 1);
			m_indices[k + 4] = (y + 1) * fileWidth + x;
			m_indices[k + 5] = (y + 1) * fileWidth + (x + 1);

			k += 6;
		}
	}
}

bool Terrain::loadHeightMap(const std::string& filePath, int fileWidth, int fileHeight, int heightScale)
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
		m_heightData[i] = (in[i] / 255.0f) * heightScale;
	}

	return true;
}

float Terrain::getHeightAt(int x, int y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= m_heightMapWidth) x = m_heightMapWidth - 1;
	if (y >= m_heightMapDepth) y = m_heightMapDepth - 1;

	return m_heightData[y * m_heightMapWidth + x];
}
