#include "DialogueState.h"
#include <stdexcept>
#include <iostream>

namespace TLS {

// Singleton instance
NpcDialogueTracker& NpcDialogueTracker::getInstance() {
    static NpcDialogueTracker instance;
    return instance;
}

void NpcDialogueTracker::updateNpcDialogueState(int npcId, DialogueState newState, int currentTick) {
    auto& state = dialogueStates_[npcId];
    if (state.npcId == -1) {
        state.npcId = npcId;
    }
    state.currentState = newState;
    state.stateEnteredTick = currentTick;
}

DialogueState NpcDialogueTracker::getNpcDialogueState(int npcId) const {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return it->second.currentState;
    }
    return DialogueState::IDLE;
}

NpcDialogueState* NpcDialogueTracker::getNpcDialogueInfo(int npcId) {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return &it->second;
    }
    return nullptr;
}

const NpcDialogueState* NpcDialogueTracker::getNpcDialogueInfo(int npcId) const {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return &it->second;
    }
    return nullptr;
}

void NpcDialogueTracker::setProblemSeverity(int npcId, float severity) {
    dialogueStates_[npcId].problemSeverity = severity;
}

float NpcDialogueTracker::getProblemSeverity(int npcId) const {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return it->second.problemSeverity;
    }
    return 0.0f;
}

void NpcDialogueTracker::setProblemDescription(int npcId, const std::string& description) {
    dialogueStates_[npcId].problemDescription = description;
}

std::string NpcDialogueTracker::getProblemDescription(int npcId) const {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return it->second.problemDescription;
    }
    return "";
}

void NpcDialogueTracker::recordConversation(int npcId, const std::string& dialogue) {
    auto& state = dialogueStates_[npcId];
    if (state.npcId == -1) {
        state.npcId = npcId;
    }
    state.conversationHistory.push_back(dialogue);
    state.conversationCount++;
}

const std::vector<std::string>& NpcDialogueTracker::getConversationHistory(int npcId) const {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return it->second.conversationHistory;
    }
    return emptyHistory_;
}

int NpcDialogueTracker::getConversationCount(int npcId) const {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return it->second.conversationCount;
    }
    return 0;
}

void NpcDialogueTracker::incrementEscalationLevel(int npcId) {
    dialogueStates_[npcId].escalationLevel++;
}

int NpcDialogueTracker::getEscalationLevel(int npcId) const {
    auto it = dialogueStates_.find(npcId);
    if (it != dialogueStates_.end()) {
        return it->second.escalationLevel;
    }
    return 0;
}

bool NpcDialogueTracker::canNpcReinitiate(int npcId, int currentTick, int cooldownTicks) const {
    auto it = dialogueStates_.find(npcId);
    if (it == dialogueStates_.end()) {
        return true;  // Never initiated before
    }

    if (it->second.lastConversationTick < 0) {
        return true;  // Never had conversation
    }

    int timeSinceLastDialogue = currentTick - it->second.lastConversationTick;
    return timeSinceLastDialogue >= cooldownTicks;
}

void NpcDialogueTracker::resetNpcDialogueState(int npcId) {
    dialogueStates_.erase(npcId);
}

void NpcDialogueTracker::clearAllDialogueStates() {
    dialogueStates_.clear();
}

std::string NpcDialogueTracker::getStateString(DialogueState state) const {
    switch (state) {
        case DialogueState::IDLE:
            return "IDLE";
        case DialogueState::PROBLEM_DETECTED:
            return "PROBLEM_DETECTED";
        case DialogueState::PATHFINDING_TO_PLAYER:
            return "PATHFINDING_TO_PLAYER";
        case DialogueState::PROXIMITY_REACHED:
            return "PROXIMITY_REACHED";
        case DialogueState::IN_DIALOGUE:
            return "IN_DIALOGUE";
        case DialogueState::RESPONSE_ACKNOWLEDGED:
            return "RESPONSE_ACKNOWLEDGED";
        case DialogueState::RESOLVED:
            return "RESOLVED";
        case DialogueState::COOLDOWN:
            return "COOLDOWN";
        default:
            return "UNKNOWN";
    }
}

}  // namespace TLS
