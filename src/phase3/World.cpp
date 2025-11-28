#include "World.h"
#include <algorithm>
#include <cstdint>

namespace TLS {

// AABB Serialization
bool AABB::toBinary(std::ofstream& file) const {
    if (!file.write(reinterpret_cast<const char*>(&min.x), sizeof(min.x))) return false;
    if (!file.write(reinterpret_cast<const char*>(&min.y), sizeof(min.y))) return false;
    if (!file.write(reinterpret_cast<const char*>(&min.z), sizeof(min.z))) return false;
    if (!file.write(reinterpret_cast<const char*>(&max.x), sizeof(max.x))) return false;
    if (!file.write(reinterpret_cast<const char*>(&max.y), sizeof(max.y))) return false;
    if (!file.write(reinterpret_cast<const char*>(&max.z), sizeof(max.z))) return false;
    return true;
}

AABB AABB::fromBinary(std::ifstream& file) {
    AABB box;
    file.read(reinterpret_cast<char*>(&box.min.x), sizeof(box.min.x));
    file.read(reinterpret_cast<char*>(&box.min.y), sizeof(box.min.y));
    file.read(reinterpret_cast<char*>(&box.min.z), sizeof(box.min.z));
    file.read(reinterpret_cast<char*>(&box.max.x), sizeof(box.max.x));
    file.read(reinterpret_cast<char*>(&box.max.y), sizeof(box.max.y));
    file.read(reinterpret_cast<char*>(&box.max.z), sizeof(box.max.z));
    return box;
}

// World Implementation
World::World()
    : worldBounds(Vector3(-100, -100, -10), Vector3(100, 100, 10)) {}

World::World(const Vector3& min, const Vector3& max)
    : worldBounds(min, max) {}

World::World(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max)
    : worldBounds(Vector3(x_min, y_min, z_min), Vector3(x_max, y_max, z_max)) {}

void World::addObstacle(const AABB& obstacle) {
    obstacles.push_back(obstacle);
}

void World::removeObstacle(size_t index) {
    if (index < obstacles.size()) {
        obstacles.erase(obstacles.begin() + index);
    }
}

void World::clearObstacles() {
    obstacles.clear();
}

bool World::isPointInWorld(const Vector3& point) const {
    return worldBounds.contains(point);
}

bool World::isPointValid(const Vector3& point) const {
    // Point must be inside world bounds
    if (!isPointInWorld(point)) {
        return false;
    }

    // Point must not be inside any obstacle
    for (const auto& obstacle : obstacles) {
        if (obstacle.contains(point)) {
            return false;
        }
    }

    return true;
}

bool World::isPathClear(const Vector3& from, const Vector3& to, float radius) const {
    // Check if moving sphere from 'from' to 'to' with given radius collides with any obstacle
    for (const auto& obstacle : obstacles) {
        // Simple check: sphere intersects obstacle at any point along path
        // Use conservative approach: check multiple points along path
        int numSamples = 5;
        for (int i = 0; i <= numSamples; ++i) {
            float t = static_cast<float>(i) / numSamples;
            Vector3 samplePoint = from + (to - from) * t;
            
            if (obstacle.intersectsSphere(samplePoint, radius)) {
                return false;
            }
        }
    }

    return true;
}

bool World::toBinary(std::ofstream& file) const {
    if (!worldBounds.toBinary(file)) return false;
    
    uint32_t obstacleCount = static_cast<uint32_t>(obstacles.size());
    if (!file.write(reinterpret_cast<const char*>(&obstacleCount), sizeof(obstacleCount))) {
        return false;
    }

    for (const auto& obstacle : obstacles) {
        if (!obstacle.toBinary(file)) return false;
    }

    return true;
}

World World::fromBinary(std::ifstream& file) {
    AABB bounds = AABB::fromBinary(file);
    World world(bounds.min, bounds.max);

    uint32_t obstacleCount = 0;
    file.read(reinterpret_cast<char*>(&obstacleCount), sizeof(obstacleCount));

    for (uint32_t i = 0; i < obstacleCount; ++i) {
        AABB obstacle = AABB::fromBinary(file);
        world.addObstacle(obstacle);
    }

    return world;
}

}  // namespace TLS
