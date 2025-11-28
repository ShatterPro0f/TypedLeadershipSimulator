#ifndef COLLISION_H
#define COLLISION_H

#include "Vector3.h"
#include "World.h"
#include <optional>

namespace TLS {

// Collision detection utilities
class CollisionDetector {
public:
    // Sphere-AABB collision
    static bool sphereAABBCollision(
        const Vector3& sphereCenter,
        float sphereRadius,
        const AABB& box
    );

    // Sphere-sphere collision
    static bool sphereSphereCollision(
        const Vector3& center1,
        float radius1,
        const Vector3& center2,
        float radius2
    );

    // Sweep sphere collision - returns collision point if hit
    static std::optional<Vector3> sweepSphereCollision(
        const Vector3& from,
        const Vector3& to,
        float radius,
        const World& world
    );

    // Check if path is clear for movement
    static bool isPathClear(
        const Vector3& from,
        const Vector3& to,
        float radius,
        const World& world
    );

    // Find closest point on AABB to given point
    static Vector3 closestPointOnAABB(const Vector3& point, const AABB& box);

    // Ray casting - check if line of sight is clear
    static bool raycastClear(
        const Vector3& from,
        const Vector3& to,
        const World& world,
        float rayRadius = 0.5f
    );

private:
    // Binary search for collision point along sweep path
    static Vector3 binarySearchCollisionPoint(
        const Vector3& from,
        const Vector3& to,
        float radius,
        const World& world,
        int iterations = 10
    );
};

}  // namespace TLS

#endif  // COLLISION_H
