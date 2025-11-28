#include "Collision.h"
#include "MovementConfig.h"
#include <algorithm>
#include <cmath>

namespace TLS {

bool CollisionDetector::sphereAABBCollision(
    const Vector3& sphereCenter,
    float sphereRadius,
    const AABB& box
) {
    // Find the closest point on the box to the sphere center
    Vector3 closest = closestPointOnAABB(sphereCenter, box);
    
    // Check if that point is within the sphere radius
    float distSq = sphereCenter.distanceSquared(closest);
    return distSq <= sphereRadius * sphereRadius;
}

bool CollisionDetector::sphereSphereCollision(
    const Vector3& center1,
    float radius1,
    const Vector3& center2,
    float radius2
) {
    float distSq = center1.distanceSquared(center2);
    float radiusSum = radius1 + radius2;
    return distSq <= radiusSum * radiusSum;
}

std::optional<Vector3> CollisionDetector::sweepSphereCollision(
    const Vector3& from,
    const Vector3& to,
    float radius,
    const World& world
) {
    // Check if starting point is colliding
    for (const auto& obstacle : world.getObstacles()) {
        if (sphereAABBCollision(from, radius, obstacle)) {
            return from;
        }
    }

    // Check if ending point is colliding
    for (const auto& obstacle : world.getObstacles()) {
        if (sphereAABBCollision(to, radius, obstacle)) {
            return binarySearchCollisionPoint(from, to, radius, world);
        }
    }

    return std::nullopt;  // No collision
}

bool CollisionDetector::isPathClear(
    const Vector3& from,
    const Vector3& to,
    float radius,
    const World& world
) {
    return !sweepSphereCollision(from, to, radius, world).has_value();
}

Vector3 CollisionDetector::closestPointOnAABB(const Vector3& point, const AABB& box) {
    return Vector3(
        std::max(box.min.x, std::min(point.x, box.max.x)),
        std::max(box.min.y, std::min(point.y, box.max.y)),
        std::max(box.min.z, std::min(point.z, box.max.z))
    );
}

bool CollisionDetector::raycastClear(
    const Vector3& from,
    const Vector3& to,
    const World& world,
    float rayRadius
) {
    // Use sweep sphere collision with ray radius
    return isPathClear(from, to, rayRadius, world);
}

Vector3 CollisionDetector::binarySearchCollisionPoint(
    const Vector3& from,
    const Vector3& to,
    float radius,
    const World& world,
    int iterations
) {
    Vector3 start = from;
    Vector3 end = to;

    for (int i = 0; i < iterations; ++i) {
        Vector3 mid = start + (end - start) * 0.5f;
        
        bool midCollides = false;
        for (const auto& obstacle : world.getObstacles()) {
            if (sphereAABBCollision(mid, radius, obstacle)) {
                midCollides = true;
                break;
            }
        }

        if (midCollides) {
            end = mid;
        } else {
            start = mid;
        }
    }

    return start;
}

}  // namespace TLS
