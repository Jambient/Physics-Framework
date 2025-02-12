#pragma once
#include <cstdint>
#include <bitset>

using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 20000;
constexpr Entity INVALID_ENTITY = -1;

using ComponentType = std::uint8_t;
const std::size_t MAX_COMPONENT_TYPES = 32;
constexpr ComponentType INVALID_COMPONENT_TYPE = -1;

using Signature = std::bitset<MAX_COMPONENT_TYPES>;

#define FPS60 1.0 / 60.0