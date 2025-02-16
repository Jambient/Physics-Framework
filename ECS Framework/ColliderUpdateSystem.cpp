#include "ColliderUpdateSystem.h"

void ColliderUpdateSystem::Update(ECSScene& scene, float dt)
{
    scene.ForEach<Transform, Collider>([](Entity entity, Transform* transform, Collider* collider)
        {
            std::visit([transform, collider](auto& specificCollider) {
                using T = std::decay_t<decltype(specificCollider)>;

                if constexpr (std::is_same_v<T, Sphere>)
                {
                    specificCollider.SetCenter(transform->position);
                }
                else if constexpr (std::is_same_v<T, OBB>)
                {
                    specificCollider.Update(transform->position, transform->scale, transform->rotation);
                }
                else if constexpr (std::is_same_v<T, AABB>)
                {
                    specificCollider.UpdatePosition(transform->position);
                    specificCollider.UpdateScale(transform->scale);
                }
                else if constexpr (std::is_same_v<T, Point>)
                {
                    specificCollider.SetPosition(transform->position);
                }
                }, collider->collider);
        });
}
