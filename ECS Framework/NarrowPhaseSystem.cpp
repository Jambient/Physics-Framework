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
        // if either entity doesn't have a collider then it cant collide
        if (!scene.HasComponent<Collider>(entity1) ||
            !scene.HasComponent<Collider>(entity2))
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
            Transform* e2Transform = scene.GetComponent<Transform>(info.entityB);
            Particle* e2Particle = scene.GetComponent<Particle>(info.entityB);
            RigidBody* e2RigidBody = scene.GetComponent<RigidBody>(info.entityB);

            float totalMass = e1Particle->inverseMass + e2Particle->inverseMass;

            for (const Vector3& contactPoint : info.manifold.contactPoints)
            {
                Vector3 relativeA = contactPoint - e1Transform->position;
                Vector3 relativeB = contactPoint - e2Transform->position;

                Vector3 angVelocityA = Vector3::Cross(e1RigidBody->angularVelocity, relativeA);
                Vector3 angVelocityB = Vector3::Cross(e2RigidBody->angularVelocity, relativeB);

                Vector3 fullVelocityA = e1Particle->linearVelocity + angVelocityA;
                Vector3 fullVelocityB = e2Particle->linearVelocity + angVelocityB;
                Vector3 contactVelocity = fullVelocityB - fullVelocityA;

                float impulseForce = Vector3::Dot(contactVelocity, info.manifold.normal);
                if (impulseForce > 0.0f) continue;

                Vector3 inertiaA = Vector3::Cross(e1RigidBody->inverseInertiaTensor * Vector3::Cross(relativeA, info.manifold.normal), relativeA);
                Vector3 inertiaB = Vector3::Cross(e2RigidBody->inverseInertiaTensor * Vector3::Cross(relativeB, info.manifold.normal), relativeB);
                float angularEffect = Vector3::Dot(inertiaA + inertiaB, info.manifold.normal);

                float restitution = e1Particle->restitution * e2Particle->restitution;
                float j = (-(1.0f + restitution) * impulseForce) / (totalMass + angularEffect);

                Vector3 fullImpulse = info.manifold.normal * j;

                e1Particle->ApplyLinearImpulse(-fullImpulse);
                e2Particle->ApplyLinearImpulse(fullImpulse);

                e1RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeA, -fullImpulse));
                e2RigidBody->ApplyAngularImpuse(Vector3::Cross(relativeB, fullImpulse));
            }

            // friction
            for (const Vector3& contactPoint : info.manifold.contactPoints)
            {
                Vector3 relativeA = contactPoint - e1Transform->position;
                Vector3 relativeB = contactPoint - e2Transform->position;

                Vector3 angVelocityA = Vector3::Cross(e1RigidBody->angularVelocity, relativeA);
                Vector3 angVelocityB = Vector3::Cross(e2RigidBody->angularVelocity, relativeB);

                Vector3 fullVelocityA = e1Particle->linearVelocity + angVelocityA;
                Vector3 fullVelocityB = e2Particle->linearVelocity + angVelocityB;
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
                Vector3 angInertiaA = Vector3::Cross(e1RigidBody->inverseInertiaTensor * rAct, relativeA);
                Vector3 angInertiaB = Vector3::Cross(e2RigidBody->inverseInertiaTensor * rBct, relativeB);
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
        for (const CollisionInfo& info : collisions)
        {
            Transform* e1Transform = scene.GetComponent<Transform>(info.entityA);
            Particle* e1Particle = scene.GetComponent<Particle>(info.entityA);
            Transform* e2Transform = scene.GetComponent<Transform>(info.entityB);
            Particle* e2Particle = scene.GetComponent<Particle>(info.entityB);

            float totalMass = e1Particle->inverseMass + e2Particle->inverseMass;
            if (totalMass == 0)
                continue;

            float penetration = (info.manifold.penetration - POSITION_PENETRATION_THRESHOLD) > 0.0f ? (info.manifold.penetration - POSITION_PENETRATION_THRESHOLD) : 0.0f;
            Vector3 correction = info.manifold.normal * (penetration * POSITION_CORRECTION_PERCENT / totalMass);

            e1Transform->position -= correction * e1Particle->inverseMass;
            e2Transform->position += correction * e2Particle->inverseMass;
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
