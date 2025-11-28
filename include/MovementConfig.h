#ifndef MOVEMENTCONFIG_H
#define MOVEMENTCONFIG_H

namespace TLS {
namespace MovementConfig {

    // NPC Movement
    constexpr float NPC_BASE_SPEED = 3.0f;  // units per second
    constexpr float NPC_SPRINT_SPEED = 6.0f;
    constexpr float NPC_COLLISION_RADIUS = 0.5f;
    constexpr float NPC_BODY_HEIGHT = 1.8f;

    // Player Movement
    constexpr float PLAYER_BASE_SPEED = 5.0f;
    constexpr float PLAYER_SPRINT_SPEED = 10.0f;
    constexpr float PLAYER_COLLISION_RADIUS = 0.4f;
    constexpr float PLAYER_EYE_HEIGHT = 1.6f;

    // Pathfinding
    constexpr int PATH_RECALC_INTERVAL = 5;  // ticks
    constexpr float PATH_RECALC_DISTANCE = 10.0f;  // units
    constexpr float WAYPOINT_ARRIVAL_DISTANCE = 2.0f;

    // Proximity
    constexpr float PROXIMITY_RANGE = 5.0f;
    constexpr float VISION_RANGE = 50.0f;

    // World
    constexpr float STUCK_DETECTION_TIME = 2.0f;  // seconds (30 ticks at 60fps)
    constexpr float STUCK_DETECTION_DISTANCE = 1.0f;  // units moved

    // Time
    constexpr int TICKS_PER_GAME_DAY = 14400;
    constexpr int TICKS_PER_GAME_HOUR = 600;
    constexpr int TICKS_PER_GAME_MINUTE = 10;

    // Frame rate reference
    constexpr float TARGET_FPS = 60.0f;
    constexpr float DELTA_TIME = 1.0f / TARGET_FPS;  // ~0.0167 seconds

}  // namespace MovementConfig
}  // namespace TLS

#endif  // MOVEMENTCONFIG_H
