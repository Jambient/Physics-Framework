#pragma once
#include "Definitions.h"
#include <vector>

class Archetype
{
public:
	Archetype(Signature signature)
	{
		signature = m_signature;
	}

	template<typename T>
	std::vector<T>& GetComponentArray()
	{

	}

private:
	Signature m_signature;
	
};