#include "ProximityDialogueSystem.h"
#include "DialogueState.h"
#include "NpcProblemSystem.h"
#include "Registries.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace TLS {

// Singleton instance
ProximityDialogueSystem& ProximityDialogueSystem::getInstance() {
    static ProximityDialogueSystem instance;
    return instance;
}

void ProximityDialogueSystem::updateProximityDetection(
    const std::vector<std::shared_ptr<class NPC>>& allNpcs,
    const std::shared_ptr<class Player>& player,
    int currentTick) {
    
    // Clear previous frame's nearby NPCs
    npcInProximity_.clear();

    if (!player) {
        return;  // No player, no proximity detection
    }

    // Check each NPC for proximity
    for (const auto& npc : allNpcs) {
        if (!npc) continue;

        float distance = calculateDistance(npc->getPosition(), player->position);

        if (distance < PROXIMITY_RANGE) {
            npcInProximity_.push_back({
                npc->getId(),
                NpcDialogueTracker::getInstance().getProblemSeverity(npc->getId()),
                0.0f,  // Will calculate influence score in queueNpcForConversation
                currentTick,
                0  // Queue position will be set during queueing
            });
        }
    }

    // Sort by distance (closer NPCs first in processing)
    std::sort(npcInProximity_.begin(), npcInProximity_.end(),
        [this, &allNpcs, &player](const ConversationQueueEntry& a, const ConversationQueueEntry& b) {
            // Get NPC objects and their distances
            auto npcA = findNpc(allNpcs, a.npcId);
            auto npcB = findNpc(allNpcs, b.npcId);
            
            if (npcA && npcB) {
                float distA = calculateDistance(npcA->getPosition(), player->position);
                float distB = calculateDistance(npcB->getPosition(), player->position);
                return distA < distB;
            }
            return false;
        });

    // Queue NPCs from proximity list
    for (const auto& entry : npcInProximity_) {
        queueNpcForConversation(entry.npcId, currentTick, allNpcs);
    }
}

void ProximityDialogueSystem::queueNpcForConversation(
    int npcId,
    int currentTick,
    const std::vector<std::shared_ptr<class NPC>>& allNpcs) {
    
    // Check if NPC already in queue
    for (const auto& entry : conversationQueue_) {
        if (entry.npcId == npcId) {
            return;  // Already queued
        }
    }

    auto& dialogueTracker = NpcDialogueTracker::getInstance();
    float severity = dialogueTracker.getProblemSeverity(npcId);
    
    // Calculate influence score (simplified - requires faction/loyalty data)
    float influenceScore = 0.5f;  // Default; can be enhanced with faction data
    
    ConversationQueueEntry entry{
        npcId,
        severity,
        influenceScore,
        currentTick,
        static_cast<int>(conversationQueue_.size())
    };

    if (conversationQueue_.size() < MAX_QUEUE_LENGTH) {
        conversationQueue_.push_back(entry);
    } else {
        // Queue full - either drop lowest priority or replace based on new NPC severity
        if (severity > conversationQueue_.back().severityScore) {
            conversationQueue_.pop_back();
            conversationQueue_.push_back(entry);
        }
    }

    // Sort queue by priority
    sortConversationQueue(allNpcs);
}

void ProximityDialogueSystem::sortConversationQueue(
    const std::vector<std::shared_ptr<class NPC>>& allNpcs) {
    
    // Calculate priority for each entry and sort
    std::sort(conversationQueue_.begin(), conversationQueue_.end(),
        [this, &allNpcs](const ConversationQueueEntry& a, const ConversationQueueEntry& b) {
            float priorityA = calculateConversationPriority(a, allNpcs);
            float priorityB = calculateConversationPriority(b, allNpcs);
            return priorityA > priorityB;  // Higher priority first
        });

    // Update queue positions
    for (size_t i = 0; i < conversationQueue_.size(); ++i) {
        conversationQueue_[i].queuePosition = static_cast<int>(i);
    }
}

float ProximityDialogueSystem::calculateConversationPriority(
    const ConversationQueueEntry& entry,
    const std::vector<std::shared_ptr<class NPC>>& allNpcs) const {
    
    // Priority formula: 0.4*severity + 0.3*influence + 0.15*(1-dist/5) + 0.15*(1-timeSinceArrival/600)
    
    float severityComponent = W_SEVERITY * entry.severityScore;
    float influenceComponent = W_INFLUENCE * entry.influenceScore;
    
    // Distance component (need player position - use cached or assume)
    float distanceComponent = W_DISTANCE * 0.5f;  // Default if no player reference
    
    // Time component
    int timeSinceArrival = lastUpdateTick_ - entry.tickArrived;
    float timeComponent = W_TIME * (1.0f - (static_cast<float>(timeSinceArrival) / MAX_TIME_IN_QUEUE));
    timeComponent = std::max(0.0f, std::min(1.0f, timeComponent));  // Clamp to [0, 1]
    
    return severityComponent + influenceComponent + distanceComponent + timeComponent;
}

ConversationQueueEntry* ProximityDialogueSystem::peekNextConversation() {
    if (conversationQueue_.empty()) {
        return nullptr;
    }
    return &conversationQueue_.front();
}

const ConversationQueueEntry* ProximityDialogueSystem::peekNextConversation() const {
    if (conversationQueue_.empty()) {
        return nullptr;
    }
    return &conversationQueue_.front();
}

ConversationQueueEntry ProximityDialogueSystem::dequeueNextConversation() {
    if (conversationQueue_.empty()) {
        return ConversationQueueEntry{-1, 0.0f, 0.0f, 0, 0};
    }

    ConversationQueueEntry entry = conversationQueue_.front();
    conversationQueue_.pop_front();

    // Update queue positions for remaining entries
    for (size_t i = 0; i < conversationQueue_.size(); ++i) {
        conversationQueue_[i].queuePosition = static_cast<int>(i);
    }

    return entry;
}

int ProximityDialogueSystem::getQueueLength() const {
    return static_cast<int>(conversationQueue_.size());
}

bool ProximityDialogueSystem::isQueueEmpty() const {
    return conversationQueue_.empty();
}

void ProximityDialogueSystem::clearConversationQueue() {
    conversationQueue_.clear();
}

void ProximityDialogueSystem::removeNpcFromQueue(int npcId) {
    conversationQueue_.erase(
        std::remove_if(conversationQueue_.begin(), conversationQueue_.end(),
            [npcId](const ConversationQueueEntry& entry) { return entry.npcId == npcId; }),
        conversationQueue_.end()
    );

    // Update queue positions
    for (size_t i = 0; i < conversationQueue_.size(); ++i) {
        conversationQueue_[i].queuePosition = static_cast<int>(i);
    }
}

float ProximityDialogueSystem::calculateDistance(
    const Vector3& pos1,
    const Vector3& pos2) const {
    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    float dz = pos1.z - pos2.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::shared_ptr<class NPC> ProximityDialogueSystem::findNpc(
    const std::vector<std::shared_ptr<class NPC>>& allNpcs,
    int npcId) const {
    
    for (const auto& npc : allNpcs) {
        if (npc && npc->getId() == npcId) {
            return npc;
        }
    }
    return nullptr;
}

}  // namespace TLS
