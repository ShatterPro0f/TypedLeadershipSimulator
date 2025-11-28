#include "MovementController.h"
#include "Core.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace TLS {

MovementController& MovementController::getInstance() {
    static MovementController instance;
    return instance;
}

void MovementController::initialize(PathfindingEngine& pathfindingEngine) {
    pathfindingEngine_ = &pathfindingEngine;
    reset();
}

void MovementController::reset() {
    positionHistory_.clear();
    recoveryAttempts_.clear();
    ticksStuck_.clear();
    npcsMoving_ = 0;
    npcsStuck_ = 0;
    averageDistanceToDestination_ = 0.0f;
}

// ==================== MOVEMENT UPDATES ====================

void MovementController::updateMovement(NPC& npc, int currentTick,
                                        const std::vector<Vector3>& nearbyNPCs) {
    if (!pathfindingEngine_ || !npc.movementState) {
        return;
    }

    auto& state = *npc.movementState;

    if (!state.isMoving) {
        return;
    }

    // Check and recover from stuck state
    if (checkAndRecoverStuck(npc, currentTick)) {
        if (debugEnabled_) {
            std::cout << "NPC " << npc.getId() << " is stuck at "
                      << state.currentPosition << std::endl;
        }
        ++npcsStuck_;
    }

    // Recalculate path if needed
    recalculatePathIfNeeded(npc, currentTick);

    if (state.currentPath.empty()) {
        state.isMoving = false;
        return;
    }

    // Get next waypoint
    auto waypoint = pathfindingEngine_->getNextWaypoint(state.currentPath,
                                                        state.currentWaypoint);
    if (!waypoint) {
        state.isMoving = false;
        state.arriveTick = currentTick;
        return;
    }

    // Calculate velocity
    Vector3 direction = normalize(*waypoint - state.currentPosition);
    Vector3 pathVelocity = direction * state.movementSpeed;

    // Apply collision avoidance
    Vector3 avoidanceForce = pathfindingEngine_->calculateSeparationForce(
        state.currentPosition, nearbyNPCs);
    Vector3 finalVelocity = pathfindingEngine_->combinedVelocity(
        pathVelocity, avoidanceForce);

    // Apply deceleration near waypoint
    float distToWaypoint = distance(state.currentPosition, *waypoint);
    finalVelocity = applyDeceleration(finalVelocity, distToWaypoint);

    // Update position
    state.currentPosition = state.currentPosition + finalVelocity;
    state.velocity = finalVelocity;

    // Update position history for stuck detection
    updatePositionHistory(npc.getId(), state.currentPosition);

    // Progress waypoint if reached
    if (pathfindingEngine_->hasReachedWaypoint(state.currentPosition, *waypoint)) {
        progressWaypoint(npc);
    }

    ++npcsMoving_;
}

void MovementController::setDestination(NPC& npc, const Vector3& destination, int currentTick) {
    if (!pathfindingEngine_) {
        return;
    }

    if (!npc.movementState) {
        npc.movementState = std::make_shared<MovementState>();
    }

    auto& state = *npc.movementState;
    state.destination = destination;
    state.lastTargetPosition = destination;
    state.currentWaypoint = 0;
    state.isMoving = true;
    state.lastPathCalcTick = currentTick;
    state.velocity = Vector3(0, 0, 0);

    // Compute initial path
    state.currentPath = pathfindingEngine_->computePath(state.currentPosition, destination);

    resetStuckTracking(npc);
}

void MovementController::stopMovement(NPC& npc) {
    if (npc.movementState) {
        npc.movementState->isMoving = false;
        npc.movementState->currentPath.clear();
        npc.movementState->velocity = Vector3(0, 0, 0);
    }
}

bool MovementController::isMoving(const NPC& npc) const {
    return npc.movementState && npc.movementState->isMoving &&
           !npc.movementState->currentPath.empty();
}

const MovementState& MovementController::getMovementState(const NPC& npc) const {
    static MovementState dummy;
    return npc.movementState ? *npc.movementState : dummy;
}

// ==================== VELOCITY & ACCELERATION ====================

