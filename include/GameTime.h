#pragma once

#include <string>
#include <cstdint>

namespace TLS {

/**
 * @class GameTime
 * @brief Utilities for game time tracking and conversion
 *
 * Converts between ticks, minutes, hours, days, and seasons.
 * One tick = one simulation step (typically 16ms at 60 FPS, or variable based on deltaTime)
 */
class GameTime {
public:
    // Time unit conversions
    static constexpr float TICKS_PER_GAME_MINUTE = 10.0f;  // Tunable
    static constexpr int TICKS_PER_HOUR = 600;
    static constexpr int TICKS_PER_DAY = 14400;            // 24 hours
    static constexpr int TICKS_PER_MONTH = 432000;         // ~30 days
    static constexpr int TICKS_PER_SEASON = 1296000;       // ~90 days
    static constexpr int TICKS_PER_YEAR = 5184000;         // 4 seasons

    // Get game time from tick count
    static int getGameMinute(int tickCounter);
    static int getGameHour(int tickCounter);
    static int getGameDay(int tickCounter);
    static int getGameMonth(int tickCounter);
    static int getGameSeason(int tickCounter);
    static int getGameYear(int tickCounter);

    // Format time as readable string
    static std::string formatTime(int tickCounter);  // "14:32 (Day 42, Season 2, Year 1)"
    static std::string formatShortTime(int tickCounter);  // "14:32"
    static std::string formatDate(int tickCounter);  // "Day 42, Season 2, Year 1"

    // Get season name
    static std::string getSeasonName(int season);

    // Calculate time elapsed
    static int getTicksSinceEvent(int currentTick, int eventTick);
    static int getTicksUntilNextDay(int currentTick);
    static int getTicksUntilNextMonth(int currentTick);
    static int getTicksUntilNextSeason(int currentTick);

    // Check time conditions
    static bool isNewDay(int previousTick, int currentTick);
    static bool isNewMonth(int previousTick, int currentTick);
    static bool isNewSeason(int previousTick, int currentTick);
    static bool isNewYear(int previousTick, int currentTick);

    // NPC age progression
    static int getAgeAtTick(int birthTick, int currentTick);  // Returns age in game years
    static bool hasNPCHadBirthday(int birthTick, int previousTick, int currentTick);

private:
    GameTime() = delete;
};

}  // namespace TLS
