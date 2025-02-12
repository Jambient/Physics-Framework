#include "OBJLoader.h"
#include <string>
#include <iostream>
#include <limits>

bool OBJLoader::FindSimilarVertex(const SimpleVertex& vertex, std::map<SimpleVertex, unsigned short>& vertToIndexMap, unsigned short& index)
{
	auto it = vertToIndexMap.find(vertex);
	
	if (it == vertToIndexMap.end())
	{
		return false;
	}
	else
	{
		index = it->second;
		return true;
	}
}

void OBJLoader::CreateIndices(const std::vector<XMFLOAT3>& inVertices, 
							  const std::vector<XMFLOAT2>& inTexCoords, 
							  const std::vector<XMFLOAT3>& inNormals, 
							  std::vector<unsigned short>& outIndices, 
							  std::vector<XMFLOAT3>& outVertices, 
							  std::vector<XMFLOAT2>& outTexCoords, 
							  std::vector<XMFLOAT3>& outNormals)
{
	// Mapping from an already-existing SimpleVertex to its corresponding index
	std::map<SimpleVertex, unsigned short> vertToIndexMap;

	std::pair<SimpleVertex, unsigned short> pair;

	int numVertices = inVertices.size();
	
	for(int i = 0; i < numVertices; ++i) //For each vertex
	{
		SimpleVertex vertex = {inVertices[i], inNormals[i],  inTexCoords[i]}; 

		unsigned short index;
		// See if a vertex already exists in the buffer that has the same attributes as this one
		bool found = FindSimilarVertex(vertex, vertToIndexMap, index); 
		
		if(found) //if found, re-use it's index for the index buffer
		{
			outIndices.push_back(index);
		}
		else //if not found, add it to the buffer
		{
			outVertices.push_back(vertex.Pos);
			outTexCoords.push_back(vertex.TexC);
			outNormals.push_back(vertex.Normal);
			
			unsigned short newIndex = (unsigned short)outVertices.size() - 1;
			
			outIndices.push_back(newIndex);
			
			//Add it to the map
			pair.first = vertex;
			pair.second = newIndex;
			
			//vertToIndexMap.insert(pair);
		}
	}
}

