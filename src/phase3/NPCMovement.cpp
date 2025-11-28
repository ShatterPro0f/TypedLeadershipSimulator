#include "NPCMovement.h"
#include "ActivitySystem.h"
#include "Collision.h"
#include "MovementConfig.h"
#include <algorithm>

namespace TLS {

Activity NPCMovement::determineActivity(
    const NPC& npc,
    const WorldState& worldState,
    const WaypointGraph& graph
) {
    // If in conversation, stay in conversation
    if (npc.getActivity() == Activity::IN_CONVERSATION) {
        return Activity::IN_CONVERSATION;
    }

    // Default activity based on time of day
    return Activity::IDLE;
}

void NPCMovement::updateNPCPosition(
    NPC& npc,
    const Activity activity,
    const Vector3& destination,
    float deltaTime,
    const World& world,
    const WaypointGraph& graph,
    int currentTick
) {
    if (deltaTime <= 0) return;

    // Don't move if in conversation or idle
    if (activity == Activity::IN_CONVERSATION || activity == Activity::IDLE) {
        return;
    }

    // Recalculate path if needed
    if (shouldRecalcPath(npc, destination, npc.lastPathCalcTick, currentTick)) {
        if (npc.currentPath) delete npc.currentPath;
        npc.currentPath = new Path(NPCNavigator::planPath(npc.getPosition(), destination, graph, world));
        npc.lastPathCalcTick = currentTick;
    }

    // If no path, create one
    if (!npc.currentPath || npc.currentPath->waypoints.empty()) {
        if (npc.currentPath) delete npc.currentPath;
        npc.currentPath = new Path(NPCNavigator::planPath(npc.getPosition(), destination, graph, world));
        npc.lastPathCalcTick = currentTick;
    }

    // Move along path
    float moveSpeed = (activity == Activity::PATROLLING) ? 
        MovementConfig::NPC_BASE_SPEED : 
        MovementConfig::NPC_BASE_SPEED;

    Vector3 newPosition = NPCNavigator::getNextPosition(
        *npc.currentPath,
        npc.getPosition(),
        moveSpeed,
        deltaTime
    );

    // Collision detection
    if (CollisionDetector::isPathClear(
        npc.getPosition(),
        newPosition,
        MovementConfig::NPC_COLLISION_RADIUS,
        world
    )) {
        npc.setPosition(newPosition);
    } else {
        // Stuck or collision, try to recover
        if (isNPCStuck(npc, npc.positionHistory)) {
            recoverFromStuck(npc, destination, graph, world);
        }
    }

    // Track position for stuck detection
    npc.positionHistory.push_back(npc.getPosition());
    if (npc.positionHistory.size() > 30) {
        npc.positionHistory.erase(npc.positionHistory.begin());
    }
}

bool NPCMovement::shouldRecalcPath(
    const NPC& npc,
    const Vector3& destination,
    int lastPathCalcTick,
    int currentTick
) {
    // Recalculate every N ticks
    if (currentTick - lastPathCalcTick >= PATH_RECALC_INTERVAL) {
        return true;
    }

    // Or if destination moved significantly
    if (npc.currentPath && npc.currentPath->waypoints.size() > 0) {
        Vector3 lastDest = npc.currentPath->waypoints.back();
        if (lastDest.distance(destination) > PATH_RECALC_DISTANCE) {
            return true;
        }
    }

    return false;
}

bool NPCMovement::isNPCStuck(
    const NPC& npc,
    const std::vector<Vector3>& positionHistory
) {
    if (positionHistory.size() < 30) {
        return false;  // Not enough data
    }

    // Check distance moved in last 30 positions
    Vector3 oldPos = positionHistory[positionHistory.size() - 30];
    Vector3 newPos = positionHistory.back();
    float distMoved = oldPos.distance(newPos);

    return distMoved < STUCK_DETECTION_DISTANCE;
}

void NPCMovement::recoverFromStuck(
    NPC& npc,
    const Vector3& destination,
    const WaypointGraph& graph,
    const World& world
) {
    // Force path recalculation
    if (npc.currentPath) delete npc.currentPath;
    npc.currentPath = new Path(NPCNavigator::planPath(npc.getPosition(), destination, graph, world));
    npc.stuckAttempts++;

    // If stuck too many times, give up
    if (npc.stuckAttempts > 3) {
        npc.setActivity(Activity::IDLE);
        npc.stuckAttempts = 0;
    }
}

}  // namespace TLS
