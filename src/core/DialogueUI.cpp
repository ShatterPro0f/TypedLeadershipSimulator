#include "DialogueUI.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace TLS {

// Formatting functions
std::string DialogueUI::formatMood(float mood) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << mood;
    return ss.str();
}

std::string DialogueUI::formatLoyalty(float loyalty) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << loyalty;
    return ss.str();
}

std::string DialogueUI::formatFactionAlignment(float alignment) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << alignment;
    return ss.str();
}

std::string DialogueUI::formatNpcName(const NPC& npc, const WorldState& world) {
    (void)npc; (void)world;
    return "NPC";
}

// System message display
std::string DialogueUI::displaySystemMessage(const std::string& message) {
    return std::string(SYSTEM_MARKER) + " " + message;
}

std::string DialogueUI::displayWarning(const std::string& message) {
    return std::string(WARNING_MARKER) + " " + message;
}

std::string DialogueUI::displaySuccess(const std::string& message) {
    return std::string(SUCCESS_MARKER) + " " + message;
}

// Dialogue display (stub)
void DialogueUI::displayDialogueWindow(const NPC& npc, const std::string& dialogue,
                                       const Player& player, const WorldState& world) {
    (void)npc; (void)dialogue; (void)player; (void)world;
}

void DialogueUI::displayDialogueOptions(const NPC& npc, const std::vector<std::string>& availableActions) {
    (void)npc; (void)availableActions;
}

void DialogueUI::displayConversationHistory(const NPC& npc, const std::vector<std::string>& history, int maxEntries) {
    (void)npc; (void)history; (void)maxEntries;
}

void DialogueUI::displayNpcContext(const NPC& npc, const WorldState& world) {
    (void)npc; (void)world;
}

void DialogueUI::displayConversationQueue(const ProximityDialogueSystem& system, int maxDisplay) {
    (void)system; (void)maxDisplay;
}

}  // namespace TLS
