#include "Terrain.h"
#include "Material.h"
#include "ShaderManager.h"
#include "Components.h"
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

	m_heightMapWidth = fileWidth;
	m_heightMapDepth = fileHeight;

	// smooth height map
	/*for (int y = 0; y < fileHeight; y++)
	{
		for (int x = 0; x < fileWidth; x++)
		{
			setHeightAt(x, y, average(x, y));
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

void Terrain::BuildCollision(ECSScene* scene, AABBTree* tree)
{
	for (int i = 0; i < 3 * 2000; i += 3)
	{
		SimpleVertex v1 = m_vertices[m_indices[i]];
		SimpleVertex v2 = m_vertices[m_indices[i + 1]];
		SimpleVertex v3 = m_vertices[m_indices[i + 2]];

		Vector3 p1 = Vector3(v1.Pos.x, v1.Pos.y, v1.Pos.z) + Vector3::Down * 5;
		Vector3 p2 = Vector3(v2.Pos.x, v2.Pos.y, v2.Pos.z) + Vector3::Down * 5;
		Vector3 p3 = Vector3(v3.Pos.x, v3.Pos.y, v3.Pos.z) + Vector3::Down * 5;
		Vector3 center = (p1 + p2 + p3) / 3.0f;

		Vector3 e1 = p2 - p1;
		Vector3 e2 = p3 - p1;
		Vector3 normal = Vector3::Cross(e1, e2).normalized();

		Entity entity = scene->CreateEntity();
		scene->AddComponent(
			entity,
			Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, 0.0f, 0.15f }
		);
		scene->AddComponent(
			entity,
			Transform{ center, Quaternion(), Vector3::One }
		);
		scene->AddComponent(
			entity,
			RigidBody{ Vector3::Zero, Vector3::Zero, Vector3::Zero, Matrix3(Vector3::Zero) }
		);
		scene->AddComponent(
			entity,
			Collider{ HalfSpaceTriangle(p1, p2, p3, normal) }
		);

		tree->InsertEntity(entity, AABB::FromTriangle(p1, p2, p3), true);
	}
}

void Terrain::Draw(ID3D11DeviceContext* context)
{
	ShaderManager* sm = ShaderManager::GetInstance();
	TransformBuffer transformBuffer;

	if (sm->GetConstantBufferData<TransformBuffer>("TransformBuffer", transformBuffer))
	{
		//transformBuffer.World = XMMatrixTranspose(XMMatrixIdentity());
		transformBuffer.World = XMMatrixTranspose(XMMatrixTranslation(0.0f, -5.0f, 0.0f));
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

			float diffX = getHeightAt(x + 1, y) - getHeightAt(x - 1, y);
			float diffZ = getHeightAt(x, y + 1) - getHeightAt(x, y - 1);

			XMVECTOR tangentVec = XMVectorSet(2.0f * dx, diffX, 0.0f, 0.0f);
			XMVECTOR bitangentVec = XMVectorSet(0.0f, diffZ, 2.0f * dz, 0.0f);

			// To get an upward-pointing normal, take the cross product in the proper order.
			XMVECTOR normalVec = XMVector3Normalize(XMVector3Cross(bitangentVec, tangentVec));

			XMFLOAT3 normal;
			XMStoreFloat3(&normal, normalVec);
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
			m_indices[k + 1] = y * fileWidth + (x + 1);
			m_indices[k + 2] = (y + 1) * fileWidth + x;

			m_indices[k + 3] = y * fileWidth + (x + 1);
			m_indices[k + 4] = (y + 1) * fileWidth + (x + 1);
			m_indices[k + 5] = (y + 1) * fileWidth + x;

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

	float test = in[imageSize - 1];

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

void Terrain::setHeightAt(int x, int y, float height)
{
	m_heightData[y * m_heightMapWidth + x] = height;
}

bool Terrain::inBounds(int x, int y)
{
	return x >= 0 && x < m_heightMapWidth && y >= 0 && y < m_heightMapDepth;
}

float Terrain::average(int xStart, int yStart)
{
	float avg = 0.0f;
	float num = 0.0f;

	for (int x = xStart - 3; x <= xStart + 3; x++)
	{
		for (int y = yStart - 3; y <= yStart + 3; y++)
		{
			if (inBounds(x, y))
			{
				avg += getHeightAt(x, y);
				num += 1.0f;
			}
		}
	}

	return avg / num;
}