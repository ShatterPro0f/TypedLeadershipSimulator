#ifndef MOVEMENT_CONTROLLER_H
#define MOVEMENT_CONTROLLER_H

#include <vector>
#include <memory>
#include <optional>
#include "Vector3.h"
#include "PathfindingEngine.h"

namespace TLS {

// Forward declarations
class NPC;
struct MovementState;
struct MovementMetrics;

/**
 * @brief Movement controller for NPC pathfinding and animation
 * 
 * Manages:
 * - NPC movement along computed paths
 * - Position updates with collision avoidance
 * - Smooth velocity interpolation
 * - Arrival detection and waypoint progression
 * - Stuck detection and recovery
 * - Performance metrics and debugging
 */
class MovementController {
public:
    // Singleton access
    static MovementController& getInstance();

    // ==================== INITIALIZATION ====================

    /**
     * @brief Initialize movement controller
     * @param pathfindingEngine Reference to pathfinding engine
     */
    void initialize(PathfindingEngine& pathfindingEngine);

    /**
     * @brief Reset controller state
     */
    void reset();

    // ==================== MOVEMENT UPDATES ====================

    /**
     * @brief Update NPC movement for one tick
     * @param npc NPC to update
     * @param currentTick Current game tick
     * @param nearbyNPCs Nearby NPC positions for collision avoidance
     */
    void updateMovement(NPC& npc, int currentTick, 
                       const std::vector<Vector3>& nearbyNPCs);

    /**
     * @brief Set NPC destination and initiate movement
     * @param npc NPC to move
     * @param destination Target location
     * @param currentTick Current game tick
     */
    void setDestination(NPC& npc, const Vector3& destination, int currentTick);

    /**
     * @brief Stop NPC movement immediately
     * @param npc NPC to stop
     */
    void stopMovement(NPC& npc);

    /**
     * @brief Check if NPC is moving
     * @param npc NPC to check
     * @return True if NPC has active path and is progressing
     */
    bool isMoving(const NPC& npc) const;

    /**
     * @brief Get NPC's current movement state
     * @param npc NPC to query
     * @return Current movement state (position, destination, path, etc.)
     */
    const MovementState& getMovementState(const NPC& npc) const;

    // ==================== VELOCITY & ACCELERATION ====================

    /**
     * @brief Calculate movement velocity for current tick
     * @param npc NPC to calculate velocity for
     * @param deltaTime Time delta (seconds)
     * @return Velocity vector (units per tick)
     */
    Vector3 calculateVelocity(const NPC& npc, float deltaTime = 1.0f / 60.0f) const;

    /**
     * @brief Apply smooth acceleration to velocity
     * @param currentVelocity Current velocity
     * @param targetVelocity Desired velocity
     * @param acceleration Rate of velocity change per tick
     * @return New velocity after acceleration
     */
    Vector3 applyAcceleration(const Vector3& currentVelocity, const Vector3& targetVelocity,
                             float acceleration = 0.1f) const;

    /**
     * @brief Apply deceleration when approaching waypoint
     * @param velocity Current velocity
     * @param distanceToWaypoint Distance to next waypoint
     * @param decelerationRange Distance over which to decelerate (default 5.0)
     * @return Reduced velocity
     */
    Vector3 applyDeceleration(const Vector3& velocity, float distanceToWaypoint,
                             float decelerationRange = 5.0f) const;

    // ==================== WAYPOINT PROGRESSION ====================

    /**
     * @brief Progress to next waypoint if current waypoint reached
     * @param npc NPC to progress
     * @return True if waypoint was advanced, false if already at destination
     */
    bool progressWaypoint(NPC& npc);

    /**
     * @brief Get distance to next waypoint
     * @param npc NPC to measure from
     * @return Distance to next waypoint or -1.0 if no path
     */
    float getDistanceToNextWaypoint(const NPC& npc) const;

    /**
     * @brief Get distance to final destination
     * @param npc NPC to measure from
     * @return Distance to final destination
     */
    float getDistanceToDestination(const NPC& npc) const;

    // ==================== STUCK DETECTION & RECOVERY ====================

    /**
     * @brief Check if NPC is stuck and attempt recovery if needed
     * @param npc NPC to check
     * @param currentTick Current game tick
     * @return True if stuck, false if progressing normally
     */
    bool checkAndRecoverStuck(NPC& npc, int currentTick);

    /**
     * @brief Get number of recovery attempts for NPC
     * @param npc NPC to query
     * @return Number of attempts (0 if not stuck)
     */
    int getRecoveryAttempts(const NPC& npc) const;

    /**
     * @brief Reset stuck tracking for NPC
     * @param npc NPC to reset
     */
    void resetStuckTracking(NPC& npc);

