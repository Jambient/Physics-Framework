#pragma once
#include "Vector3.h"

struct Transform
{
	Vector3 Position = Vector3::Zero;
	Vector3 Rotation = Vector3::Zero;
	Vector3 Scale = Vector3::One;
};

struct Gravity
{
	Vector3 Force = Vector3::Zero;
};

struct RigidBody
{
	Vector3 Velocity = Vector3::Zero;
	Vector3 Acceleration = Vector3::Zero;
};