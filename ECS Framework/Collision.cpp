#include "Collision.h"

bool Collision::Intersects(SphereCollider c1, SphereCollider c2)
{
    float combinedRadii = c1.radius + c2.radius;
    return (c1.center - c2.center).sqrMagnitude() < combinedRadii * combinedRadii;
}
