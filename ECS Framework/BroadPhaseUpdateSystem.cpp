#include "BroadPhaseUpdateSystem.h"
#include "Components.h"
#include "ECSScene.h"
#include "AABBTree.h"

void BroadPhaseUpdateSystem::Update(ECSScene& scene, float dt)
{
    // aabb update
    scene.ForEach<Transform, Collider>([&](Entity entity, Transform* transform, Collider* collider) {
        std::visit([&](auto& specificCollider) {
            using T = std::decay_t<decltype(specificCollider)>;

            if constexpr (std::is_same_v<T, Sphere>)
            {
                m_aabbTree.UpdatePosition(entity, specificCollider.center);
                m_aabbTree.UpdateScale(entity, Vector3::One * 2.0f * specificCollider.radius);
            }
            else if constexpr (std::is_same_v<T, OBB>)
            {
                AABB aabb = specificCollider.toAABB();
                m_aabbTree.UpdatePosition(entity, aabb.getPosition());
                m_aabbTree.UpdateScale(entity, aabb.getSize());
            }
            else if constexpr (std::is_same_v<T, AABB>)
            {
                m_aabbTree.UpdatePosition(entity, specificCollider.getPosition());
                m_aabbTree.UpdateScale(entity, specificCollider.getSize());
            }
            else if constexpr (std::is_same_v<T, Point>)
            {
                m_aabbTree.UpdatePosition(entity, specificCollider.position);
            }
            }, collider->Collider);

        m_aabbTree.TriggerUpdate(entity);
        });
}
