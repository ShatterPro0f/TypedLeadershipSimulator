#include "DialogueController.h"
#include "DialogueState.h"
#include <iostream>

namespace TLS {

// Singleton instance
DialogueController& DialogueController::getInstance() {
    static DialogueController instance;
    return instance;
}

bool DialogueController::startConversation(int npcId, const std::string& issue) {
    activeConversationNpcId_ = npcId;
    isWaitingForPlayerResponse_ = true;
    conversationStartTick_ = 0;  // Simplified - no tick tracking for now
    
    auto& tracker = NpcDialogueTracker::getInstance();
    tracker.updateNpcDialogueState(npcId, DialogueState::IN_DIALOGUE, 0);
    tracker.recordConversation(npcId, "[NPC] " + issue);
    
    return true;
}

bool DialogueController::handlePlayerResponse(const std::string& playerInput, int currentTick) {
    if (activeConversationNpcId_ < 0) {
        return false;
    }
    
    auto& tracker = NpcDialogueTracker::getInstance();
    tracker.recordConversation(activeConversationNpcId_, "[PLAYER] " + playerInput);
    tracker.updateNpcDialogueState(activeConversationNpcId_, DialogueState::RESPONSE_ACKNOWLEDGED, currentTick);
    
    isWaitingForPlayerResponse_ = false;
    return true;
}

bool DialogueController::endConversation(int npcId, bool resolved) {
    if (activeConversationNpcId_ != npcId) {
        return false;
    }
    
    auto& tracker = NpcDialogueTracker::getInstance();
    DialogueState newState = resolved ? DialogueState::RESOLVED : DialogueState::PROBLEM_DETECTED;
    tracker.updateNpcDialogueState(npcId, newState, 0);
    tracker.recordConversation(npcId, "[END] Conversation completed.");
    
    activeConversationNpcId_ = -1;
    isWaitingForPlayerResponse_ = false;
    return true;
}

}  // namespace TLS
