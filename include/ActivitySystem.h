#ifndef ACTIVITYSYSTEM_H
#define ACTIVITYSYSTEM_H

#include "Vector3.h"
#include "Core.h"
#include "Pathfinding.h"
#include "Enums.h"

namespace TLS {

// NPC activity scheduling and state management
class ActivitySystem {
public:
    // Determine what activity NPC should perform based on time and state
    static Activity determineNPCActivity(
        const NPC& npc,
        const WorldState& worldState,
        int currentTick
    );

    // Get destination based on activity and NPC role
    static Vector3 getActivityDestination(
        const NPC& npc,
        Activity activity,
        const WaypointGraph& graph
    );

    // Get time-of-day based on current tick
    static Activity getScheduledActivity(int tickOfDay);

    // Check if NPC should transition to a different activity
    static bool shouldTransitionActivity(
        const NPC& npc,
        Activity currentActivity,
        int currentTick
    );

    // Convert tick to game time (hour of day: 0-24)
    static float getGameHour(int tick);

    // Convert tick to game time (minute: 0-59)
    static float getGameMinute(int tick);

private:
    // Game time configuration
    static constexpr int TICKS_PER_GAME_DAY = 14400;
    static constexpr int TICKS_PER_GAME_HOUR = 600;
    static constexpr int TICKS_PER_GAME_MINUTE = 10;

    // Activity schedule (returns activity based on hour of day)
    static Activity getActivityByTimeOfDay(float hourOfDay);
};

}  // namespace TLS

#endif  // ACTIVITYSYSTEM_H
