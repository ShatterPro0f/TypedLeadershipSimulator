#ifndef PATHFINDING_ENGINE_H
#define PATHFINDING_ENGINE_H

#include <vector>
#include <queue>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <cmath>
#include "Vector3.h"

namespace TLS {

// Forward declarations
class Grid;
struct PathfindingMetrics;

/**
 * @brief Pathfinding engine implementing A* algorithm with caching and optimization
 * 
 * Implements:
 * - A* pathfinding with Manhattan + vertical heuristic
 * - LRU path caching (max 1000 cached paths)
 * - Lazy path recalculation (every 5 ticks or when target moves >10 units)
 * - Movement speed modifiers based on role and terrain
 * - Stuck detection and recovery strategies
 */
class PathfindingEngine {
public:
    // Singleton access
    static PathfindingEngine& getInstance();

    // ==================== INITIALIZATION ====================
    
    /**
     * @brief Initialize pathfinding engine with grid dimensions
     * @param width World width (X dimension)
     * @param height World height (Y dimension)
     * @param depth World depth (Z dimension)
     * @param cellSize Grid cell size for spatial partitioning (default 10.0)
     */
    void initialize(float width, float height, float depth, float cellSize = 10.0f);

    /**
     * @brief Reset engine state (clear caches, reset metrics)
     */
    void reset();

    // ==================== PATHFINDING ====================

    /**
     * @brief Compute path from start to goal using A* algorithm
     * @param start Starting position
     * @param goal Goal position
     * @return Vector of waypoints forming the path, empty if no path found
     */
    std::vector<Vector3> computePath(const Vector3& start, const Vector3& goal);

    /**
     * @brief Check if a path computation should be recalculated
     * @param currentPos NPC's current position
     * @param targetPos Current target position
     * @param lastCalcTick Tick when path was last calculated
     * @param currentTick Current game tick
     * @param lastTargetPos Target position at last calculation
     * @return True if path should be recalculated
     */
    bool shouldRecalcPath(const Vector3& currentPos, const Vector3& targetPos,
                         int lastCalcTick, int currentTick, const Vector3& lastTargetPos) const;

    /**
     * @brief Get next waypoint along path
     * @param path Vector of waypoints
     * @param currentWaypoint Current waypoint index
     * @return Next waypoint or empty if at end of path
     */
    std::optional<Vector3> getNextWaypoint(const std::vector<Vector3>& path,
                                           size_t currentWaypoint) const;

    /**
     * @brief Check if NPC has reached a waypoint
     * @param position Current position
     * @param waypoint Target waypoint
     * @param arrivalTolerance Distance threshold for arrival (default 1.0)
     * @return True if within arrival tolerance
     */
    bool hasReachedWaypoint(const Vector3& position, const Vector3& waypoint,
                           float arrivalTolerance = 1.0f) const;

    // ==================== MOVEMENT SPEED ====================

    /**
     * @brief Calculate effective movement speed based on NPC role and state
     * @param baseSpeed Base speed for NPC's role
     * @param mobilityModifier Health/fatigue modifier (0.3-1.0)
     * @param terrainModifier Terrain difficulty modifier (0.5-1.3)
     * @return Effective speed (units per tick)
     */
    float calculateEffectiveSpeed(float baseSpeed, float mobilityModifier,
                                 float terrainModifier) const;

    /**
     * @brief Get base speed for NPC role
     * @param role NPC role (farmer, warrior, merchant, etc.)
     * @return Base speed in units per tick
     */
    float getBaseSpeedForRole(const std::string& role) const;

    /**
     * @brief Get terrain modifier at position
     * @param position World position to check
     * @return Terrain modifier (0.5-1.3)
     */
    float getTerrainModifier(const Vector3& position) const;

    // ==================== STUCK DETECTION & RECOVERY ====================

    /**
     * @brief Detect if NPC is stuck (unable to reach goal)
     * @param positionHistory Last 30 ticks of positions
     * @param destination Goal position
     * @return True if stuck (no progress for 30 ticks)
     */
    bool isNPCStuck(const std::vector<Vector3>& positionHistory,
                   const Vector3& destination) const;

    /**
     * @brief Generate recovery destination for stuck NPC
     * @param currentGoal Original goal position
     * @param currentPos NPC's current position
     * @param recoveryAttempt Which recovery attempt (1=offset, 2=teleport)
     * @return New destination or empty if all recoveries exhausted
     */
    std::optional<Vector3> getRecoveryDestination(const Vector3& currentGoal,
                                                 const Vector3& currentPos,
                                                 int recoveryAttempt) const;

    // ==================== COLLISION AVOIDANCE ====================

