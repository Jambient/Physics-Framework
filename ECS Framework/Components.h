#pragma once
#include <DirectXMath.h>

using namespace DirectX;

struct Transform
{
	XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Scale = XMFLOAT3(0.0f, 0.0f, 0.0f);
};

struct Gravity
{
	XMFLOAT3 Force = XMFLOAT3(0.0f, 0.0f, 0.0f);
};

struct RigidBody
{
	XMFLOAT3 Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Acceleration = XMFLOAT3(0.0f, 0.0f, 0.0f);
};