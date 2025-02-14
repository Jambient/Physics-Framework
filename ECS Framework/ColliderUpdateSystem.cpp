#include "ColliderUpdateSystem.h"

void ColliderUpdateSystem::Update(ECSScene& scene, float dt)
{
    scene.ForEach<Transform, Collider>([](Entity entity, Transform* transform, Collider* collider)
        {
            std::visit([transform, collider](auto& specificCollider) {
                using T = std::decay_t<decltype(specificCollider)>;

                if constexpr (std::is_same_v<T, Sphere>)
                {
                    specificCollider.center = transform->Position;
                }
                else if constexpr (std::is_same_v<T, OBB>)
                {
                    specificCollider.Update(transform->Position, transform->Scale, transform->Rotation);
                }
                else if constexpr (std::is_same_v<T, AABB>)
                {
                    specificCollider.updatePosition(transform->Position);
                    specificCollider.updateScale(transform->Scale);
                }
                else if constexpr (std::is_same_v<T, Point>)
                {
                    specificCollider.position = transform->Position;
                }
                }, collider->Collider);
        });
}
