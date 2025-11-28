#ifndef PROXIMITYDETECTION_H
#define PROXIMITYDETECTION_H

#include "Vector3.h"
#include "Core.h"
#include "Registries.h"
#include "World.h"
#include "Player.h"
#include <vector>

namespace TLS {

// Forward declarations
class Player;

// Proximity detection for NPC-player interactions
class ProximityDetector {
private:
    static constexpr float DEFAULT_PROXIMITY_RANGE = 5.0f;
    static constexpr float DEFAULT_VISION_RANGE = 50.0f;

public:
    // Check if NPC is within proximity of player
    static bool isNPCInProximity(
        const NPC& npc,
        const Player& player,
        float proximityRange = DEFAULT_PROXIMITY_RANGE
    );

    // Get all NPCs within proximity of player
    static std::vector<int> getNPCsInProximity(
        const NPCRegistry& registry,
        const Player& player,
        float proximityRange = DEFAULT_PROXIMITY_RANGE
    );

    // Get distance between NPC and player
    static float getDistanceToPlayer(const NPC& npc, const Player& player);

    // Check if NPC can detect player (within vision range and line-of-sight)
    static bool canNPCDetectPlayer(
        const NPC& npc,
        const Player& player,
        const World& world,
        float visionRange = DEFAULT_VISION_RANGE
    );

    // Get sorted list of NPCs by distance to player
    static std::vector<int> getNPCsSortedByDistance(
        const NPCRegistry& registry,
        const Player& player
    );

private:
    // Line-of-sight check with ray casting
    static bool hasLineOfSight(
        const Vector3& from,
        const Vector3& to,
        const World& world,
        float rayRadius = 0.5f
    );
};

}  // namespace TLS

#endif  // PROXIMITYDETECTION_H
