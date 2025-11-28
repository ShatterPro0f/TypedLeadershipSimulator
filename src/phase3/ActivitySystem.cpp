#include "ActivitySystem.h"
#include "MovementConfig.h"

namespace TLS {

Activity ActivitySystem::determineNPCActivity(
    const NPC& npc,
    const WorldState& worldState,
    int currentTick
) {
    // If in conversation, return conversation state
    if (npc.getActivity() == Activity::IN_CONVERSATION) {
        return Activity::IN_CONVERSATION;
    }

    // Get game hour
    float hourOfDay = getGameHour(currentTick);

    // Time-based scheduling
    return getActivityByTimeOfDay(hourOfDay);
}

Vector3 ActivitySystem::getActivityDestination(
    const NPC& npc,
    Activity activity,
    const WaypointGraph& graph
) {
    switch (activity) {
        case Activity::IDLE:
            return npc.getPosition();  // Stay where we are

        case Activity::WORKING: {
            // Go to work location based on role
            std::string role = npc.getRole();
            if (role == "farmer") {
                auto waypointOpt = graph.getWaypoint("farm_south");
                if (waypointOpt) return *waypointOpt;
            } else if (role == "merchant") {
                auto waypointOpt = graph.getWaypoint("settlement_center");
                if (waypointOpt) return *waypointOpt;
            } else if (role == "priest") {
                auto waypointOpt = graph.getWaypoint("village_hall");
                if (waypointOpt) return *waypointOpt;
            }
            return npc.getPosition();
        }

        case Activity::RESTING:
            return npc.getHomeLocation();

        case Activity::PATROLLING: {
            // Patrol around waypoints
            auto waypointOpt = graph.getWaypoint("settlement_center");
            if (waypointOpt) return *waypointOpt;
            return npc.getPosition();
        }

        case Activity::IN_CONVERSATION:
            return npc.getPosition();  // Freeze in place

        default:
            return npc.getPosition();
    }
}

Activity ActivitySystem::getScheduledActivity(int tickOfDay) {
    float hourOfDay = getGameHour(tickOfDay);
    return getActivityByTimeOfDay(hourOfDay);
}

bool ActivitySystem::shouldTransitionActivity(
    const NPC& npc,
    Activity currentActivity,
    int currentTick
) {
    Activity scheduledActivity = getScheduledActivity(currentTick % TICKS_PER_GAME_DAY);
    return scheduledActivity != currentActivity && currentActivity != Activity::IN_CONVERSATION;
}

float ActivitySystem::getGameHour(int tick) {
    int tickOfDay = tick % TICKS_PER_GAME_DAY;
    return (float)tickOfDay / TICKS_PER_GAME_HOUR;
}

float ActivitySystem::getGameMinute(int tick) {
    int tickOfHour = tick % TICKS_PER_GAME_HOUR;
    return (float)tickOfHour / TICKS_PER_GAME_MINUTE;
}

Activity ActivitySystem::getActivityByTimeOfDay(float hourOfDay) {
    // Simple daily schedule
    if (hourOfDay < 6.0f) {
        return Activity::RESTING;  // Night
    } else if (hourOfDay < 12.0f) {
        return Activity::WORKING;  // Morning work
    } else if (hourOfDay < 18.0f) {
        return Activity::WORKING;  // Afternoon work
    } else if (hourOfDay < 22.0f) {
        return Activity::IDLE;  // Evening leisure
    } else {
        return Activity::RESTING;  // Night
    }
}

}  // namespace TLS