    /**
     * @brief Calculate separation force for collision avoidance
     * @param npcPosition Current NPC position
     * @param nearbyPositions Positions of nearby NPCs
     * @param avoidanceRadius Personal space radius (default 2.0)
     * @param avoidanceStrength Force magnitude (default 0.3)
     * @return Separation force vector
     */
    Vector3 calculateSeparationForce(const Vector3& npcPosition,
                                    const std::vector<Vector3>& nearbyPositions,
                                    float avoidanceRadius = 2.0f,
                                    float avoidanceStrength = 0.3f) const;

    /**
     * @brief Combine path-following and avoidance velocities
     * @param pathVelocity Velocity toward next waypoint
     * @param avoidanceForce Separation/avoidance force
     * @param avoidanceWeight Weight for avoidance (0.0-1.0)
     * @return Combined velocity
     */
    Vector3 combinedVelocity(const Vector3& pathVelocity, const Vector3& avoidanceForce,
                            float avoidanceWeight = 0.3f) const;

    // ==================== SPATIAL PARTITIONING ====================

    /**
     * @brief Add NPC to spatial grid
     * @param npcId NPC identifier
     * @param position NPC position
     */
    void addNPCToGrid(int npcId, const Vector3& position);

    /**
     * @brief Update NPC position in spatial grid
     * @param npcId NPC identifier
     * @param oldPosition Previous position
     * @param newPosition New position
     */
    void updateNPCInGrid(int npcId, const Vector3& oldPosition, const Vector3& newPosition);

    /**
     * @brief Remove NPC from spatial grid
     * @param npcId NPC identifier
     * @param position NPC position
     */
    void removeNPCFromGrid(int npcId, const Vector3& position);

    /**
     * @brief Query nearby NPCs within radius
     * @param position Query center position
     * @param radius Search radius
     * @return Vector of NPC IDs within radius
     */
    std::vector<int> queryNearbyNPCs(const Vector3& position, float radius) const;

    // ==================== CACHING ====================

    /**
     * @brief Clear path cache
     */
    void clearPathCache();

    /**
     * @brief Get cache statistics
     * @return Number of cached paths
     */
    size_t getCacheSize() const;

    /**
     * @brief Get cache hit rate
     * @return Percentage of path queries that hit cache (0.0-100.0)
     */
    float getCacheHitRate() const;

    // ==================== METRICS & DEBUGGING ====================

    /**
     * @brief Get pathfinding metrics
     * @return Current metrics (nodes expanded, time, etc.)
     */
    PathfindingMetrics getMetrics() const;

    /**
     * @brief Check if position is walkable
     * @param position Position to check
     * @return True if position is accessible
     */
    bool isWalkable(const Vector3& position) const;

    /**
     * @brief Set obstacle at position
     * @param position Obstacle position
     * @param radius Obstacle radius
     */
    void setObstacle(const Vector3& position, float radius);

    /**
     * @brief Clear all obstacles
     */
    void clearObstacles();

private:
    // Private constructor (singleton)
    PathfindingEngine() = default;

    // A* helper functions
    float heuristic(const Vector3& a, const Vector3& b) const;
    std::vector<Vector3> getNeighbors(const Vector3& pos) const;
    float edgeCost(const Vector3& from, const Vector3& to) const;
    std::vector<Vector3> reconstructPath(const std::map<Vector3, Vector3>& cameFrom,
                                        const Vector3& current) const;

    // Grid helper functions
    Vector3 gridCoord(const Vector3& position) const;
    std::vector<Vector3> getNearbyGridCells(const Vector3& position, int cellRadius) const;

    // Member variables
    std::unique_ptr<Grid> grid_;
    
    // Path cache (LRU with max 1000 entries)
    std::map<std::pair<Vector3, Vector3>, std::vector<Vector3>> pathCache_;
    std::queue<std::pair<Vector3, Vector3>> cacheOrder_;
    
    // Metrics
    size_t cacheHits_ = 0;
    size_t cacheMisses_ = 0;
    size_t lastNodesExpanded_ = 0;
    float lastComputeTimeMs_ = 0.0f;

    // Configuration
    float worldWidth_ = 100.0f;
    float worldHeight_ = 100.0f;
    float worldDepth_ = 100.0f;
    float cellSize_ = 10.0f;
    
    // Constants
    static constexpr size_t MAX_CACHE_SIZE = 1000;
    static constexpr size_t MAX_NODES_EXPANDED = 500;
    static constexpr float VERTICAL_WEIGHT = 1.5f;
};

/**
 * @brief Pathfinding metrics for performance monitoring
 */
struct PathfindingMetrics {
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    size_t lastNodesExpanded = 0;
    float lastComputeTimeMs = 0.0f;
    size_t activePaths = 0;
    float averagePathLength = 0.0f;
};

} // namespace TLS

#endif // PATHFINDING_ENGINE_H
