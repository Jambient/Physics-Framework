#pragma once
#include "Components.h"
#include "Scene.h"

class ColliderUpdateSystem : public System
{
public:
	void Update()
	{
		// integrate linear movement
		m_scene->ForEach<Transform, Collider>([](Entity entity, Transform* transform, Collider* collider)
			{
                std::visit([&](auto& specificCollider) {
                    using T = std::decay_t<decltype(specificCollider)>;

                    if constexpr (std::is_same_v<T, Sphere>)
                    {
                        specificCollider.center = transform->Position;
                    }
                    else if constexpr (std::is_same_v<T, OBB>)
                    {
                        specificCollider.Update(transform->Position, transform->Scale, transform->Rotation);
                    }
                    }, collider->Collider);
			});
	}
};