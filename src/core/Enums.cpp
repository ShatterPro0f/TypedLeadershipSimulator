#include "Enums.h"

namespace TLS {

// ============================================================================
// Mood Conversions
// ============================================================================

std::string moodToString(Mood mood) {
    switch (mood) {
        case Mood::NEUTRAL:   return "NEUTRAL";
        case Mood::HAPPY:     return "HAPPY";
        case Mood::CONTENT:   return "CONTENT";
        case Mood::ANXIOUS:   return "ANXIOUS";
        case Mood::ANGRY:     return "ANGRY";
        case Mood::FEARFUL:   return "FEARFUL";
        case Mood::SAD:       return "SAD";
        case Mood::EXCITED:   return "EXCITED";
        default:              return "UNKNOWN";
    }
}

Mood stringToMood(const std::string& str) {
    if (str == "NEUTRAL")  return Mood::NEUTRAL;
    if (str == "HAPPY")    return Mood::HAPPY;
    if (str == "CONTENT")  return Mood::CONTENT;
    if (str == "ANXIOUS")  return Mood::ANXIOUS;
    if (str == "ANGRY")    return Mood::ANGRY;
    if (str == "FEARFUL")  return Mood::FEARFUL;
    if (str == "SAD")      return Mood::SAD;
    if (str == "EXCITED")  return Mood::EXCITED;
    return Mood::NEUTRAL;
}

// ============================================================================
// Skill Conversions
// ============================================================================

std::string skillToString(Skill skill) {
    switch (skill) {
        case Skill::AGRICULTURE:    return "AGRICULTURE";
        case Skill::DIPLOMACY:      return "DIPLOMACY";
        case Skill::COMBAT:         return "COMBAT";
        case Skill::EDUCATION:      return "EDUCATION";
        case Skill::CRAFTSMANSHIP:  return "CRAFTSMANSHIP";
        case Skill::LEADERSHIP:     return "LEADERSHIP";
        case Skill::SPIRITUALITY:   return "SPIRITUALITY";
        case Skill::COMMERCE:       return "COMMERCE";
        case Skill::MEDICINE:       return "MEDICINE";
        case Skill::STEALTH:        return "STEALTH";
        default:                    return "UNKNOWN";
    }
}

Skill stringToSkill(const std::string& str) {
    if (str == "AGRICULTURE")   return Skill::AGRICULTURE;
    if (str == "DIPLOMACY")     return Skill::DIPLOMACY;
    if (str == "COMBAT")        return Skill::COMBAT;
    if (str == "EDUCATION")     return Skill::EDUCATION;
    if (str == "CRAFTSMANSHIP") return Skill::CRAFTSMANSHIP;
    if (str == "LEADERSHIP")    return Skill::LEADERSHIP;
    if (str == "SPIRITUALITY")  return Skill::SPIRITUALITY;
    if (str == "COMMERCE")      return Skill::COMMERCE;
    if (str == "MEDICINE")      return Skill::MEDICINE;
    if (str == "STEALTH")       return Skill::STEALTH;
    return Skill::AGRICULTURE;
}

// ============================================================================
// Specialty Conversions
// ============================================================================

std::string specialtyToString(Specialty specialty) {
    switch (specialty) {
        case Specialty::POLITICS:   return "POLITICS";
        case Specialty::MILITARY:   return "MILITARY";
        case Specialty::CULTURE:    return "CULTURE";
        case Specialty::RELIGION:   return "RELIGION";
        default:                    return "UNKNOWN";
    }
}

Specialty stringToSpecialty(const std::string& str) {
    if (str == "POLITICS")  return Specialty::POLITICS;
    if (str == "MILITARY")  return Specialty::MILITARY;
    if (str == "CULTURE")   return Specialty::CULTURE;
    if (str == "RELIGION")  return Specialty::RELIGION;
    return Specialty::POLITICS;
}

// ============================================================================
// Alignment Conversions
// ============================================================================

std::string alignmentToString(Alignment alignment) {
    switch (alignment) {
        case Alignment::PLAYER_FRIENDLY: return "PLAYER_FRIENDLY";
        case Alignment::NEUTRAL:         return "NEUTRAL";
        case Alignment::HOSTILE:         return "HOSTILE";
        default:                         return "UNKNOWN";
    }
}

Alignment stringToAlignment(const std::string& str) {
    if (str == "PLAYER_FRIENDLY") return Alignment::PLAYER_FRIENDLY;
    if (str == "NEUTRAL")         return Alignment::NEUTRAL;
    if (str == "HOSTILE")         return Alignment::HOSTILE;
    return Alignment::NEUTRAL;
}

// ============================================================================
// EventType Conversions
// ============================================================================

std::string eventTypeToString(EventType type) {
    switch (type) {
        case EventType::ENVIRONMENTAL: return "ENVIRONMENTAL";
        case EventType::POLITICAL:     return "POLITICAL";
        case EventType::ECONOMIC:      return "ECONOMIC";
        case EventType::SOCIAL:        return "SOCIAL";
        case EventType::RELIGIOUS:     return "RELIGIOUS";
        case EventType::MILITARY:      return "MILITARY";
        default:                       return "UNKNOWN";
    }
}

EventType stringToEventType(const std::string& str) {
    if (str == "ENVIRONMENTAL") return EventType::ENVIRONMENTAL;
    if (str == "POLITICAL")     return EventType::POLITICAL;
    if (str == "ECONOMIC")      return EventType::ECONOMIC;
    if (str == "SOCIAL")        return EventType::SOCIAL;
    if (str == "RELIGIOUS")     return EventType::RELIGIOUS;
    if (str == "MILITARY")      return EventType::MILITARY;
    return EventType::ENVIRONMENTAL;
}

// ============================================================================
// Activity Conversions
// ============================================================================

std::string activityToString(Activity activity) {
    switch (activity) {
        case Activity::IDLE:            return "IDLE";
        case Activity::PATROLLING:      return "PATROLLING";
        case Activity::WORKING:         return "WORKING";
        case Activity::RESTING:         return "RESTING";
        case Activity::IN_CONVERSATION: return "IN_CONVERSATION";
        case Activity::TRAVELING:       return "TRAVELING";
        case Activity::SEARCHING:       return "SEARCHING";
        case Activity::FLEEING:         return "FLEEING";
        default:                        return "UNKNOWN";
    }
}

Activity stringToActivity(const std::string& str) {
    if (str == "IDLE")            return Activity::IDLE;
    if (str == "PATROLLING")      return Activity::PATROLLING;
    if (str == "WORKING")         return Activity::WORKING;
    if (str == "RESTING")         return Activity::RESTING;
    if (str == "IN_CONVERSATION") return Activity::IN_CONVERSATION;
    if (str == "TRAVELING")       return Activity::TRAVELING;
    if (str == "SEARCHING")       return Activity::SEARCHING;
    if (str == "FLEEING")         return Activity::FLEEING;
    return Activity::IDLE;
}

// ============================================================================
// Agenda Conversions
// ============================================================================

std::string agendaToString(Agenda agenda) {
    switch (agenda) {
        case Agenda::SHORT_TERM: return "SHORT_TERM";
        case Agenda::LONG_TERM:  return "LONG_TERM";
        default:                 return "UNKNOWN";
    }
}

Agenda stringToAgenda(const std::string& str) {
    if (str == "SHORT_TERM") return Agenda::SHORT_TERM;
    if (str == "LONG_TERM")  return Agenda::LONG_TERM;
    return Agenda::SHORT_TERM;
}

// ============================================================================
// StrategyStyle Conversions
// ============================================================================

std::string strategyStyleToString(StrategyStyle style) {
    switch (style) {
        case StrategyStyle::MANIPULATIVE: return "MANIPULATIVE";
        case StrategyStyle::HONEST:       return "HONEST";
        case StrategyStyle::PERSUASIVE:   return "PERSUASIVE";
        default:                          return "UNKNOWN";
    }
}

StrategyStyle stringToStrategyStyle(const std::string& str) {
    if (str == "MANIPULATIVE") return StrategyStyle::MANIPULATIVE;
    if (str == "HONEST")       return StrategyStyle::HONEST;
    if (str == "PERSUASIVE")   return StrategyStyle::PERSUASIVE;
    return StrategyStyle::HONEST;
}

// ============================================================================
// ProblemState Conversions
// ============================================================================

std::string problemStateToString(ProblemState state) {
    switch (state) {
        case ProblemState::UNRESOLVED:   return "UNRESOLVED";
        case ProblemState::IN_DIALOGUE:  return "IN_DIALOGUE";
        case ProblemState::ACKNOWLEDGED: return "ACKNOWLEDGED";
        case ProblemState::RESOLVED:     return "RESOLVED";
        case ProblemState::PERSISTENT:   return "PERSISTENT";
        default:                         return "UNKNOWN";
    }
}

ProblemState stringToProblemState(const std::string& str) {
    if (str == "UNRESOLVED")   return ProblemState::UNRESOLVED;
    if (str == "IN_DIALOGUE")  return ProblemState::IN_DIALOGUE;
    if (str == "ACKNOWLEDGED") return ProblemState::ACKNOWLEDGED;
    if (str == "RESOLVED")     return ProblemState::RESOLVED;
    if (str == "PERSISTENT")   return ProblemState::PERSISTENT;
    return ProblemState::UNRESOLVED;
}

}  // namespace TLS
