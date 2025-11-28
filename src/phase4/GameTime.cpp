#include "GameTime.h"
#include <sstream>
#include <iomanip>

using namespace TLS;

int GameTime::getGameMinute(int tickCounter)
{
    int totalMinutes = static_cast<int>(tickCounter / TICKS_PER_GAME_MINUTE);
    return totalMinutes % 60;
}

int GameTime::getGameHour(int tickCounter)
{
    int totalMinutes = static_cast<int>(tickCounter / TICKS_PER_GAME_MINUTE);
    return (totalMinutes / 60) % 24;
}

int GameTime::getGameDay(int tickCounter)
{
    int totalMinutes = static_cast<int>(tickCounter / TICKS_PER_GAME_MINUTE);
    return totalMinutes / 1440;
}

int GameTime::getGameMonth(int tickCounter)
{
    int totalMinutes = static_cast<int>(tickCounter / TICKS_PER_GAME_MINUTE);
    int days = totalMinutes / 1440;
    return (days / 30) % 12;
}

int GameTime::getGameSeason(int tickCounter)
{
    int totalMinutes = static_cast<int>(tickCounter / TICKS_PER_GAME_MINUTE);
    int days = totalMinutes / 1440;
    return (days / 90) % 4;
}

int GameTime::getGameYear(int tickCounter)
{
    int totalMinutes = static_cast<int>(tickCounter / TICKS_PER_GAME_MINUTE);
    int days = totalMinutes / 1440;
    return days / 360;
}

std::string GameTime::formatTime(int tickCounter)
{
    std::ostringstream oss;

    int hour = getGameHour(tickCounter);
    int minute = getGameMinute(tickCounter);
    int day = getGameDay(tickCounter);
    int season = getGameSeason(tickCounter);
    int year = getGameYear(tickCounter);

    oss << std::setfill('0') << std::setw(2) << hour << ":"
        << std::setfill('0') << std::setw(2) << minute
        << " (Day " << day << ", " << getSeasonName(season)
        << ", Year " << year << ")";

    return oss.str();
}

std::string GameTime::formatShortTime(int tickCounter)
{
    std::ostringstream oss;
    int hour = getGameHour(tickCounter);
    int minute = getGameMinute(tickCounter);

    oss << std::setfill('0') << std::setw(2) << hour << ":"
        << std::setfill('0') << std::setw(2) << minute;

    return oss.str();
}

std::string GameTime::formatDate(int tickCounter)
{
    std::ostringstream oss;
    int day = getGameDay(tickCounter);
    int season = getGameSeason(tickCounter);
    int year = getGameYear(tickCounter);

    oss << "Day " << day << ", " << getSeasonName(season) << ", Year " << year;
    return oss.str();
}

std::string GameTime::getSeasonName(int season)
{
    switch (season % 4)
    {
    case 0: return "Spring";
    case 1: return "Summer";
    case 2: return "Autumn";
    case 3: return "Winter";
    default: return "Unknown";
    }
}

int GameTime::getTicksSinceEvent(int currentTick, int eventTick)
{
    return currentTick - eventTick;
}

int GameTime::getTicksUntilNextDay(int currentTick)
{
    int currentMinute = static_cast<int>(currentTick / TICKS_PER_GAME_MINUTE) % 1440;
    return (1440 - currentMinute) * static_cast<int>(TICKS_PER_GAME_MINUTE);
}

int GameTime::getTicksUntilNextMonth(int currentTick)
{
    int totalMinutes = static_cast<int>(currentTick / TICKS_PER_GAME_MINUTE);
    int days = totalMinutes / 1440;
    int daysInMonth = 30;
    int dayOfMonth = days % daysInMonth;
    int daysUntilNextMonth = daysInMonth - dayOfMonth;

    return daysUntilNextMonth * 1440 * static_cast<int>(TICKS_PER_GAME_MINUTE);
}

int GameTime::getTicksUntilNextSeason(int currentTick)
{
    int totalMinutes = static_cast<int>(currentTick / TICKS_PER_GAME_MINUTE);
    int days = totalMinutes / 1440;
    int daysInSeason = 90;
    int dayOfSeason = days % daysInSeason;
    int daysUntilNextSeason = daysInSeason - dayOfSeason;

    return daysUntilNextSeason * 1440 * static_cast<int>(TICKS_PER_GAME_MINUTE);
}

bool GameTime::isNewDay(int previousTick, int currentTick)
{
    return getGameDay(previousTick) != getGameDay(currentTick);
}

bool GameTime::isNewMonth(int previousTick, int currentTick)
{
    return getGameMonth(previousTick) != getGameMonth(currentTick);
}

bool GameTime::isNewSeason(int previousTick, int currentTick)
{
    return getGameSeason(previousTick) != getGameSeason(currentTick);
}

bool GameTime::isNewYear(int previousTick, int currentTick)
{
    return getGameYear(previousTick) != getGameYear(currentTick);
}

int GameTime::getAgeAtTick(int birthTick, int currentTick)
{
    int ageInTicks = currentTick - birthTick;
    return ageInTicks / TICKS_PER_YEAR;
}

bool GameTime::hasNPCHadBirthday(int birthTick, int previousTick, int currentTick)
{
    int previousAge = getAgeAtTick(birthTick, previousTick);
    int currentAge = getAgeAtTick(birthTick, currentTick);
    return previousAge != currentAge;
}
