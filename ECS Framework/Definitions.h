#pragma once
#include <cstdint>
#include <bitset>

// ECS
using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 40000;
constexpr Entity INVALID_ENTITY = -1;

using ComponentType = std::uint8_t;
const std::size_t MAX_COMPONENT_TYPES = 32;
constexpr ComponentType INVALID_COMPONENT_TYPE = -1;

using Signature = std::bitset<MAX_COMPONENT_TYPES>;

// PHYSICS
#define FPS60 1.0f / 60.0f

const int VELOCITY_ITERATIONS = 10;
const int POSITION_ITERATIONS = 5;
const int SPRING_ITERATIONS = 10;

const float POSITION_CORRECTION_PERCENT = 0.2f;
const float POSITION_PENETRATION_THRESHOLD = 0.01f;

// MISC
#define EPSILON 1e-6f