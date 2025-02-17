#include "PhysicsHelper.h"
#include "ECSScene.h"
#include "Components.h"
#include "MeshLoader.h"
#include "AABBTree.h"
#include <vector>

void PhysicsHelper::CreateCloth(ECSScene& scene, AABBTree& tree, Vector3 center, unsigned int rows, unsigned int cols, float spacing, float stiffness, bool hasStructureSprings, bool hasShearingSprings, bool hasBendingSrings)
{
    float startEntity = scene.GetEntityCount() - 1;
    Entity currentEntityNum = 0;

    std::vector<Entity> clothEntities;
    clothEntities.resize(rows * cols);

    Vector3 startPosition = center + Vector3::Left * (rows / 2) * spacing + Vector3::Back * (cols / 2) * spacing;

    for (int y = 0; y < rows; y++)
    {
        for (int x = 0; x < cols; x++)
        {
            Vector3 pointPosition = startPosition + Vector3::Right * x * spacing + Vector3::Forward * y * spacing;
            float anchored = y == 0 || y == rows - 1 || x == 0 || x == cols - 1;
            float mass = anchored ? 0.0f : 1 / 0.05f;

            currentEntityNum = scene.CreateEntity();
            scene.AddComponent(
                currentEntityNum,
                Particle{ Vector3::Zero, Vector3::Zero, Vector3::Zero, mass, 0.2f }
            );
            scene.AddComponent(
                currentEntityNum,
                Transform{ pointPosition, Quaternion(), Vector3(0.05f, 0.05f, 0.05f) }
            );
            scene.AddComponent(
                currentEntityNum,
                RigidBody{ Vector3::Zero, Vector3::Zero, Vector3::Zero, Matrix3(Vector3::Zero) }
            );
            scene.AddComponent(
                currentEntityNum,
                Collider{ Point(pointPosition) }
            );
            scene.AddComponent(
                currentEntityNum,
                Mesh{ MeshLoader::GetMeshID("Sphere") }
            );

            tree.InsertEntity(currentEntityNum, AABB::FromPositionScale(pointPosition, Vector3(0.1f, 0.1f, 0.1f)), anchored);
            clothEntities[x + y * cols] = currentEntityNum;

            // structural springs
            if (hasStructureSprings)
            {
                if (x > 0 && y > 0)
                {
                    Entity a = clothEntities[x + y * cols];
                    Entity b = clothEntities[(x - 1) + y * cols];
                    Entity c = clothEntities[x + (y - 1) * cols];

                    Entity spring1 = scene.CreateEntity();
                    scene.AddComponent(
                        spring1,
                        Spring{ a, b, spacing, stiffness }
                    );

                    Entity spring2 = scene.CreateEntity();
                    scene.AddComponent(
                        spring2,
                        Spring{ a, c, spacing, stiffness }
                    );
                }
                if (x == 0 && y > 0)
                {
                    Entity a = clothEntities[x + y * cols];
                    Entity b = clothEntities[x + (y - 1) * cols];

                    Entity spring1 = scene.CreateEntity();
                    scene.AddComponent(
                        spring1,
                        Spring{ a, b, spacing, stiffness }
                    );
                }
                if (x > 0 && y == 0)
                {
                    Entity a = clothEntities[x + y * cols];
                    Entity b = clothEntities[(x - 1) + y * cols];

                    Entity spring1 = scene.CreateEntity();
                    scene.AddComponent(
                        spring1,
                        Spring{ a, b, spacing, stiffness }
                    );
                }
            }

            // shearing springs
            if (hasShearingSprings)
            {
                if (y > 0 && x < cols - 1)
                {
                    Entity a = clothEntities[x + y * cols];
                    Entity b = clothEntities[(x + 1) + (y - 1) * cols];

                    Entity spring1 = scene.CreateEntity();
                    scene.AddComponent(
                        spring1,
                        Spring{ a, b, spacing, stiffness }
                    );
                }

                if (y > 0 && x > 0)
                {
                    Entity a = clothEntities[x + y * cols];
                    Entity b = clothEntities[(x - 1) + (y - 1) * cols];

                    Entity spring1 = scene.CreateEntity();
                    scene.AddComponent(
                        spring1,
                        Spring{ a, b, spacing, stiffness }
                    );
                }
            }

            // bending springs
            if (hasBendingSrings)
            {
                if (x > 0 && x % 2 == 0)
                {
                    Entity a = clothEntities[x + y * cols];
                    Entity b = clothEntities[(x - 2) + y * cols];

                    Entity spring1 = scene.CreateEntity();
                    scene.AddComponent(
                        spring1,
                        Spring{ a, b, spacing, stiffness }
                    );
                }
                if (y > 0 && y % 2 == 0)
                {
                    Entity a = clothEntities[x + y * cols];
                    Entity b = clothEntities[x + (y - 2) * cols];

                    Entity spring1 = scene.CreateEntity();
                    scene.AddComponent(
                        spring1,
                        Spring{ a, b, spacing, stiffness }
                    );
                }
            }
        }
    }
}
