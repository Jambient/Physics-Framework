#include "NarrowPhaseSystem.h"
#include "Components.h"
#include "ECSScene.h"
#include "AABBTree.h"
#include "Collision.h"

void NarrowPhaseSystem::Update(ECSScene& scene, float dt)
{
    // broad phase to get collisions that could be intersecting
    std::vector<std::tuple<Entity, Entity, CollisionManifold>> collisions;
    std::vector<std::pair<Entity, Entity>> potential = m_aabbTree.GetPotentialIntersections();
    m_debugPoints.clear();

    // narrow phase to confirm each collision
    for (const auto [entity1, entity2] : potential)
    {
        // if either entity doesn't have a collider then it cant collide
        if (!scene.HasComponent<Collider>(entity1) ||
            !scene.HasComponent<Collider>(entity2))
        {
            continue;
        }

        ColliderBase& e1Collider = scene.GetComponent<Collider>(entity1)->GetColliderBase();
        ColliderBase& e2Collider = scene.GetComponent<Collider>(entity2)->GetColliderBase();

        CollisionManifold info = Collision::Collide(e1Collider, e2Collider);
        if (!info) { continue; } // Skip if narrow-phase fails
        
        m_debugPoints.insert(m_debugPoints.end(), info.contactPoints.begin(), info.contactPoints.end());

        collisions.emplace_back(entity1, entity2, info);
    }

    // resolve velocities
    for (int i = 0; i < VELOCITY_ITERATIONS; i++)
    {
        for (const auto [entity1, entity2, manifold] : collisions)
        {
            // get components for entities
            Transform* e1Transform = scene.GetComponent<Transform>(entity1);
            Particle* e1Particle = scene.GetComponent<Particle>(entity1);
            RigidBody* e1RigidBody = scene.GetComponent<RigidBody>(entity1);
            Transform* e2Transform = scene.GetComponent<Transform>(entity2);
            Particle* e2Particle = scene.GetComponent<Particle>(entity2);
            RigidBody* e2RigidBody = scene.GetComponent<RigidBody>(entity2);

            float totalMass = e1Particle->InverseMass + e2Particle->InverseMass;

            for (const Vector3& contactPoint : manifold.contactPoints)
            {
                Vector3 relativeA = contactPoint - e1Transform->Position;
                Vector3 relativeB = contactPoint - e2Transform->Position;

                Vector3 angVelocityA = Vector3::Cross(e1RigidBody->AngularVelocity, relativeA);
                Vector3 angVelocityB = Vector3::Cross(e2RigidBody->AngularVelocity, relativeB);

                Vector3 fullVelocityA = e1Particle->LinearVelocity + angVelocityA;
                Vector3 fullVelocityB = e2Particle->LinearVelocity + angVelocityB;
                Vector3 contactVelocity = fullVelocityB - fullVelocityA;

                float impulseForce = Vector3::Dot(contactVelocity, manifold.collisionNormal);
                if (impulseForce > 0.0f) continue;

                Vector3 inertiaA = Vector3::Cross(e1RigidBody->InverseInertiaTensor * Vector3::Cross(relativeA, manifold.collisionNormal), relativeA);
                Vector3 inertiaB = Vector3::Cross(e2RigidBody->InverseInertiaTensor * Vector3::Cross(relativeB, manifold.collisionNormal), relativeB);
                float angularEffect = Vector3::Dot(inertiaA + inertiaB, manifold.collisionNormal);

                float restitution = e1Particle->Restitution * e2Particle->Restitution;
                float j = (-(1.0f + restitution) * impulseForce) / (totalMass + angularEffect);

                Vector3 fullImpulse = manifold.collisionNormal * j;

                e1Particle->ApplyLinearImpulse(-fullImpulse);
                e2Particle->ApplyLinearImpulse(fullImpulse);

                e1RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeA, -fullImpulse));
                e2RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeB, fullImpulse));
            }

            // friction
            for (const Vector3& contactPoint : manifold.contactPoints)
            {
                Vector3 relativeA = contactPoint - e1Transform->Position;
                Vector3 relativeB = contactPoint - e2Transform->Position;

                Vector3 angVelocityA = Vector3::Cross(e1RigidBody->AngularVelocity, relativeA);
                Vector3 angVelocityB = Vector3::Cross(e2RigidBody->AngularVelocity, relativeB);

                Vector3 fullVelocityA = e1Particle->LinearVelocity + angVelocityA;
                Vector3 fullVelocityB = e2Particle->LinearVelocity + angVelocityB;
                Vector3 contactVelocity = fullVelocityB - fullVelocityA;

                // calculate tangent velocity
                Vector3 tangent = contactVelocity - manifold.collisionNormal * Vector3::Dot(contactVelocity, manifold.collisionNormal);
                if (tangent.magnitude() < 1e-6f)
                    continue;

                tangent.normalize();

                // calcualte tangent impulse
                float jt = -Vector3::Dot(contactVelocity, tangent);

                Vector3 rAct = Vector3::Cross(relativeA, tangent);
                Vector3 rBct = Vector3::Cross(relativeB, tangent);
                Vector3 angInertiaA = Vector3::Cross(e1RigidBody->InverseInertiaTensor * rAct, relativeA);
                Vector3 angInertiaB = Vector3::Cross(e2RigidBody->InverseInertiaTensor * rBct, relativeB);
                float angularEffect = Vector3::Dot(angInertiaA + angInertiaB, tangent);

                jt = jt / (totalMass + angularEffect);

                // constant friction coefficients
                // to calculate properly - add frictions from both bodies and average them
                float staticFriction = 0.6f;
                float dynamicFriction = 0.3f;

                // TODO: compare jt and j from other loop to decide if static or dynamic friction should be used.
                Vector3 frictionImpulse = tangent * jt * dynamicFriction;

                e1Particle->ApplyLinearImpulse(-frictionImpulse);
                e2Particle->ApplyLinearImpulse(frictionImpulse);

                e1RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeA, -frictionImpulse));
                e2RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeB, frictionImpulse));
            }
        }
    }

    // resolve positions
    for (int i = 0; i < POSITION_ITERATIONS; i++)
    {
        for (const auto [entity1, entity2, manifold] : collisions)
        {
            Transform* e1Transform = scene.GetComponent<Transform>(entity1);
            Particle* e1Particle = scene.GetComponent<Particle>(entity1);
            Transform* e2Transform = scene.GetComponent<Transform>(entity2);
            Particle* e2Particle = scene.GetComponent<Particle>(entity2);

            float totalMass = e1Particle->InverseMass + e2Particle->InverseMass;
            if (totalMass == 0)
                continue;

            float penetration = (manifold.penetrationDepth - POSITION_PENETRATION_THRESHOLD) > 0.0f ? (manifold.penetrationDepth - POSITION_PENETRATION_THRESHOLD) : 0.0f;
            Vector3 correction = manifold.collisionNormal * (penetration * POSITION_CORRECTION_PERCENT / totalMass);

            e1Transform->Position -= correction * e1Particle->InverseMass;
            e2Transform->Position += correction * e2Particle->InverseMass;
        }
    }

    // resolve springs
    for (int i = 0; i < SPRING_ITERATIONS; i++)
    {
        scene.ForEach<Spring>([&](Entity entity, Spring* spring) {
            Transform* e1Transform = scene.GetComponent<Transform>(spring->Entity1);
            Particle* e1Particle = scene.GetComponent<Particle>(spring->Entity1);
            Transform* e2Transform = scene.GetComponent<Transform>(spring->Entity2);
            Particle* e2Particle = scene.GetComponent<Particle>(spring->Entity2);

            float totalMass = e1Particle->InverseMass + e2Particle->InverseMass;
            if (totalMass == 0.0f) return;

            Vector3 delta = e2Transform->Position - e1Transform->Position;
            float currentLength = delta.magnitude();
            if (currentLength < 1e-6f) return;

            if (currentLength > 5.0f * spring->RestLength)
            {
                scene.DestroyEntity(entity);
                return;
            }

            Vector3 normal = delta / currentLength;

            Vector3 relativeVelocity = e2Particle->LinearVelocity - e1Particle->LinearVelocity;
            float relVelAlongNormal = Vector3::Dot(relativeVelocity, normal);
            float displacement = currentLength - spring->RestLength;
            float F = -spring->Stiffness * displacement - 0.5f * relVelAlongNormal;

            float impulseScalar = F / totalMass;
            Vector3 impulse = normal * impulseScalar;

            // Apply the impulse to adjust velocities.
            e1Particle->ApplyLinearImpulse(-impulse);
            e2Particle->ApplyLinearImpulse(impulse);
            });
    }
}