Vector3 MovementController::calculateVelocity(const NPC& npc, float deltaTime) const {
    if (!npc.movementState || !npc.movementState->isMoving) {
        return Vector3(0, 0, 0);
    }

    const auto& state = *npc.movementState;
    auto waypoint = pathfindingEngine_->getNextWaypoint(state.currentPath,
                                                        state.currentWaypoint);
    if (!waypoint) {
        return Vector3(0, 0, 0);
    }

    Vector3 direction = normalize(*waypoint - state.currentPosition);
    return direction * state.movementSpeed;
}

Vector3 MovementController::applyAcceleration(const Vector3& currentVelocity,
                                             const Vector3& targetVelocity,
                                             float acceleration) const {
    Vector3 diff = targetVelocity - currentVelocity;
    float diffLength = length(diff);

    if (diffLength < acceleration) {
        return targetVelocity;
    }

    return currentVelocity + normalize(diff) * acceleration;
}

Vector3 MovementController::applyDeceleration(const Vector3& velocity, float distanceToWaypoint,
                                             float decelerationRange) const {
    if (distanceToWaypoint < decelerationRange && distanceToWaypoint > 0.1f) {
        float factor = distanceToWaypoint / decelerationRange;
        return velocity * factor;
    }
    return velocity;
}

// ==================== WAYPOINT PROGRESSION ====================

bool MovementController::progressWaypoint(NPC& npc) {
    if (!npc.movementState) {
        return false;
    }

    auto& state = *npc.movementState;

    if (state.currentWaypoint >= state.currentPath.size() - 1) {
        state.isMoving = false;
        return false;
    }

    ++state.currentWaypoint;
    return true;
}

float MovementController::getDistanceToNextWaypoint(const NPC& npc) const {
    if (!npc.movementState) {
        return -1.0f;
    }

    const auto& state = *npc.movementState;
    auto waypoint = pathfindingEngine_->getNextWaypoint(state.currentPath,
                                                        state.currentWaypoint);
    if (!waypoint) {
        return -1.0f;
    }

    return distance(state.currentPosition, *waypoint);
}

float MovementController::getDistanceToDestination(const NPC& npc) const {
    if (!npc.movementState) {
        return -1.0f;
    }

    return distance(npc.movementState->currentPosition, npc.movementState->destination);
}

// ==================== STUCK DETECTION & RECOVERY ====================

bool MovementController::checkAndRecoverStuck(NPC& npc, int currentTick) {
    if (!npc.movementState) {
        return false;
    }

    auto& state = *npc.movementState;
    auto& posHistory = positionHistory_[npc.getId()];

    if (posHistory.size() < POSITION_HISTORY_SIZE) {
        return false;
    }

    // Check if stuck
    if (pathfindingEngine_->isNPCStuck(posHistory, state.destination)) {
        ++ticksStuck_[npc.getId()];

        if (ticksStuck_[npc.getId()] >= STUCK_THRESHOLD_TICKS) {
            int& attempts = recoveryAttempts_[npc.getId()];
            attempts++;

            if (attempts < MAX_RECOVERY_ATTEMPTS) {
                auto newDest = pathfindingEngine_->getRecoveryDestination(
                    state.destination, state.currentPosition, attempts);

                if (newDest) {
                    state.destination = *newDest;
                    state.currentPath = pathfindingEngine_->computePath(
                        state.currentPosition, state.destination);
                    state.currentWaypoint = 0;
                    ticksStuck_[npc.getId()] = 0;
                }
            } else {
                // Give up
                stopMovement(npc);
                return true;
            }
        }
        return true;
    } else {
        ticksStuck_[npc.getId()] = 0;
    }

    return false;
}

int MovementController::getRecoveryAttempts(const NPC& npc) const {
    auto it = recoveryAttempts_.find(npc.getId());
    return it != recoveryAttempts_.end() ? it->second : 0;
}

void MovementController::resetStuckTracking(NPC& npc) {
    ticksStuck_[npc.getId()] = 0;
    recoveryAttempts_[npc.getId()] = 0;
    positionHistory_[npc.getId()].clear();
}

// ==================== COLLISION AVOIDANCE ====================

