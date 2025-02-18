#include "NarrowPhaseSystem.h"
#include "Components.h"
#include "ECSScene.h"
#include "AABBTree.h"
#include "Collision.h"

void NarrowPhaseSystem::Update(ECSScene& scene, float dt)
{
    // broad phase to get collisions that could be intersecting
    std::vector<CollisionInfo> collisions;
    std::vector<std::pair<Entity, Entity>> potential = m_aabbTree.GetPotentialIntersections();
    m_debugPoints.clear();

    // narrow phase to confirm each collision
    for (const auto& [entity1, entity2] : potential)
    {
        // confirm that both entities have the components required for collision resolution
        if (!(scene.HasComponent<Collider>(entity1) && scene.HasComponent<Transform>(entity1))
            || !(scene.HasComponent<Collider>(entity2) && scene.HasComponent<Transform>(entity2)))
        {
            continue;
        }

        ColliderBase& e1Collider = scene.GetComponent<Collider>(entity1)->GetColliderBase();
        ColliderBase& e2Collider = scene.GetComponent<Collider>(entity2)->GetColliderBase();
        CollisionManifold manifold;

        if (!Collision::Collide(e1Collider, e2Collider, manifold))
            continue; // skip if narrow-phase fails
        
        m_debugPoints.insert(m_debugPoints.end(), manifold.contactPoints.begin(), manifold.contactPoints.end());

        collisions.push_back({ entity1, entity2, manifold });
    }

    // resolve velocities
    for (int i = 0; i < VELOCITY_ITERATIONS; i++)
    {
        for (const CollisionInfo& info : collisions)
        {
            // get components for entities
            Transform* e1Transform = scene.GetComponent<Transform>(info.entityA);
            Particle* e1Particle = scene.GetComponent<Particle>(info.entityA);
            RigidBody* e1RigidBody = scene.GetComponent<RigidBody>(info.entityA);
            PhysicsMaterial* e1Material = scene.GetComponent<PhysicsMaterial>(info.entityA);
            Transform* e2Transform = scene.GetComponent<Transform>(info.entityB);
            Particle* e2Particle = scene.GetComponent<Particle>(info.entityB);
            RigidBody* e2RigidBody = scene.GetComponent<RigidBody>(info.entityB);
            PhysicsMaterial* e2Material = scene.GetComponent<PhysicsMaterial>(info.entityB);

            float totalInverseMass = (e1Particle != nullptr ? e1Particle->inverseMass : 0.0f) + (e2Particle != nullptr ? e2Particle->inverseMass : 0.0f);

            float e1Restitution = e1Material != nullptr ? e1Material->restitution : 0.5f;
            float e2Restitution = e2Material != nullptr ? e2Material->restitution : 0.5f;
            float collisionRestitution = e1Restitution * e2Restitution;

            float e1Friction = e1Material != nullptr ? e1Material->dynamicFriction : 0.5f;
            float e2Friction = e2Material != nullptr ? e2Material->dynamicFriction : 0.5f;
            float collisionFriction = (e1Friction + e2Friction) / 2.0f;

            for (const Vector3& contactPoint : info.manifold.contactPoints)
            {
                Vector3 relativeA = contactPoint - e1Transform->position;
                Vector3 relativeB = contactPoint - e2Transform->position;

                Vector3 angVelocityA = Vector3::Cross(e1RigidBody != nullptr ? e1RigidBody->angularVelocity : Vector3::Zero, relativeA);
                Vector3 angVelocityB = Vector3::Cross(e2RigidBody != nullptr ? e2RigidBody->angularVelocity : Vector3::Zero, relativeB);

                Vector3 fullVelocityA = (e1Particle != nullptr ? e1Particle->linearVelocity : Vector3::Zero) + angVelocityA;
                Vector3 fullVelocityB = (e2Particle != nullptr ? e2Particle->linearVelocity : Vector3::Zero) + angVelocityB;
                Vector3 contactVelocity = fullVelocityB - fullVelocityA;

                float impulseForce = Vector3::Dot(contactVelocity, info.manifold.normal);
                if (impulseForce > 0.0f) continue;

                Vector3 inertiaA = Vector3::Cross((e1RigidBody != nullptr ? e1RigidBody->inverseInertiaTensor : Matrix3(Vector3::Zero)) * Vector3::Cross(relativeA, info.manifold.normal), relativeA);
                Vector3 inertiaB = Vector3::Cross((e2RigidBody != nullptr ? e2RigidBody->inverseInertiaTensor : Matrix3(Vector3::Zero)) * Vector3::Cross(relativeB, info.manifold.normal), relativeB);
                float angularEffect = Vector3::Dot(inertiaA + inertiaB, info.manifold.normal);

                float j = (-(1.0f + collisionRestitution) * impulseForce) / (totalInverseMass + angularEffect);

                Vector3 fullImpulse = info.manifold.normal * j;

                if (e1Particle != nullptr) e1Particle->ApplyLinearImpulse(-fullImpulse);
                if (e2Particle != nullptr) e2Particle->ApplyLinearImpulse(fullImpulse);

                if (e1RigidBody != nullptr) e1RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeA, -fullImpulse));
                if (e2RigidBody != nullptr) e2RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeB, fullImpulse));
            }

            // friction
            for (const Vector3& contactPoint : info.manifold.contactPoints)
            {
                Vector3 relativeA = contactPoint - e1Transform->position;
                Vector3 relativeB = contactPoint - e2Transform->position;

                Vector3 angVelocityA = Vector3::Cross(e1RigidBody != nullptr ? e1RigidBody->angularVelocity : Vector3::Zero, relativeA);
                Vector3 angVelocityB = Vector3::Cross(e2RigidBody != nullptr ? e2RigidBody->angularVelocity : Vector3::Zero, relativeB);

                Vector3 fullVelocityA = (e1Particle != nullptr ? e1Particle->linearVelocity : Vector3::Zero) + angVelocityA;
                Vector3 fullVelocityB = (e2Particle != nullptr ? e2Particle->linearVelocity : Vector3::Zero) + angVelocityB;
                Vector3 contactVelocity = fullVelocityB - fullVelocityA;

                // calculate tangent velocity
                Vector3 tangent = contactVelocity - info.manifold.normal * Vector3::Dot(contactVelocity, info.manifold.normal);
                if (tangent.magnitude() < EPSILON)
                    continue;

                tangent.normalize();

                // calcualte tangent impulse
                float jt = -Vector3::Dot(contactVelocity, tangent);

                Vector3 rAct = Vector3::Cross(relativeA, tangent);
                Vector3 rBct = Vector3::Cross(relativeB, tangent);
                Vector3 angInertiaA = Vector3::Cross((e1RigidBody != nullptr ? e1RigidBody->inverseInertiaTensor : Matrix3(Vector3::Zero)) * rAct, relativeA);
                Vector3 angInertiaB = Vector3::Cross((e2RigidBody != nullptr ? e2RigidBody->inverseInertiaTensor : Matrix3(Vector3::Zero)) * rBct, relativeB);
                float angularEffect = Vector3::Dot(angInertiaA + angInertiaB, tangent);

                jt = jt / (totalInverseMass + angularEffect);

                // TODO: compare jt and j from other loop to decide if static or dynamic friction should be used.
                Vector3 frictionImpulse = tangent * jt * collisionFriction;

                if (e1Particle != nullptr) e1Particle->ApplyLinearImpulse(-frictionImpulse);
                if (e2Particle != nullptr) e2Particle->ApplyLinearImpulse(frictionImpulse);

                if (e1RigidBody != nullptr) e1RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeA, -frictionImpulse));
                if (e2RigidBody != nullptr) e2RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeB, frictionImpulse));
            }
        }
    }

    // resolve positions
    for (int i = 0; i < POSITION_ITERATIONS; i++)
    {
        for (const CollisionInfo& info : collisions)
        {
            Transform* e1Transform = scene.GetComponent<Transform>(info.entityA);
            Particle* e1Particle = scene.GetComponent<Particle>(info.entityA);
            Transform* e2Transform = scene.GetComponent<Transform>(info.entityB);
            Particle* e2Particle = scene.GetComponent<Particle>(info.entityB);

            float e1InverseMass = e1Particle != nullptr ? e1Particle->inverseMass : 0.0f;
            float e2InverseMass = e2Particle != nullptr ? e2Particle->inverseMass : 0.0f;

            float totalInverseMass = e1InverseMass + e2InverseMass;
            if (totalInverseMass == 0)
                continue;

            float penetration = (info.manifold.penetration - POSITION_PENETRATION_THRESHOLD) > 0.0f ? (info.manifold.penetration - POSITION_PENETRATION_THRESHOLD) : 0.0f;
            Vector3 correction = info.manifold.normal * (penetration * POSITION_CORRECTION_PERCENT / totalInverseMass);

            e1Transform->position -= correction * e1InverseMass;
            e2Transform->position += correction * e2InverseMass;
        }
    }

    // resolve springs
    for (int i = 0; i < SPRING_ITERATIONS; i++)
    {
        scene.ForEach<Spring>([&](Entity entity, Spring* spring) {
            Transform* e1Transform = scene.GetComponent<Transform>(spring->entityA);
            Particle* e1Particle = scene.GetComponent<Particle>(spring->entityA);
            Transform* e2Transform = scene.GetComponent<Transform>(spring->entityB);
            Particle* e2Particle = scene.GetComponent<Particle>(spring->entityB);

            // confirm that both entities have the components required for spring physics
            if (e1Transform == nullptr || e1Particle == nullptr || e2Transform == nullptr || e2Particle == nullptr)
            {
                return;
            }

            float totalMass = e1Particle->inverseMass + e2Particle->inverseMass;
            if (totalMass == 0.0f) return;

            Vector3 delta = e2Transform->position - e1Transform->position;
            float currentLength = delta.magnitude();
            if (currentLength < EPSILON) return;

            if (currentLength > 5.0f * spring->restLength)
            {
                scene.DestroyEntity(entity);
                return;
            }

            Vector3 normal = delta / currentLength;

            Vector3 relativeVelocity = e2Particle->linearVelocity - e1Particle->linearVelocity;
            float relVelAlongNormal = Vector3::Dot(relativeVelocity, normal);
            float displacement = currentLength - spring->restLength;
            float F = -spring->stiffness * displacement - 0.5f * relVelAlongNormal;

            float impulseScalar = F / totalMass;
            Vector3 impulse = normal * impulseScalar;

            // Apply the impulse to adjust velocities.
            e1Particle->ApplyLinearImpulse(-impulse);
            e2Particle->ApplyLinearImpulse(impulse);
            });
    }
}