// converted code from https://gamedev.stackexchange.com/questions/68612/how-to-compute-tangent-and-bitangent-vectors into dx11
void OBJLoader::ComputeTangents(SimpleVertex* vertices, unsigned int numVertices, unsigned short* indices, unsigned int numIndices)
{
	XMFLOAT3* tan1 = new XMFLOAT3[numVertices * 2];
	XMFLOAT3* tan2 = tan1 + numVertices;
	ZeroMemory(tan1, numVertices * sizeof(XMFLOAT3) * 2);

	for (unsigned int i = 0; i < numIndices; i += 3)
	{
		unsigned short i1 = indices[i];
		unsigned short i2 = indices[i + 1];
		unsigned short i3 = indices[i + 2];

		SimpleVertex v1 = vertices[i1];
		SimpleVertex v2 = vertices[i2];
		SimpleVertex v3 = vertices[i3];

		XMFLOAT3 p1 = v1.Pos;
		XMFLOAT3 p2 = v2.Pos;
		XMFLOAT3 p3 = v3.Pos;

		XMFLOAT2 w1 = v1.TexC;
		XMFLOAT2 w2 = v2.TexC;
		XMFLOAT2 w3 = v3.TexC;

		float x1 = p2.x - p1.x;
		float x2 = p3.x - p1.x;
		float y1 = p2.y - p1.y;
		float y2 = p3.y - p1.y;
		float z1 = p2.z - p1.z;
		float z2 = p3.z - p1.z;

		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;

		float r = 1.0F / (s1 * t2 - s2 * t1);
		XMVECTOR sdir = XMVectorSet((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r, 0.0f);
		XMVECTOR tdir = XMVectorSet((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r, 0.0f);

		XMStoreFloat3(&tan1[i1], XMVectorAdd(XMLoadFloat3(&tan1[i1]), sdir));
		XMStoreFloat3(&tan1[i2], XMVectorAdd(XMLoadFloat3(&tan1[i2]), sdir));
		XMStoreFloat3(&tan1[i3], XMVectorAdd(XMLoadFloat3(&tan1[i3]), sdir));

		XMStoreFloat3(&tan2[i1], XMVectorAdd(XMLoadFloat3(&tan2[i1]), tdir));
		XMStoreFloat3(&tan2[i2], XMVectorAdd(XMLoadFloat3(&tan2[i2]), tdir));
		XMStoreFloat3(&tan2[i3], XMVectorAdd(XMLoadFloat3(&tan2[i3]), tdir));
	}

	for (unsigned int i = 0; i < numVertices; i++)
	{
		XMVECTOR normal = XMLoadFloat3(&vertices[i].Normal);
		XMVECTOR tangent = XMLoadFloat3(&tan1[i]);

		// Gram-Schmidt orthogonalize
		//XMStoreFloat3(&vertices[i].Tangent, XMVector3Normalize(XMVectorMultiply(XMVectorSubtract(tangent, normal), XMVector3Dot(normal, tangent))));
		XMVECTOR dot = XMVector3Dot(normal, tangent);
		XMVECTOR nMult = XMVectorMultiply(normal, dot);
		XMStoreFloat4(&vertices[i].Tangent, XMVector3Normalize(XMVectorSubtract(tangent, nMult)));

		// calculate handedness
		float handedness;
		XMStoreFloat(&handedness, XMVector3Dot(XMVector3Cross(normal, tangent), XMLoadFloat3(&tan2[i])));

		vertices[i].Tangent.w = (handedness < 0.0f) ? -1.0f : 1.0f;
	}

	delete[] tan1;
}

//WARNING: This code makes a big assumption -- that your models have texture coordinates AND normals which they should have anyway (else you can't do texturing and lighting!)
//If your .obj file has no lines beginning with "vt" or "vn", then you'll need to change the Export settings in your modelling software so that it exports the texture coordinates 
//and normals. If you still have no "vt" lines, you'll need to do some texture unwrapping, also known as UV unwrapping.
MeshData OBJLoader::Load(const char* filename, ID3D11Device* _pd3dDevice, bool invertTexCoords)
{
	MeshData meshData;
	std::string binaryFilename = filename;
	binaryFilename.append("Binary");
	std::ifstream binaryInFile;
	binaryInFile.open(binaryFilename, std::ios::in | std::ios::binary);

	if(!binaryInFile.good())
	{
		std::ifstream inFile;
		inFile.open(filename);

		if(!inFile.good())
		{
			return MeshData();
		}
		else
		{
			//Vectors to store the vertex positions, normals and texture coordinates. Need to use vectors since they're resizeable and we have
			//no way of knowing ahead of time how large these meshes will be
			std::vector<XMFLOAT3> verts;
			std::vector<XMFLOAT3> normals;
			std::vector<XMFLOAT2> texCoords;

			//DirectX uses 1 index buffer, OBJ is optimized for storage and not rendering and so uses 3 smaller index buffers.....great...
			//We'll have to merge this into 1 index buffer which we'll do after loading in all of the required data.
			std::vector<unsigned short> vertIndices;
			std::vector<unsigned short> normalIndices;
			std::vector<unsigned short> textureIndices;

			std::string input;

			XMFLOAT3 vert;
			XMFLOAT2 texCoord;
			XMFLOAT3 normal;
			unsigned short vInd[3]; //indices for the vertex position
			unsigned short tInd[3]; //indices for the texture coordinate
			unsigned short nInd[3]; //indices for the normal
			std::string beforeFirstSlash;
			std::string afterFirstSlash;
			std::string afterSecondSlash;

			while(!inFile.eof()) //While we have yet to reach the end of the file...
			{
				inFile >> input; //Get the next input from the file

				//Check what type of input it was, we are only interested in vertex positions, texture coordinates, normals and indices, nothing else
				if(input.compare("v") == 0) //Vertex position
				{
					inFile >> vert.x;
					inFile >> vert.y;
					inFile >> vert.z;

					verts.push_back(vert);
				}
				else if(input.compare("vt") == 0) //Texture coordinate
				{
					inFile >> texCoord.x;
					inFile >> texCoord.y;

					if(invertTexCoords) texCoord.y = 1.0f - texCoord.y;

					texCoords.push_back(texCoord);
				}
				else if(input.compare("vn") == 0) //Normal
				{
					inFile >> normal.x;
					inFile >> normal.y;
					inFile >> normal.z;

					normals.push_back(normal);
				}
				else if(input.compare("f") == 0) //Face
				{
					for(int i = 0; i < 3; ++i)
					{
						inFile >> input;
						int slash = input.find("/"); //Find first forward slash
						int secondSlash = input.find("/", slash + 1); //Find second forward slash

						//Extract from string
						beforeFirstSlash = input.substr(0, slash); //The vertex position index
						afterFirstSlash = input.substr(slash + 1, secondSlash - slash - 1); //The texture coordinate index
						afterSecondSlash = input.substr(secondSlash + 1); //The normal index

						//Parse into int
						vInd[i] = (unsigned short)atoi(beforeFirstSlash.c_str()); //atoi = "ASCII to int"
						tInd[i] = (unsigned short)atoi(afterFirstSlash.c_str());
						nInd[i] = (unsigned short)atoi(afterSecondSlash.c_str());
					}

					//Place into vectors
					for(int i = 0; i < 3; ++i)
					{
						vertIndices.push_back(vInd[i] - 1);		//Minus 1 from each as these as OBJ indexes start from 1 whereas C++ arrays start from 0
						textureIndices.push_back(tInd[i] - 1);	//which is really annoying. Apart from Lua and SQL, there's not much else that has indexing 
						normalIndices.push_back(nInd[i] - 1);	//starting at 1. So many more languages index from 0, the .OBJ people screwed up there.
					}
				}
			}
			inFile.close(); //Finished with input file now, all the data we need has now been loaded in

			//Get vectors to be of same size, ready for singular indexing
			std::vector<XMFLOAT3> expandedVertices;
			std::vector<XMFLOAT3> expandedNormals;
			std::vector<XMFLOAT2> expandedTexCoords;
			unsigned int numIndices = vertIndices.size();
			for(unsigned int i = 0; i < numIndices; i++)
			{
				expandedVertices.push_back(verts[vertIndices[i]]);
				expandedTexCoords.push_back(texCoords[textureIndices[i]]);
				expandedNormals.push_back(normals[normalIndices[i]]);
			}

			//Now to (finally) form the final vertex, texture coord, normal list and single index buffer using the above expanded vectors
			std::vector<unsigned short> meshIndices;
			meshIndices.reserve(numIndices);
			std::vector<XMFLOAT3> meshVertices;
			meshVertices.reserve(expandedVertices.size());
			std::vector<XMFLOAT3> meshNormals;
			meshNormals.reserve(expandedNormals.size());
			std::vector<XMFLOAT2> meshTexCoords;
			meshTexCoords.reserve(expandedTexCoords.size());

			CreateIndices(expandedVertices, expandedTexCoords, expandedNormals, meshIndices, meshVertices, meshTexCoords, meshNormals);

			//Turn data from vector form to arrays
			SimpleVertex* finalVerts = new SimpleVertex[meshVertices.size()];
			unsigned int numMeshVertices = meshVertices.size();
			for(unsigned int i = 0; i < numMeshVertices; ++i)
			{
				finalVerts[i].Pos = meshVertices[i];
				finalVerts[i].Normal = meshNormals[i];
				finalVerts[i].TexC = meshTexCoords[i];
			}

			unsigned short* indicesArray = new unsigned short[meshIndices.size()];
			unsigned int numMeshIndices = meshIndices.size();
			for (unsigned int i = 0; i < numMeshIndices; ++i)
			{
				indicesArray[i] = meshIndices[i];
			}

			ComputeTangents(finalVerts, numMeshVertices, indicesArray, numMeshIndices);
			meshData.Vertices = finalVerts;
			meshData.VerticesCount = numMeshVertices;

			//Put data into vertex and index buffers, then pass the relevant data to the MeshData object.
			//The rest of the code will hopefully look familiar to you, as it's similar to whats in your InitVertexBuffer and InitIndexBuffer methods
			ID3D11Buffer* vertexBuffer;

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(SimpleVertex) * meshVertices.size();
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = finalVerts;

			_pd3dDevice->CreateBuffer(&bd, &InitData, &vertexBuffer);

			meshData.VertexBuffer = vertexBuffer;
			meshData.VBOffset = 0;
			meshData.VBStride = sizeof(SimpleVertex);

			//Output data into binary file, the next time you run this function, the binary file will exist and will load that instead which is much quicker than parsing into vectors
			std::ofstream outbin(binaryFilename.c_str(), std::ios::out | std::ios::binary);
			outbin.write((char*)&numMeshVertices, sizeof(unsigned int));
			outbin.write((char*)&numMeshIndices, sizeof(unsigned int));
			outbin.write((char*)finalVerts, sizeof(SimpleVertex) * numMeshVertices);
			outbin.write((char*)indicesArray, sizeof(unsigned short) * numMeshIndices);
			outbin.close();

			ID3D11Buffer* indexBuffer;

			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(WORD) * meshIndices.size();     
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = indicesArray;
			_pd3dDevice->CreateBuffer(&bd, &InitData, &indexBuffer);

			meshData.IndexCount = meshIndices.size();
			meshData.IndexBuffer = indexBuffer;

			//This data has now been sent over to the GPU so we can delete this CPU-side stuff
			delete [] indicesArray;

			return meshData;
		}	
	}
	else
	{
		unsigned int numVertices;
		unsigned int numIndices;

		//Read in array sizes
		binaryInFile.read((char*)&numVertices, sizeof(unsigned int));
		binaryInFile.read((char*)&numIndices, sizeof(unsigned int));
		
		//Read in data from binary file
		SimpleVertex* finalVerts = new SimpleVertex[numVertices];
		unsigned short* indices = new unsigned short[numIndices];
		binaryInFile.read((char*)finalVerts, sizeof(SimpleVertex) * numVertices);
		binaryInFile.read((char*)indices, sizeof(unsigned short) * numIndices);

		//Put data into vertex and index buffers, then pass the relevant data to the MeshData object.
		//The rest of the code will hopefully look familiar to you, as it's similar to whats in your InitVertexBuffer and InitIndexBuffer methods
		ID3D11Buffer* vertexBuffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(SimpleVertex) * numVertices;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = finalVerts;

		_pd3dDevice->CreateBuffer(&bd, &InitData, &vertexBuffer);

		meshData.VertexBuffer = vertexBuffer;
		meshData.VBOffset = 0;
		meshData.VBStride = sizeof(SimpleVertex);

		meshData.Vertices = finalVerts;
		meshData.VerticesCount = numVertices;

		ID3D11Buffer* indexBuffer;

		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(WORD) * numIndices;     
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;

		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = indices;
		_pd3dDevice->CreateBuffer(&bd, &InitData, &indexBuffer);

		meshData.IndexCount = numIndices;
		meshData.IndexBuffer = indexBuffer;

		//This data has now been sent over to the GPU so we can delete this CPU-side stuff
		delete [] indices;
	}

	return meshData;
}