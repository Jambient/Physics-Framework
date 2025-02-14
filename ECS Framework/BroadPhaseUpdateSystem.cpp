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
                m_aabbTree.UpdatePosition(entity, specificCollider.GetCenter());
                m_aabbTree.UpdateScale(entity, Vector3::One * 2.0f * specificCollider.GetRadius());
            }
            else if constexpr (std::is_same_v<T, OBB>)
            {
                AABB aabb = specificCollider.ToAABB();
                m_aabbTree.UpdatePosition(entity, aabb.GetPosition());
                m_aabbTree.UpdateScale(entity, aabb.GetSize());
            }
            else if constexpr (std::is_same_v<T, AABB>)
            {
                m_aabbTree.UpdatePosition(entity, specificCollider.GetPosition());
                m_aabbTree.UpdateScale(entity, specificCollider.GetSize());
            }
            else if constexpr (std::is_same_v<T, Point>)
            {
                m_aabbTree.UpdatePosition(entity, specificCollider.GetPosition());
            }
            }, collider->Collider);

        m_aabbTree.TriggerUpdate(entity);
        });
}
