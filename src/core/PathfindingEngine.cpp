#include "PathfindingEngine.h"
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <chrono>
#include <iostream>
#include <random>

namespace TLS {

// ==================== GRID STRUCTURE ====================

class Grid {
public:
    struct Cell {
        std::vector<int> npcIds;
    };

    Grid(float width, float height, float depth, float cellSize)
        : width_(width), height_(height), depth_(depth), cellSize_(cellSize) {}

    void addNPC(int npcId, const Vector3& pos) {
        Vector3 coord = gridCoord(pos);
        cells_[coord].npcIds.push_back(npcId);
    }

    void removeNPC(int npcId, const Vector3& pos) {
        Vector3 coord = gridCoord(pos);
        if (cells_.count(coord)) {
            auto& ids = cells_[coord].npcIds;
            ids.erase(std::remove(ids.begin(), ids.end(), npcId), ids.end());
        }
    }

    void updateNPC(int npcId, const Vector3& oldPos, const Vector3& newPos) {
        removeNPC(npcId, oldPos);
        addNPC(npcId, newPos);
    }

    std::vector<int> getNearbyNPCs(const Vector3& pos, float radius) const {
        std::vector<int> result;
        Vector3 centerCell = gridCoord(pos);
        int cellRadius = static_cast<int>(std::ceil(radius / cellSize_));

        for (int dx = -cellRadius; dx <= cellRadius; ++dx) {
            for (int dy = -cellRadius; dy <= cellRadius; ++dy) {
                for (int dz = -cellRadius; dz <= cellRadius; ++dz) {
                    Vector3 cell = centerCell + Vector3(dx, dy, dz);
                    if (cells_.count(cell)) {
                        for (int npcId : cells_.at(cell).npcIds) {
                            result.push_back(npcId);
                        }
                    }
                }
            }
        }
        return result;
    }

    void addObstacle(const Vector3& pos, float radius) {
        obstacles_.emplace_back(pos, radius);
    }

    void clearObstacles() {
        obstacles_.clear();
    }

    bool isWalkable(const Vector3& pos) const {
        // Check bounds
        if (pos.x < 0 || pos.x > width_ || pos.y < 0 || pos.y > height_ ||
            pos.z < 0 || pos.z > depth_) {
            return false;
        }

        // Check obstacles
        for (const auto& obstacle : obstacles_) {
            if (distance(pos, obstacle.first) < obstacle.second) {
                return false;
            }
        }
        return true;
    }

private:
    Vector3 gridCoord(const Vector3& pos) const {
        return Vector3(
            std::floor(pos.x / cellSize_),
            std::floor(pos.y / cellSize_),
            std::floor(pos.z / cellSize_)
        );
    }

    float width_, height_, depth_, cellSize_;
    std::map<Vector3, Cell> cells_;
    std::vector<std::pair<Vector3, float>> obstacles_;  // (position, radius)
};

// Hash function for Vector3 in unordered containers (outside TLS namespace)
} // Close TLS namespace for std specialization