    // ==================== COLLISION AVOIDANCE ====================

    /**
     * @brief Apply collision avoidance to position
     * @param position Current position
     * @param velocity Current velocity
     * @param nearbyNPCs Positions of nearby NPCs
     * @param avoidanceRadius Personal space radius (default 2.0)
     * @return New position with avoidance applied
     */
    Vector3 applyCollisionAvoidance(const Vector3& position, const Vector3& velocity,
                                   const std::vector<Vector3>& nearbyNPCs,
                                   float avoidanceRadius = 2.0f);

    /**
     * @brief Check if movement would cause collision
     * @param from Current position
     * @param to Target position
     * @param NPCRadius NPC collision radius (default 0.5)
     * @return True if collision would occur
     */
    bool wouldCollide(const Vector3& from, const Vector3& to, float NPCRadius = 0.5f) const;

    /**
     * @brief Resolve collision by finding nearest safe position
     * @param position Position with collision
     * @param otherPosition Position of blocking entity
     * @return Safe position or original if no collision detected
     */
    Vector3 resolveCollision(const Vector3& position, const Vector3& otherPosition) const;

    // ==================== PATH MANAGEMENT ====================

    /**
     * @brief Clear path and reset NPC to idle
     * @param npc NPC to reset
     */
    void clearPath(NPC& npc);

    /**
     * @brief Get current path waypoints
     * @param npc NPC to query
     * @return Vector of waypoints in current path
     */
    std::vector<Vector3> getCurrentPath(const NPC& npc) const;

    /**
     * @brief Get current waypoint index
     * @param npc NPC to query
     * @return Index of current waypoint in path
     */
    size_t getCurrentWaypoint(const NPC& npc) const;

    /**
     * @brief Check if path is valid and active
     * @param npc NPC to check
     * @return True if NPC has valid path to follow
     */
    bool hasValidPath(const NPC& npc) const;

    // ==================== METRICS & DEBUGGING ====================

    /**
     * @brief Get movement metrics for all NPCs
     * @return Current metrics (NPCs moving, stuck, average distance, etc.)
     */
    MovementMetrics getMetrics() const;

    /**
     * @brief Get movement state for debugging
     * @param npcId NPC ID to query
     * @return Movement state details or empty if NPC not found
     */
    std::optional<std::string> getDebugInfo(int npcId) const;

    /**
     * @brief Enable/disable movement debugging output
     * @param enabled True to enable debug output
     */
    void setDebugEnabled(bool enabled);

private:
    // Private constructor (singleton)
    MovementController() = default;

    // Helper functions
    void recalculatePathIfNeeded(NPC& npc, int currentTick);
    void updatePositionHistory(int npcId, const Vector3& position);
    Vector3 getNextWaypointDirection(const NPC& npc) const;

    // Member variables
    PathfindingEngine* pathfindingEngine_ = nullptr;

    // Track position history for stuck detection (max 30 entries per NPC)
    std::map<int, std::vector<Vector3>> positionHistory_;
    
    // Track recovery attempts per NPC
    std::map<int, int> recoveryAttempts_;
    std::map<int, int> ticksStuck_;

    // Performance metrics
    size_t npcsMoving_ = 0;
    size_t npcsStuck_ = 0;
    float averageDistanceToDestination_ = 0.0f;

    // Debug
    bool debugEnabled_ = false;

    // Constants
    static constexpr size_t POSITION_HISTORY_SIZE = 30;
    static constexpr int STUCK_THRESHOLD_TICKS = 30;
    static constexpr int MAX_RECOVERY_ATTEMPTS = 3;
    static constexpr float STUCK_PROGRESS_THRESHOLD = 3.0f;
};

/**
 * @brief Movement state for an NPC
 */
struct MovementState {
    Vector3 currentPosition;
    Vector3 destination;
    Vector3 velocity;
    Vector3 lastTargetPosition;
    
    std::vector<Vector3> currentPath;
    size_t currentWaypoint = 0;
    
    float movementSpeed = 1.0f;
    float mobilityModifier = 1.0f;
    float terrainModifier = 1.0f;
    
    bool isMoving = false;
    int lastPathCalcTick = -1;
    int arriveTick = -1;
    
    // Stuck tracking
    int ticksStuck = 0;
    int recoveryAttempts = 0;
};

/**
 * @brief Movement metrics for performance monitoring
 */
struct MovementMetrics {
    size_t npcsMoving = 0;
    size_t npcsStuck = 0;
    size_t averagePathLength = 0;
    float averageDistanceToDestination = 0.0f;
    float averageMovementSpeed = 0.0f;
    size_t pathRecalculations = 0;
    size_t stuckRecoveries = 0;
};

} // namespace TLS

#endif // MOVEMENT_CONTROLLER_H
