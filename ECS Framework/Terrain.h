#pragma once
#include <d3d11_4.h>
#include <string>
#include <vector>
#include "Structures.h"
#include "ECSScene.h"
#include "AABBTree.h"

class Terrain
{
public:
	Terrain() {};
	~Terrain();

	bool Init(ID3D11Device* device, ID3D11DeviceContext* context, const std::string& heightMapFile, int fileWidth, int fileHeight, int terrainWidth, int terrainDepth, int heightScale);
	void BuildCollision(ECSScene* scene, AABBTree* tree);
	void Draw(ID3D11DeviceContext* context);

private:
	std::vector<float> m_heightData;
	int m_heightMapWidth;
	int m_heightMapDepth;
	ID3D11Buffer* m_vertexBuffer = nullptr;
	ID3D11Buffer* m_indexBuffer = nullptr;

	ID3D11ShaderResourceView* m_rockMaterialSRV;
	ID3D11ShaderResourceView* m_grassMaterialSRV;

	std::vector<SimpleVertex> m_vertices;
	std::vector<UINT> m_indices;

	void createGrid(unsigned int terrainWidth, unsigned int terrainDepth, unsigned int fileWidth, unsigned int fileHeight);
	bool loadHeightMap(const std::string& filePath, unsigned int fileWidth, unsigned int fileHeight, int terrainScale);

	float getHeightAt(int x, int y);
	void setHeightAt(int x, int y, float height);
	bool inBounds(int x, int y);
	float average(int xStart, int yStart);
};