Vector3 MovementController::applyCollisionAvoidance(const Vector3& position,
                                                   const Vector3& velocity,
                                                   const std::vector<Vector3>& nearbyNPCs,
                                                   float avoidanceRadius) {
    Vector3 avoidanceForce = pathfindingEngine_->calculateSeparationForce(
        position, nearbyNPCs, avoidanceRadius);

    Vector3 finalVelocity = pathfindingEngine_->combinedVelocity(
        velocity, avoidanceForce);

    return position + finalVelocity;
}

bool MovementController::wouldCollide(const Vector3& from, const Vector3& to,
                                     float npcRadius) const {
    // Simple sphere collision check
    // Could be enhanced with more sophisticated collision detection
    float moveDistance = distance(from, to);
    return moveDistance < npcRadius * 2.0f;
}

Vector3 MovementController::resolveCollision(const Vector3& position,
                                            const Vector3& otherPosition) const {
    // Move perpendicular to collision
    Vector3 diff = position - otherPosition;
    if (length(diff) < 0.01f) {
        return position;
    }

    Vector3 direction = normalize(diff);
    return position + direction * 1.0f;  // Push away by 1 unit
}

// ==================== PATH MANAGEMENT ====================

void MovementController::clearPath(NPC& npc) {
    if (npc.movementState) {
        npc.movementState->currentPath.clear();
        npc.movementState->currentWaypoint = 0;
        npc.movementState->isMoving = false;
    }
}

std::vector<Vector3> MovementController::getCurrentPath(const NPC& npc) const {
    if (!npc.movementState) {
        return {};
    }
    return npc.movementState->currentPath;
}

size_t MovementController::getCurrentWaypoint(const NPC& npc) const {
    if (!npc.movementState) {
        return 0;
    }
    return npc.movementState->currentWaypoint;
}

bool MovementController::hasValidPath(const NPC& npc) const {
    return npc.movementState && !npc.movementState->currentPath.empty() &&
           npc.movementState->isMoving;
}

// ==================== METRICS & DEBUGGING ====================

MovementMetrics MovementController::getMetrics() const {
    MovementMetrics metrics;
    metrics.npcsMoving = npcsMoving_;
    metrics.npcsStuck = npcsStuck_;
    metrics.averageDistanceToDestination = averageDistanceToDestination_;
    metrics.pathRecalculations = 0;  // Would track in full implementation
    metrics.stuckRecoveries = recoveryAttempts_.size();
    return metrics;
}

std::optional<std::string> MovementController::getDebugInfo(int npcId) const {
    if (positionHistory_.find(npcId) == positionHistory_.end()) {
        return std::nullopt;
    }

    std::string info = "NPC " + std::to_string(npcId) + " movement: ";
    auto it = recoveryAttempts_.find(npcId);
    int attempts = (it != recoveryAttempts_.end()) ? it->second : 0;
    info += "attempts=" + std::to_string(attempts);
    return info;
}

void MovementController::setDebugEnabled(bool enabled) {
    debugEnabled_ = enabled;
}

// ==================== PRIVATE HELPERS ====================

void MovementController::recalculatePathIfNeeded(NPC& npc, int currentTick) {
    if (!npc.movementState || !pathfindingEngine_) {
        return;
    }

    auto& state = *npc.movementState;

    if (pathfindingEngine_->shouldRecalcPath(state.currentPosition, state.destination,
                                            state.lastPathCalcTick, currentTick,
                                            state.lastTargetPosition)) {
        state.currentPath = pathfindingEngine_->computePath(state.currentPosition,
                                                           state.destination);
        state.currentWaypoint = 0;
        state.lastPathCalcTick = currentTick;
        state.lastTargetPosition = state.destination;
    }
}

void MovementController::updatePositionHistory(int npcId, const Vector3& position) {
    auto& history = positionHistory_[npcId];
    history.push_back(position);

    if (history.size() > POSITION_HISTORY_SIZE) {
        history.erase(history.begin());
    }
}

Vector3 MovementController::getNextWaypointDirection(const NPC& npc) const {
    if (!npc.movementState) {
        return Vector3(0, 0, 0);
    }

    const auto& state = *npc.movementState;
    auto waypoint = pathfindingEngine_->getNextWaypoint(state.currentPath,
                                                        state.currentWaypoint);
    if (!waypoint) {
        return Vector3(0, 0, 0);
    }

    return normalize(*waypoint - state.currentPosition);
}

} // namespace TLS
