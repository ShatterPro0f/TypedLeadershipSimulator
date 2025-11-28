#ifndef ENUMS_H
#define ENUMS_H

#include <string>
#include <cstdint>

namespace TLS {

// NPC Mood states (representing emotional valence/arousal)
enum class Mood : uint8_t {
    NEUTRAL = 0,
    HAPPY = 1,
    CONTENT = 2,
    ANXIOUS = 3,
    ANGRY = 4,
    FEARFUL = 5,
    SAD = 6,
    EXCITED = 7
};

// NPC Skills and Specializations
enum class Skill : uint8_t {
    AGRICULTURE = 0,
    DIPLOMACY = 1,
    COMBAT = 2,
    EDUCATION = 3,
    CRAFTSMANSHIP = 4,
    LEADERSHIP = 5,
    SPIRITUALITY = 6,
    COMMERCE = 7,
    MEDICINE = 8,
    STEALTH = 9
};

// Advisor Specialization
enum class Specialty : uint8_t {
    POLITICS = 0,
    MILITARY = 1,
    CULTURE = 2,
    RELIGION = 3
};

// Faction/Player Alignment
enum class Alignment : uint8_t {
    PLAYER_FRIENDLY = 0,
    NEUTRAL = 1,
    HOSTILE = 2
};

// Event Types (from Equations.txt and events_crises.txt)
enum class EventType : uint8_t {
    ENVIRONMENTAL = 0,  // Weather, famine, natural disaster
    POLITICAL = 1,      // Faction action, rebellion, leadership change
    ECONOMIC = 2,       // Resource shortage, trade, inflation
    SOCIAL = 3,         // Immigration, marriage, conflict
    RELIGIOUS = 4,      // Schism, conversion, doctrinal conflict
    MILITARY = 5        // Invasion, skirmish, military event
};

// NPC Activity State
enum class Activity : uint8_t {
    IDLE = 0,
    PATROLLING = 1,
    WORKING = 2,
    RESTING = 3,
    IN_CONVERSATION = 4,
    TRAVELING = 5,
    SEARCHING = 6,
    FLEEING = 7
};

// Advisor Decision Agenda
enum class Agenda : uint8_t {
    SHORT_TERM = 0,
    LONG_TERM = 1
};

// Advisor Strategy Style
enum class StrategyStyle : uint8_t {
    MANIPULATIVE = 0,
    HONEST = 1,
    PERSUASIVE = 2
};

// Problem severity/resolution state
enum class ProblemState : uint8_t {
    UNRESOLVED = 0,
    IN_DIALOGUE = 1,
    ACKNOWLEDGED = 2,
    RESOLVED = 3,
    PERSISTENT = 4
};

// String conversion functions
std::string moodToString(Mood mood);
Mood stringToMood(const std::string& str);

std::string skillToString(Skill skill);
Skill stringToSkill(const std::string& str);

std::string specialtyToString(Specialty specialty);
Specialty stringToSpecialty(const std::string& str);

std::string alignmentToString(Alignment alignment);
Alignment stringToAlignment(const std::string& str);

std::string eventTypeToString(EventType type);
EventType stringToEventType(const std::string& str);

std::string activityToString(Activity activity);
Activity stringToActivity(const std::string& str);

std::string agendaToString(Agenda agenda);
Agenda stringToAgenda(const std::string& str);

std::string strategyStyleToString(StrategyStyle style);
StrategyStyle stringToStrategyStyle(const std::string& str);

std::string problemStateToString(ProblemState state);
ProblemState stringToProblemState(const std::string& str);

}  // namespace TLS

#endif  // ENUMS_H
