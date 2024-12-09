#pragma once
#include <cstdint>
#include <bitset>

using Entity = std::uint32_t;
const Entity MAX_ENTITIES = 100000;

using ComponentType = std::uint8_t;
const std::size_t MAX_COMPONENT_TYPES = 32;
constexpr ComponentType INVALID_COMPONENT_TYPE = -1;

using Signature = std::bitset<MAX_COMPONENT_TYPES>;