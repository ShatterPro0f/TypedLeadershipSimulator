#include "AmbientDialogueSystem.h"
#include "Core.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <random>
#include <iostream>

namespace TypedLeadership {

// Static member initialization
AmbientDialogueSystem* AmbientDialogueSystem::instance_ = nullptr;

// Singleton implementation
AmbientDialogueSystem& AmbientDialogueSystem::getInstance() {
    if (!instance_) {
        instance_ = new AmbientDialogueSystem();
    }
    return *instance_;
}

// Constructor
AmbientDialogueSystem::AmbientDialogueSystem()
    : bufferHead_(0),
      metrics_{0, 0, 0, 0, 0, 0.0f, 0.0f} {
    conversationBuffer_.reserve(BUFFER_SIZE);
}

// Destructor
AmbientDialogueSystem::~AmbientDialogueSystem() {
}

// Initialize system
void AmbientDialogueSystem::initialize() {
    conversationBuffer_.clear();
    bufferHead_ = 0;
    pairConversationMap_.clear();
    npcBondStrength_.clear();
    factionTensionMap_.clear();
    
    // Clear the queue by creating a new empty one
    while (!conversationQueue_.empty()) {
        conversationQueue_.pop();
    }
    
    resetMetrics();
}

// Shutdown system
void AmbientDialogueSystem::shutdown() {
    conversationBuffer_.clear();
    pairConversationMap_.clear();
    npcBondStrength_.clear();
    factionTensionMap_.clear();
}

// Reset metrics
void AmbientDialogueSystem::resetMetrics() {
    metrics_.totalPairsEvaluated = 0;
    metrics_.validPairsFound = 0;
    metrics_.conversationsGenerated = 0;
    metrics_.llmCallsAttempted = 0;
    metrics_.llmCallsSucceeded = 0;
    metrics_.avgQualityScore = 0.0f;
    metrics_.totalExecutionTimeMs = 0.0f;
}

// Get metrics
AmbientDialogueSystem::PerformanceMetrics AmbientDialogueSystem::getMetrics() const {
    return metrics_;
}

// Enum conversion functions
std::string AmbientDialogueSystem::dialogueTopicToString(DialogueTopic topic) const {
    switch (topic) {
        case DialogueTopic::WORK: return "work";
        case DialogueTopic::CONCERNS: return "concerns";
        case DialogueTopic::TRADE: return "trade";
        case DialogueTopic::GOSSIP: return "gossip";
        case DialogueTopic::MORALE: return "morale";
        case DialogueTopic::FOOD_SHORTAGE: return "food_shortage";
        case DialogueTopic::IMMIGRATION: return "immigration";
        case DialogueTopic::FACTION_CONFLICT: return "faction_conflict";
        case DialogueTopic::CELEBRATION: return "celebration";
        default: return "unknown";
    }
}

std::string AmbientDialogueSystem::dialogueToneToString(DialogueTone tone) const {
    switch (tone) {
        case DialogueTone::CASUAL: return "casual";
        case DialogueTone::SERIOUS: return "serious";
        case DialogueTone::CONCERNED: return "concerned";
        case DialogueTone::EXCITED: return "excited";
        case DialogueTone::HOSTILE: return "hostile";
        case DialogueTone::DIPLOMATIC: return "diplomatic";
        default: return "unknown";
    }
}

std::string AmbientDialogueSystem::cascadeTypeToString(CascadeType cascadeType) const {
    switch (cascadeType) {
        case CascadeType::FACTION_TENSION: return "faction_tension";
        case CascadeType::GOSSIP_PROPAGATION: return "gossip_propagation";
        case CascadeType::ALLIANCE_FORMATION: return "alliance_formation";
        case CascadeType::LEADERSHIP_AWARENESS: return "leadership_awareness";
        case CascadeType::NONE: return "none";
        default: return "unknown";
    }
}

// Select tone based on mood
DialogueTone AmbientDialogueSystem::selectToneByMood(float mood) {
    if (mood < 0.3f) return DialogueTone::CONCERNED;
    if (mood < 0.4f) return DialogueTone::SERIOUS;
    if (mood < 0.6f) return DialogueTone::CASUAL;
    if (mood < 0.7f) return DialogueTone::EXCITED;
    return DialogueTone::EXCITED;
}

// Build LLM prompt from dialogue context
std::string AmbientDialogueSystem::buildLLMPrompt(const DialogueContext& context) {
    std::ostringstream oss;
    oss << "Generate a brief dialogue between two NPCs:\n";
    oss << "NPC1: " << context.npc1Name << " (" << context.npc1Role << ")\n";
    oss << "NPC2: " << context.npc2Name << " (" << context.npc2Role << ")\n";
    oss << "Topic: " << dialogueTopicToString(context.topic) << "\n";
    oss << "Tone: " << dialogueToneToString(context.tone) << "\n";
    oss << "Location: " << context.location << "\n";
    oss << "Keep it brief (2-3 exchanges max). Make it feel natural and organic.";
    return oss.str();
}

// Generate dialogue (with fallback)
std::optional<GeneratedDialogue> AmbientDialogueSystem::generateDialogue(
    const DialogueContext& context,
    LLMProvider* /* llmProvider */,
    bool /* useOnlineLLM */) {
    
    metrics_.llmCallsAttempted++;
    
    // Always use offline template (no online LLM integration yet)
    std::string npc1Dialogue = context.npc1Name + ": 'Hello there.'";
    std::string npc2Dialogue = context.npc2Name + ": 'Hello to you too!'";
    std::string impliedEmotion = "greeting";
    float qualityScore = 0.75f;
    
    GeneratedDialogue dialogue{
        npc1Dialogue,
        npc2Dialogue,
        impliedEmotion,
        qualityScore,
        false,
        0
    };
    
    metrics_.conversationsGenerated++;
    return dialogue;
}

// Score dialogue quality
float AmbientDialogueSystem::scoreDialogueQuality(
    const GeneratedDialogue& dialogue,
    const DialogueContext& context) {
    
    float score = 0.3f;  // Start with low score, build up
    
    // Check dialogue lengths
    if (dialogue.npc1Dialogue.length() > 10 && dialogue.npc2Dialogue.length() > 10) {
        score += 0.2f;
    }
    
    // Check for NPC names in dialogue
    if (dialogue.npc1Dialogue.find(context.npc1Name) != std::string::npos) {
        score += 0.15f;
    }
    if (dialogue.npc2Dialogue.find(context.npc2Name) != std::string::npos) {
        score += 0.15f;
    }
    
    return std::min(1.0f, score);
}

// Check if dialogue should be rejected
bool AmbientDialogueSystem::shouldRejectDialogue(float qualityScore) {
    return qualityScore < 0.7f;
}

// Store conversation
void AmbientDialogueSystem::storeConversation(const ConversationRecord& record) {
    if (conversationBuffer_.size() >= BUFFER_SIZE) {
        conversationBuffer_.erase(conversationBuffer_.begin());
    }
    
    conversationBuffer_.push_back(record);
}

// Get conversation history for a pair
std::vector<ConversationRecord> AmbientDialogueSystem::getConversationHistory(
    int npcId1, int npcId2, int maxRecords) {
    
    std::vector<ConversationRecord> result;
    
    // Find conversations with these NPCs in the buffer
    for (const auto& record : conversationBuffer_) {
        if ((record.npcId1 == npcId1 && record.npcId2 == npcId2) ||
            (record.npcId1 == npcId2 && record.npcId2 == npcId1)) {
            result.push_back(record);
        }
    }
    
    // Keep only the most recent
    if (result.size() > (size_t)maxRecords) {
        result.erase(result.begin(), result.end() - maxRecords);
    }
    
    return result;
}

// Get all conversations
std::vector<ConversationRecord> AmbientDialogueSystem::getAllConversations(int maxRecords) {
    std::vector<ConversationRecord> result;
    
    size_t start = conversationBuffer_.size() > (size_t)maxRecords ?
        conversationBuffer_.size() - (size_t)maxRecords : 0;
    
    result.insert(result.end(),
        conversationBuffer_.begin() + start,
        conversationBuffer_.end());
    
    return result;
}

// Enqueue conversation for processing
void AmbientDialogueSystem::enqueueConversation(
    int npcId1, int npcId2, float priority, DialogueTopic topic) {
    
    // Cap queue at 100 entries
    if (conversationQueue_.size() >= 100) {
        return;  // Queue at capacity, don't add
    }
    
    ConversationQueueEntry entry{npcId1, npcId2, priority, topic};
    conversationQueue_.push(entry);
}

// Dequeue top priority conversation
std::optional<ConversationQueueEntry> AmbientDialogueSystem::dequeueTopPriority() {
    if (conversationQueue_.empty()) {
        return std::nullopt;
    }
    
    ConversationQueueEntry entry = conversationQueue_.front();
    conversationQueue_.pop();
    
    return entry;
}

// Get queue size
size_t AmbientDialogueSystem::getQueueSize() const {
    return conversationQueue_.size();
}

}  // namespace TypedLeadership
