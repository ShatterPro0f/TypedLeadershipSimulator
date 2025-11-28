#pragma once

#include <string>
#include <vector>

namespace TLS {

// Forward declarations
class NPC;
class Player;
class WorldState;
class Faction;
class ProximityDialogueSystem;

/**
 * @class DialogueUI
 * @brief Handles all dialogue display and UI formatting
 *
 * Displays:
 * - NPC dialogue windows with context
 * - Dialogue options/actions available
 * - Conversation history
 * - NPC status (loyalty, mood, faction)
 * - Conversation queue status (other NPCs waiting)
 */
class DialogueUI {
public:
    // Main dialogue display
    static void displayDialogueWindow(
        const NPC& npc,
        const std::string& dialogue,
        const Player& player,
        const WorldState& world
    );

    // Available actions/responses
    static void displayDialogueOptions(
        const NPC& npc,
        const std::vector<std::string>& availableActions
    );

    // NPC conversation history
    static void displayConversationHistory(
        const NPC& npc,
        const std::vector<std::string>& history,
        int maxEntries = 3
    );

    // NPC context (status, faction, concern)
    static void displayNpcContext(
        const NPC& npc,
        const WorldState& world
    );

    // Queue status (other NPCs waiting)
    static void displayConversationQueue(
        const ProximityDialogueSystem& system,
        int maxDisplay = 3
    );

    // System messages
    static std::string displaySystemMessage(const std::string& message);
    static std::string displayWarning(const std::string& message);
    static std::string displaySuccess(const std::string& message);

    // Helper formatting functions
    static std::string formatNpcName(const NPC& npc, const WorldState& world);
    static std::string formatMood(float mood);
    static std::string formatLoyalty(float loyalty);
    static std::string formatFactionAlignment(float alignment);

private:
    // Formatting constants
    static constexpr const char* SEPARATOR_LINE = "════════════════════════════════════";
    static constexpr const char* DIALOGUE_MARKER = "[DIALOGUE]";
    static constexpr const char* SYSTEM_MARKER = "[SYSTEM]";
    static constexpr const char* CONTEXT_MARKER = "[CONTEXT]";
    static constexpr const char* QUEUE_MARKER = "[QUEUE]";
    static constexpr const char* WARNING_MARKER = "[⚠ WARNING]";
    static constexpr const char* SUCCESS_MARKER = "[✓ SUCCESS]";
};

}  // namespace TLS
