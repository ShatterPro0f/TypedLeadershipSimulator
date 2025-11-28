#include "ProximityDetection.h"
#include "Collision.h"
#include <algorithm>

namespace TLS {

bool ProximityDetector::isNPCInProximity(
    const NPC& npc,
    const Player& player,
    float proximityRange
) {
    float dist = npc.getPosition().distance(player.position);
    return dist <= proximityRange;
}

std::vector<int> ProximityDetector::getNPCsInProximity(
    const NPCRegistry& registry,
    const Player& player,
    float proximityRange
) {
    std::vector<int> nearby;

    auto allNPCs = registry.getAllNPCs();
    for (const auto& npc : allNPCs) {
        if (npc && isNPCInProximity(*npc, player, proximityRange)) {
            nearby.push_back(npc->getId());
        }
    }

    return nearby;
}

float ProximityDetector::getDistanceToPlayer(const NPC& npc, const Player& player) {
    return npc.getPosition().distance(player.position);
}

bool ProximityDetector::canNPCDetectPlayer(
    const NPC& npc,
    const Player& player,
    const World& world,
    float visionRange
) {
    // Check if within vision range
    float dist = getDistanceToPlayer(npc, player);
    if (dist > visionRange) {
        return false;
    }

    // Check line of sight
    return hasLineOfSight(npc.getPosition(), player.position, world);
}

std::vector<int> ProximityDetector::getNPCsSortedByDistance(
    const NPCRegistry& registry,
    const Player& player
) {
    std::vector<std::pair<int, float>> distancePairs;

    auto allNPCs = registry.getAllNPCs();
    for (const auto& npc : allNPCs) {
        if (npc) {
            float dist = getDistanceToPlayer(*npc, player);
            distancePairs.push_back({npc->getId(), dist});
        }
    }

    std::sort(distancePairs.begin(), distancePairs.end(),
        [](const auto& a, const auto& b) {
            return a.second < b.second;
        }
    );

    std::vector<int> result;
    for (const auto& pair : distancePairs) {
        result.push_back(pair.first);
    }

    return result;
}

bool ProximityDetector::hasLineOfSight(
    const Vector3& from,
    const Vector3& to,
    const World& world,
    float rayRadius
) {
    return CollisionDetector::raycastClear(from, to, world, rayRadius);
}

}  // namespace TLS
