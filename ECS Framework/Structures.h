#pragma once
#include <windows.h>
#include <DirectXMath.h>
#include <cstring>
#include <vector>
#include <memory>
#include "Definitions.h"
#include "Vector3.h"
#include "Quaternion.h"

using namespace DirectX;

#define MAX_LIGHTS 10

enum class DrawPass
{
	OPAQUE_PASS,
	SKYBOX_PASS,
	TRANSLUCENT_PASS
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