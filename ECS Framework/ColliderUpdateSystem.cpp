#include "ColliderUpdateSystem.h"

void ColliderUpdateSystem::Update(ECSScene& scene, float dt)
{
    scene.ForEach<Transform, Collider>([](Entity entity, Transform* transform, Collider* collider)
        {
            std::visit([transform, collider](auto& specificCollider) {
                using T = std::decay_t<decltype(specificCollider)>;

                if constexpr (std::is_same_v<T, Sphere>)
                {
                    specificCollider.SetCenter(transform->Position);
                }
                else if constexpr (std::is_same_v<T, OBB>)
                {
                    specificCollider.Update(transform->Position, transform->Scale, transform->Rotation);
                }
                else if constexpr (std::is_same_v<T, AABB>)
                {
                    specificCollider.UpdatePosition(transform->Position);
                    specificCollider.UpdateScale(transform->Scale);
                }
                else if constexpr (std::is_same_v<T, Point>)
                {
                    specificCollider.SetPosition(transform->Position);
                }
                }, collider->Collider);
        });
}
