#ifndef WORLD_H
#define WORLD_H

#include "Vector3.h"
#include <vector>
#include <fstream>

namespace TLS {

// Axis-Aligned Bounding Box for collision and containment
struct AABB {
    Vector3 min;
    Vector3 max;

    AABB() : min(0, 0, 0), max(1, 1, 1) {}
    AABB(const Vector3& min_, const Vector3& max_) : min(min_), max(max_) {}

    // Get center and size
    Vector3 center() const {
        return (min + max) * 0.5f;
    }

    Vector3 size() const {
        return max - min;
    }

    float volume() const {
        Vector3 s = size();
        return s.x * s.y * s.z;
    }

    // Intersection tests
    bool intersects(const AABB& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y ||
                 max.z < other.min.z || min.z > other.max.z);
    }

    bool contains(const Vector3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    // Sphere-AABB collision
    bool intersectsSphere(const Vector3& center, float radius) const {
        Vector3 closest = center.clamp(min, max);
        float distSq = center.distanceSquared(closest);
        return distSq <= radius * radius;
    }

    // Serialization
    bool toBinary(std::ofstream& file) const;
    static AABB fromBinary(std::ifstream& file);
};

// World bounds and obstacles
class World {
private:
    AABB worldBounds;
    std::vector<AABB> obstacles;

public:
    World();
    World(const Vector3& min, const Vector3& max);
    World(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);

    // Accessors
    const AABB& getBounds() const { return worldBounds; }
    const std::vector<AABB>& getObstacles() const { return obstacles; }

    // Obstacle management
    void addObstacle(const AABB& obstacle);
    void removeObstacle(size_t index);
    void clearObstacles();

    // Spatial queries
    bool isPointValid(const Vector3& point) const;
    bool isPointInWorld(const Vector3& point) const;
    bool isPathClear(const Vector3& from, const Vector3& to, float radius) const;

    // Serialization
    bool toBinary(std::ofstream& file) const;
    static World fromBinary(std::ifstream& file);
};

// Math utilities for spatial calculations
namespace MathUtils {
    constexpr float PI = 3.14159265358979f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float EPSILON = 1e-6f;

    inline bool approxEqual(float a, float b, float epsilon = EPSILON) {
        return std::fabs(a - b) < epsilon;
    }

    // Normalize angle to [0, 2π)
    inline float normalizeAngle(float angle) {
        while (angle < 0.0f) angle += TWO_PI;
        while (angle >= TWO_PI) angle -= TWO_PI;
        return angle;
    }

    // Angle between two vectors (in radians)
    inline float angleBetween(const Vector3& a, const Vector3& b) {
        float cosAngle = a.dot(b) / (a.magnitude() * b.magnitude() + EPSILON);
        cosAngle = std::max(-1.0f, std::min(1.0f, cosAngle));
        return std::acos(cosAngle);
    }
}

}  // namespace TLS

#endif  // WORLD_H
