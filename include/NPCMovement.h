#ifndef NPCMOVEMENT_H
#define NPCMOVEMENT_H

#include "Vector3.h"
#include "Core.h"
#include "Pathfinding.h"
#include "World.h"
#include <vector>

namespace TLS {

// NPC movement and navigation system
class NPCMovement {
private:
    static constexpr int PATH_RECALC_INTERVAL = 5;  // Recalculate every 5 ticks
    static constexpr float PATH_RECALC_DISTANCE = 10.0f;  // Or if target moved >10 units
    static constexpr float STUCK_DETECTION_TIME = 2.0f;  // 30 ticks at 60fps
    static constexpr float STUCK_DETECTION_DISTANCE = 1.0f;  // units moved

public:
    // Determine what activity NPC should perform
    static Activity determineActivity(
        const NPC& npc,
        const WorldState& worldState,
        const WaypointGraph& graph
    );

    // Update NPC position based on current activity
    static void updateNPCPosition(
        NPC& npc,
        const Activity activity,
        const Vector3& destination,
        float deltaTime,
        const World& world,
        const WaypointGraph& graph,
        int currentTick
    );

    // Check if should recalculate path
    static bool shouldRecalcPath(
        const NPC& npc,
        const Vector3& destination,
        int lastPathCalcTick,
        int currentTick
    );

    // Detect if NPC is stuck
    static bool isNPCStuck(
        const NPC& npc,
        const std::vector<Vector3>& positionHistory
    );

    // Recover from stuck state
    static void recoverFromStuck(
        NPC& npc,
        const Vector3& destination,
        const WaypointGraph& graph,
        const World& world
    );

private:
    // Move NPC one step toward destination
    static Vector3 moveTowardDestination(
        const Vector3& from,
        const Vector3& to,
        float moveSpeed,
        float deltaTime,
        const World& world
    );
};

}  // namespace TLS

#endif  // NPCMOVEMENT_H
