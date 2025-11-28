#pragma once

#include <string>

namespace TLS {

// Forward declarations
class WorldState;
class NpcDialogueTracker;

/**
 * @class DialogueController
 * @brief Manages dialogue flow state machine and player interactions
 *
 * Handles:
 * - Starting conversations with NPCs
 * - Processing player responses during dialogue
 * - Ending conversations and resuming NPC activity
 * - Conversation timeouts (if player inactive)
 */
class DialogueController {
public:
    static DialogueController& getInstance();

    // Conversation lifecycle - simplified for testing
    bool startConversation(int npcId, const std::string& issue);
    bool handlePlayerResponse(const std::string& playerInput, int currentTick);
    bool endConversation(int npcId, bool resolved);

    // Conversation state
    bool isConversationActive() const { return activeConversationNpcId_ >= 0; }
    int getActiveConversationNpcId() const { return activeConversationNpcId_; }
    bool isWaitingForPlayerResponse() const { return isWaitingForPlayerResponse_; }
    int getConversationStartTick() const { return conversationStartTick_; }

    // Pause/resume conversation
    void pause() { isPaused_ = true; }
    void resume() { isPaused_ = false; }
    bool isPaused() const { return isPaused_; }

    // Utility
    static constexpr int CONVERSATION_TIMEOUT_TICKS = 3600;  // ~1 game minute

private:
    DialogueController() = default;
    DialogueController(const DialogueController&) = delete;
    DialogueController& operator=(const DialogueController&) = delete;

    int activeConversationNpcId_ = -1;
    bool isWaitingForPlayerResponse_ = false;
    int conversationStartTick_ = 0;
    bool isPaused_ = false;
};

}  // namespace TLS