namespace std {
    template<>
    struct hash<TLS::Vector3> {
        size_t operator()(const TLS::Vector3& v) const {
            size_t h1 = std::hash<int>{}(static_cast<int>(v.x));
            size_t h2 = std::hash<int>{}(static_cast<int>(v.y));
            size_t h3 = std::hash<int>{}(static_cast<int>(v.z));
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

namespace TLS {  // Re-enter TLS namespace

// ==================== PATHFINDING ENGINE ====================

PathfindingEngine& PathfindingEngine::getInstance() {
    static PathfindingEngine instance;
    return instance;
}

void PathfindingEngine::initialize(float width, float height, float depth, float cellSize) {
    worldWidth_ = width;
    worldHeight_ = height;
    worldDepth_ = depth;
    cellSize_ = cellSize;
    
    grid_ = std::make_unique<Grid>(width, height, depth, cellSize);
    clearPathCache();
}

void PathfindingEngine::reset() {
    clearPathCache();
    cacheHits_ = 0;
    cacheMisses_ = 0;
    lastNodesExpanded_ = 0;
    lastComputeTimeMs_ = 0.0f;
    if (grid_) {
        grid_->clearObstacles();
    }
}

// ==================== A* IMPLEMENTATION ====================

float PathfindingEngine::heuristic(const Vector3& a, const Vector3& b) const {
    // Manhattan distance + vertical weight
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) + 
           std::abs(a.z - b.z) * VERTICAL_WEIGHT;
}

std::vector<Vector3> PathfindingEngine::getNeighbors(const Vector3& pos) const {
    std::vector<Vector3> neighbors;
    
    // 8-directional horizontal + vertical neighbors
    float dx[] = {-1, -1, -1, 0, 0, 1, 1, 1, 0, 0};
    float dy[] = {-1, 0, 1, -1, 1, -1, 0, 1, 0, 0};
    float dz[] = {0, 0, 0, 0, 0, 0, 0, 0, -1, 1};

    for (int i = 0; i < 10; ++i) {
        Vector3 neighbor = pos + Vector3(dx[i], dy[i], dz[i]);
        if (grid_->isWalkable(neighbor)) {
            neighbors.push_back(neighbor);
        }
    }
    return neighbors;
}

float PathfindingEngine::edgeCost(const Vector3& from, const Vector3& to) const {
    Vector3 diff = to - from;
    float horizontal = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    float vertical = std::abs(diff.z);
    
    // Diagonal or cardinal horizontal movement
    if (horizontal > 1.0f) {
        return 1.414f;  // Diagonal
    } else if (vertical > 0.5f) {
        return 1.5f;    // Vertical
    }
    return 1.0f;       // Cardinal
}

std::vector<Vector3> PathfindingEngine::reconstructPath(
    const std::map<Vector3, Vector3>& cameFrom, const Vector3& current) const {
    std::vector<Vector3> path;
    Vector3 c = current;
    path.push_back(c);
    
    while (cameFrom.count(c)) {
        c = cameFrom.at(c);
        path.push_back(c);
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<Vector3> PathfindingEngine::computePath(const Vector3& start, const Vector3& goal) {
    // Check cache first
    auto cacheKey = std::make_pair(start, goal);
    if (pathCache_.count(cacheKey)) {
        ++cacheHits_;
        return pathCache_[cacheKey];
    }
    ++cacheMisses_;

    auto startTime = std::chrono::high_resolution_clock::now();

    // A* implementation
    std::map<Vector3, float> gScore;
    std::map<Vector3, float> fScore;
    std::map<Vector3, Vector3> cameFrom;
    
    // Priority queue: (fScore, position)
    auto cmp = [&fScore](const Vector3& a, const Vector3& b) {
        return fScore[a] > fScore[b];
    };
    std::priority_queue<Vector3, std::vector<Vector3>, decltype(cmp)> openSet(cmp);

    std::unordered_set<Vector3> closedSet;
    std::unordered_set<Vector3> inOpenSet;

    gScore[start] = 0.0f;
    fScore[start] = heuristic(start, goal);
    openSet.push(start);
    inOpenSet.insert(start);

    size_t nodesExpanded = 0;

    while (!openSet.empty() && nodesExpanded < MAX_NODES_EXPANDED) {
        Vector3 current = openSet.top();
        openSet.pop();
        inOpenSet.erase(current);
        ++nodesExpanded;

        // Goal reached
        if (distance(current, goal) < 0.5f) {
            lastNodesExpanded_ = nodesExpanded;
            auto endTime = std::chrono::high_resolution_clock::now();
            lastComputeTimeMs_ = 
                std::chrono::duration<float, std::milli>(endTime - startTime).count();

            auto path = reconstructPath(cameFrom, current);
            
            // Cache the result
            if (pathCache_.size() >= MAX_CACHE_SIZE) {
                auto oldKey = cacheOrder_.front();
                cacheOrder_.pop();
                pathCache_.erase(oldKey);
            }
            pathCache_[cacheKey] = path;
            cacheOrder_.push(cacheKey);

            return path;
        }

        closedSet.insert(current);

        for (const Vector3& neighbor : getNeighbors(current)) {
            if (closedSet.count(neighbor)) {
                continue;
            }

            float tentativeGScore = gScore[current] + edgeCost(current, neighbor);

            if (!inOpenSet.count(neighbor) || tentativeGScore < gScore[neighbor]) {
                cameFrom[neighbor] = current;
                gScore[neighbor] = tentativeGScore;
                fScore[neighbor] = gScore[neighbor] + heuristic(neighbor, goal);

                if (!inOpenSet.count(neighbor)) {
                    openSet.push(neighbor);
                    inOpenSet.insert(neighbor);
                }
            }
        }
    }

    // No path found
    lastNodesExpanded_ = nodesExpanded;
    auto endTime = std::chrono::high_resolution_clock::now();
    lastComputeTimeMs_ = 
        std::chrono::duration<float, std::milli>(endTime - startTime).count();

    std::vector<Vector3> emptyPath;
    pathCache_[cacheKey] = emptyPath;
    cacheOrder_.push(cacheKey);

    return emptyPath;
}

// ==================== LAZY RECALCULATION ====================

bool PathfindingEngine::shouldRecalcPath(const Vector3& currentPos, const Vector3& targetPos,
                                         int lastCalcTick, int currentTick,
                                         const Vector3& lastTargetPos) const {
    // Recalculate every 5 ticks
    if (currentTick - lastCalcTick >= 5) {
        return true;
    }

    // Recalculate if target moved >10 units
    if (distance(targetPos, lastTargetPos) > 10.0f) {
        return true;
    }

    return false;
}

std::optional<Vector3> PathfindingEngine::getNextWaypoint(const std::vector<Vector3>& path,
                                                          size_t currentWaypoint) const {
    if (currentWaypoint >= path.size()) {
        return std::nullopt;
    }
    return path[currentWaypoint];
}

bool PathfindingEngine::hasReachedWaypoint(const Vector3& position, const Vector3& waypoint,
                                          float arrivalTolerance) const {
    return distance(position, waypoint) < arrivalTolerance;
}

// ==================== MOVEMENT SPEED ====================

float PathfindingEngine::calculateEffectiveSpeed(float baseSpeed, float mobilityModifier,
                                                float terrainModifier) const {
    return baseSpeed * mobilityModifier * terrainModifier;
}

float PathfindingEngine::getBaseSpeedForRole(const std::string& role) const {
    // Speed in units per tick (60 ticks/sec baseline)
    const std::map<std::string, float> roleSpeeds = {
        {"warrior", 1.0f},
        {"scout", 1.2f},
        {"merchant", 0.7f},
        {"farmer", 0.6f},
        {"priest", 0.5f},
        {"child", 0.4f},
        {"elder", 0.3f},
    };

    auto it = roleSpeeds.find(role);
    return it != roleSpeeds.end() ? it->second : 0.6f;  // Default to farmer pace
}

float PathfindingEngine::getTerrainModifier(const Vector3& position) const {
    // Placeholder: could read from terrain map later
    // For now, return 1.0 (flat ground)
    return 1.0f;
}

// ==================== STUCK DETECTION ====================

bool PathfindingEngine::isNPCStuck(const std::vector<Vector3>& positionHistory,
                                  const Vector3& destination) const {
    if (positionHistory.size() < 30) {
        return false;
    }

    float distanceThen = distance(positionHistory[0], destination);
    float distanceNow = distance(positionHistory.back(), destination);

    return (distanceThen - distanceNow) < 3.0f;
}

std::optional<Vector3> PathfindingEngine::getRecoveryDestination(const Vector3& currentGoal,
                                                                const Vector3& currentPos,
                                                                int recoveryAttempt) const {
    if (recoveryAttempt == 1) {
        // Offset destination slightly
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dis(-5.0f, 5.0f);
        
        Vector3 offset(dis(gen), dis(gen), 0.0f);
        return currentGoal + offset;
    } else if (recoveryAttempt == 2) {
        // Return alternative position (could be teleport target)
        // For now, just offset more
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<float> dis(-10.0f, 10.0f);
        
        Vector3 offset(dis(gen), dis(gen), 0.0f);
        return currentPos + offset;
    }

    return std::nullopt;  // Max attempts exceeded
}

// ==================== COLLISION AVOIDANCE ====================

Vector3 PathfindingEngine::calculateSeparationForce(const Vector3& npcPosition,
                                                    const std::vector<Vector3>& nearbyPositions,
                                                    float avoidanceRadius,
                                                    float avoidanceStrength) const {
    Vector3 separationForce(0, 0, 0);

    for (const Vector3& neighborPos : nearbyPositions) {
        float dist = distance(npcPosition, neighborPos);

        if (dist < avoidanceRadius && dist > 0.01f) {
            Vector3 direction = normalize(npcPosition - neighborPos);
            separationForce = separationForce + (direction / (dist * dist));
        }
    }

    if (length(separationForce) > 0.01f) {
        separationForce = normalize(separationForce) * avoidanceStrength;
    }

    return separationForce;
}

Vector3 PathfindingEngine::combinedVelocity(const Vector3& pathVelocity,
                                           const Vector3& avoidanceForce,
                                           float avoidanceWeight) const {
    return pathVelocity * (1.0f - avoidanceWeight) + avoidanceForce * avoidanceWeight;
}

// ==================== SPATIAL PARTITIONING ====================

void PathfindingEngine::addNPCToGrid(int npcId, const Vector3& position) {
    if (grid_) {
        grid_->addNPC(npcId, position);
    }
}

void PathfindingEngine::updateNPCInGrid(int npcId, const Vector3& oldPosition,
                                       const Vector3& newPosition) {
    if (grid_) {
        grid_->updateNPC(npcId, oldPosition, newPosition);
    }
}

void PathfindingEngine::removeNPCFromGrid(int npcId, const Vector3& position) {
    if (grid_) {
        grid_->removeNPC(npcId, position);
    }
}

std::vector<int> PathfindingEngine::queryNearbyNPCs(const Vector3& position, float radius) const {
    if (!grid_) {
        return {};
    }
    return grid_->getNearbyNPCs(position, radius);
}

// ==================== CACHING ====================

void PathfindingEngine::clearPathCache() {
    pathCache_.clear();
    while (!cacheOrder_.empty()) {
        cacheOrder_.pop();
    }
    cacheHits_ = 0;
    cacheMisses_ = 0;
}

size_t PathfindingEngine::getCacheSize() const {
    return pathCache_.size();
}

float PathfindingEngine::getCacheHitRate() const {
    size_t total = cacheHits_ + cacheMisses_;
    if (total == 0) return 0.0f;
    return (static_cast<float>(cacheHits_) / total) * 100.0f;
}

// ==================== METRICS ====================

PathfindingMetrics PathfindingEngine::getMetrics() const {
    PathfindingMetrics metrics;
    metrics.cacheHits = cacheHits_;
    metrics.cacheMisses = cacheMisses_;
    metrics.lastNodesExpanded = lastNodesExpanded_;
    metrics.lastComputeTimeMs = lastComputeTimeMs_;
    metrics.activePaths = pathCache_.size();
    metrics.averagePathLength = pathCache_.empty() ? 0.0f : 
        std::accumulate(pathCache_.begin(), pathCache_.end(), 0.0f,
            [](float sum, const auto& pair) { return sum + pair.second.size(); }) 
        / pathCache_.size();
    return metrics;
}

bool PathfindingEngine::isWalkable(const Vector3& position) const {
    return grid_ ? grid_->isWalkable(position) : true;
}

void PathfindingEngine::setObstacle(const Vector3& position, float radius) {
    if (grid_) {
        grid_->addObstacle(position, radius);
    }
}

void PathfindingEngine::clearObstacles() {
    if (grid_) {
        grid_->clearObstacles();
    }
}

} // namespace TLS
