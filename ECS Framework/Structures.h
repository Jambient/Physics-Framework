#pragma once
#include <windows.h>
#include <DirectXMath.h>
#include <cstring>
#include <vector>
#include <memory>
#include "Definitions.h"

using namespace DirectX;

#define MAX_LIGHTS 10

enum class DrawPass
{
	OPAQUE_PASS,
	SKYBOX_PASS,
	TRANSLUCENT_PASS
};

struct AABB
{
	XMFLOAT3 Center;
	XMFLOAT3 HalfSize;

	bool Contains(const XMFLOAT3& point) const 
	{
		return std::abs(point.x - Center.x) <= HalfSize.x &&
			std::abs(point.y - Center.y) <= HalfSize.y &&
			std::abs(point.z - Center.z) <= HalfSize.z;
	}

	bool Intersects(const AABB& other) const 
	{
		return std::abs(Center.x - other.Center.x) <= (HalfSize.x + other.HalfSize.x) &&
			std::abs(Center.y - other.Center.y) <= (HalfSize.y + other.HalfSize.y) &&
			std::abs(Center.z - other.Center.z) <= (HalfSize.z + other.HalfSize.z);
	}
};

struct OctreeNode
{
	AABB Bounds;
	std::vector<Entity> Entities;
	std::unique_ptr<OctreeNode> Children[8];
	int Depth = 0;

	bool IsLeaf() const 
	{
		return Children[0] == nullptr;
	}
};

struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 TexC;
	XMFLOAT4 Tangent;

	bool operator<(const SimpleVertex other) const
	{
		return memcmp((void*)this, (void*)&other, sizeof(SimpleVertex)) > 0;
	};
};

enum class LightType
{
	DIRECTIONAL,
	POINT,
	SPOT
};

struct Light
{
	LightType Type;
	XMFLOAT3 Position;
	XMFLOAT3 Direction;
	float Range;
	XMFLOAT3 Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	float Intensity = 1;
	float SpotAngle;
	XMFLOAT3 _padding;
};

// Constant Buffers
struct TransformBuffer
{
	XMMATRIX Projection;
	XMMATRIX View;
	XMMATRIX World;
};

struct GlobalBuffer
{
	XMFLOAT4 AmbientLight;
	XMFLOAT3 CameraPosition;
	float TimeStamp;
};

struct LightBuffer
{
	Light Lights[MAX_LIGHTS];
	UINT LightCount;
	XMFLOAT3 _padding;
};