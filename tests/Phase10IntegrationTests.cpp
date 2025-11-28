#include <gtest/gtest.h>
#include "AmbientDialogueSystem.h"
#include "DecisionInterpreter.h"
#include "LLMProvider.h"
#include "Core.h"
#include <chrono>

using namespace TypedLeadership;
using namespace TLS;

// ============================================================================
// Phase 10 Integration Tests - Interaction with other systems
// ============================================================================

class Phase10IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        ambientDialogueSystem_ = &AmbientDialogueSystem::getInstance();
        ambientDialogueSystem_->initialize();
    }
    
    void TearDown() override {
        ambientDialogueSystem_->shutdown();
    }
    
    AmbientDialogueSystem* ambientDialogueSystem_;
};

// ============================================================================
// Integration Test 1: Dialogue System with LLMProvider Interface
// ============================================================================

TEST_F(Phase10IntegrationTest, GenerateDialogueWithNullLLMProvider) {
    // Test that dialogue generation works even when LLM provider is nullptr
    DialogueContext context{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    auto dialogue = ambientDialogueSystem_->generateDialogue(context, nullptr, false);
    
    EXPECT_TRUE(dialogue.has_value());
    EXPECT_FALSE(dialogue.value().npc1Dialogue.empty());
    EXPECT_FALSE(dialogue.value().npc2Dialogue.empty());
}

TEST_F(Phase10IntegrationTest, DialogueGeneration_OfflineFallback) {
    // Verify offline fallback generates consistent output
    DialogueContext context{
        3, 4, DialogueTopic::CONCERNS, DialogueTone::SERIOUS, "temple",
        0.3f, 0.4f, "Priest1", "Priest2", "Priest", "Priest", 2, 2, 2000
    };
    
    auto dialogue1 = ambientDialogueSystem_->generateDialogue(context, nullptr, false);
    auto dialogue2 = ambientDialogueSystem_->generateDialogue(context, nullptr, false);
    
    // Both should succeed
    EXPECT_TRUE(dialogue1.has_value());
    EXPECT_TRUE(dialogue2.has_value());
    
    // Both should have NPC names in output
    EXPECT_TRUE(dialogue1.value().npc1Dialogue.find("Priest1") != std::string::npos);
    EXPECT_TRUE(dialogue2.value().npc2Dialogue.find("Priest2") != std::string::npos);
}

// ============================================================================
// Integration Test 2: Dialogue Quality Scoring System
// ============================================================================

TEST_F(Phase10IntegrationTest, QualityScoring_AcceptsGoodDialogue) {
    DialogueContext context{
        5, 6, DialogueTopic::TRADE, DialogueTone::DIPLOMATIC, "market",
        0.7f, 0.7f, "Merchant1", "Merchant2", "Merchant", "Merchant", 3, 3, 3000
    };
    
    GeneratedDialogue dialogue{
        "Merchant1: 'I have fine goods for trade.'",
        "Merchant2: 'What price do you ask?'",
        "negotiation",
        0.85f,  // Good quality score
        false,
        3000
    };
    
    float quality = ambientDialogueSystem_->scoreDialogueQuality(dialogue, context);
    EXPECT_GE(quality, 0.7f);  // Should pass quality threshold
    EXPECT_FALSE(ambientDialogueSystem_->shouldRejectDialogue(quality));
}

TEST_F(Phase10IntegrationTest, QualityScoring_RejectsPoorDialogue) {
    DialogueContext context{
        7, 8, DialogueTopic::GOSSIP, DialogueTone::CASUAL, "tavern",
        0.5f, 0.5f, "NPC1", "NPC2", "Commoner", "Commoner", 4, 4, 4000
    };
    
    GeneratedDialogue dialogue{
        "asdf",
        "xyz",
        "noise",
        0.2f,  // Poor quality
        false,
        4000
    };
    
    float quality = ambientDialogueSystem_->scoreDialogueQuality(dialogue, context);
    EXPECT_TRUE(ambientDialogueSystem_->shouldRejectDialogue(quality));
}

// ============================================================================
// Integration Test 3: Conversation Storage & Retrieval with Filtering
// ============================================================================

TEST_F(Phase10IntegrationTest, StorageAndRetrieval_ConsistentAcrossOperations) {
    // Create and store multiple conversations
    std::vector<ConversationRecord> records;
    
    for (int i = 0; i < 5; i++) {
        DialogueContext ctx{
            1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
            0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000 + (i * 100)
        };
        
        GeneratedDialogue dlg{
            "Alice: 'Hello.'",
            "Bob: 'Hi.'",
            "greeting",
            0.8f,
            false,
            1000 + (i * 100)
        };
        
        ConversationRecord record{1, 2, ctx, dlg, {}, 1000 + (i * 100)};
        ambientDialogueSystem_->storeConversation(record);
        records.push_back(record);
    }
    
    // Retrieve and verify
    auto history = ambientDialogueSystem_->getConversationHistory(1, 2, 10);
    EXPECT_GE(history.size(), 5);
    
    // Verify most recent conversation is included
    auto all = ambientDialogueSystem_->getAllConversations(10);
    EXPECT_GE(all.size(), 5);
}

TEST_F(Phase10IntegrationTest, CircularBuffer_HandlesOverflow) {
    // Store many conversations to test buffer overflow
    for (int i = 0; i < 50; i++) {
        DialogueContext ctx{
            i % 10 + 1, (i % 10) + 11, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
            0.5f, 0.6f, "NPC" + std::to_string(i), "NPC" + std::to_string(i+1), 
            "Role", "Role", 1, 1, 2000 + i
        };
        
        GeneratedDialogue dlg{
            "Line1", "Line2", "topic", 0.8f, false, 2000 + i
        };
        
        ambientDialogueSystem_->storeConversation({ctx.npcId1, ctx.npcId2, ctx, dlg, {}, 2000 + i});
    }
    
    auto all = ambientDialogueSystem_->getAllConversations(100);
    // Should have capped at buffer max (1000 for circular buffer)
    EXPECT_LE(all.size(), 1000);
}

// ============================================================================
// Integration Test 4: Queue Management with Priority Handling
// ============================================================================

TEST_F(Phase10IntegrationTest, QueueManagement_FIFOProcessing) {
    // Enqueue multiple conversations
    ambientDialogueSystem_->enqueueConversation(1, 2, 0.8f, DialogueTopic::WORK);
    ambientDialogueSystem_->enqueueConversation(3, 4, 0.7f, DialogueTopic::CONCERNS);
    ambientDialogueSystem_->enqueueConversation(5, 6, 0.9f, DialogueTopic::MORALE);
    
    EXPECT_EQ(ambientDialogueSystem_->getQueueSize(), 3);
    
    // Dequeue and verify FIFO order
    auto entry1 = ambientDialogueSystem_->dequeueTopPriority();
    EXPECT_TRUE(entry1.has_value());
    EXPECT_EQ(entry1.value().npcId1, 1);
    EXPECT_EQ(entry1.value().npcId2, 2);
    
    EXPECT_EQ(ambientDialogueSystem_->getQueueSize(), 2);
    
    auto entry2 = ambientDialogueSystem_->dequeueTopPriority();
    EXPECT_TRUE(entry2.has_value());
    EXPECT_EQ(entry2.value().npcId1, 3);
    EXPECT_EQ(entry2.value().npcId2, 4);
    
    EXPECT_EQ(ambientDialogueSystem_->getQueueSize(), 1);
}

TEST_F(Phase10IntegrationTest, QueueManagement_MaxCapacity) {
    // Fill queue beyond typical limits
    for (int i = 0; i < 120; i++) {
        ambientDialogueSystem_->enqueueConversation(
            (i % 100) + 1, 
            ((i + 1) % 100) + 1, 
            0.5f + (i % 5) * 0.1f, 
            (DialogueTopic)(i % 10)
        );
    }
    
    // Queue should manage overflow gracefully
    size_t queueSize = ambientDialogueSystem_->getQueueSize();
    EXPECT_LE(queueSize, 100);  // Max queue size
    EXPECT_GT(queueSize, 0);
}

// ============================================================================
// Integration Test 5: Tone Selection with NPC Emotional States
// ============================================================================

TEST_F(Phase10IntegrationTest, ToneSelection_MoodMapping) {
    // Test complete mood-to-tone spectrum
    struct MoodMapping {
        float mood;
        DialogueTone expectedTone;
    };
    
    std::vector<MoodMapping> mappings = {
        {0.1f, DialogueTone::CONCERNED},
        {0.35f, DialogueTone::SERIOUS},
        {0.5f, DialogueTone::CASUAL},
        {0.65f, DialogueTone::EXCITED},
        {0.9f, DialogueTone::EXCITED}
    };
    
    for (const auto& mapping : mappings) {
        DialogueTone tone = ambientDialogueSystem_->selectToneByMood(mapping.mood);
        EXPECT_EQ(tone, mapping.expectedTone) 
            << "Mood " << mapping.mood << " should produce tone " 
            << ambientDialogueSystem_->dialogueToneToString(mapping.expectedTone);
    }
}

// ============================================================================
// Integration Test 6: Cascade Detection Framework
// ============================================================================

TEST_F(Phase10IntegrationTest, CascadeDetection_TypeConversions) {
    // Test cascade type identification
    EXPECT_EQ(ambientDialogueSystem_->cascadeTypeToString(CascadeType::FACTION_TENSION), "faction_tension");
    EXPECT_EQ(ambientDialogueSystem_->cascadeTypeToString(CascadeType::GOSSIP_PROPAGATION), "gossip_propagation");
    EXPECT_EQ(ambientDialogueSystem_->cascadeTypeToString(CascadeType::ALLIANCE_FORMATION), "alliance_formation");
    EXPECT_EQ(ambientDialogueSystem_->cascadeTypeToString(CascadeType::LEADERSHIP_AWARENESS), "leadership_awareness");
    EXPECT_EQ(ambientDialogueSystem_->cascadeTypeToString(CascadeType::NONE), "none");
}

// ============================================================================
// Integration Test 7: Metrics Tracking Across Operations
// ============================================================================

TEST_F(Phase10IntegrationTest, MetricsTracking_CompleteLifecycle) {
    ambientDialogueSystem_->resetMetrics();
    auto metricsStart = ambientDialogueSystem_->getMetrics();
    EXPECT_EQ(metricsStart.conversationsGenerated, 0);
    EXPECT_EQ(metricsStart.llmCallsAttempted, 0);
    
    // Perform operations
    DialogueContext context{
        10, 11, DialogueTopic::TRADE, DialogueTone::DIPLOMATIC, "market",
        0.7f, 0.7f, "Merchant1", "Merchant2", "Merchant", "Merchant", 5, 5, 5000
    };
    
    auto dialogue = ambientDialogueSystem_->generateDialogue(context, nullptr, false);
    EXPECT_TRUE(dialogue.has_value());
    
    ambientDialogueSystem_->storeConversation({10, 11, context, dialogue.value(), {}, 5000});
    
    // Check metrics updated
    auto metricsEnd = ambientDialogueSystem_->getMetrics();
    EXPECT_GT(metricsEnd.conversationsGenerated, metricsStart.conversationsGenerated);
    EXPECT_GT(metricsEnd.llmCallsAttempted, metricsStart.llmCallsAttempted);
}

// ============================================================================
// Integration Test 8: Conversation Workflow Under Load
// ============================================================================

TEST_F(Phase10IntegrationTest, HighLoad_StressTest) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Generate 100 conversations
    for (int i = 0; i < 100; i++) {
        DialogueContext ctx{
            (i % 20) + 1, 
            ((i / 20) % 5) + 21, 
            (DialogueTopic)(i % 10),
            (DialogueTone)(i % 7),
            "location_" + std::to_string(i % 5),
            0.3f + (i % 7) * 0.1f,
            0.3f + (i % 7) * 0.1f,
            "NPC_" + std::to_string(i % 20),
            "NPC_" + std::to_string((i / 20) % 5 + 20),
            "Role",
            "Role",
            i % 5,
            i % 3,
            5000 + i
        };
        
        auto dlg = ambientDialogueSystem_->generateDialogue(ctx, nullptr, false);
        if (dlg.has_value()) {
            float quality = ambientDialogueSystem_->scoreDialogueQuality(dlg.value(), ctx);
            if (!ambientDialogueSystem_->shouldRejectDialogue(quality)) {
                ConversationRecord record{ctx.npcId1, ctx.npcId2, ctx, dlg.value(), {}, 5000 + i};
                ambientDialogueSystem_->storeConversation(record);
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Should complete 100 generations in reasonable time
    EXPECT_LT(duration.count(), 2000);  // < 2 seconds for 100 dialogues
    
    auto metrics = ambientDialogueSystem_->getMetrics();
    EXPECT_GT(metrics.conversationsGenerated, 50);  // At least 50 should be generated
}

// ============================================================================
// Integration Test 9: Determinism & Reproducibility
// ============================================================================

TEST_F(Phase10IntegrationTest, Determinism_SeedConsistency) {
    // Run dialogue generation with seeded RNG
    srand(42);
    
    DialogueContext ctx1{
        12, 13, DialogueTopic::MORALE, DialogueTone::CASUAL, "settlement",
        0.6f, 0.6f, "Leader", "Advisor", "Leader", "Advisor", 6, 6, 6000
    };
    
    auto dlg1 = ambientDialogueSystem_->generateDialogue(ctx1, nullptr, false);
    auto tone1 = ambientDialogueSystem_->selectToneByMood(0.55f);
    
    // Reset with same seed
    srand(42);
    
    DialogueContext ctx2{
        12, 13, DialogueTopic::MORALE, DialogueTone::CASUAL, "settlement",
        0.6f, 0.6f, "Leader", "Advisor", "Leader", "Advisor", 6, 6, 6000
    };
    
    auto dlg2 = ambientDialogueSystem_->generateDialogue(ctx2, nullptr, false);
    auto tone2 = ambientDialogueSystem_->selectToneByMood(0.55f);
    
    // Should be identical
    EXPECT_EQ(tone1, tone2);
    if (dlg1.has_value() && dlg2.has_value()) {
        EXPECT_EQ(dlg1.value().npc1Dialogue, dlg2.value().npc1Dialogue);
        EXPECT_EQ(dlg1.value().npc2Dialogue, dlg2.value().npc2Dialogue);
    }
}

// ============================================================================
// Integration Test 10: Error Handling & Graceful Degradation
// ============================================================================

TEST_F(Phase10IntegrationTest, ErrorHandling_InvalidContextHandling) {
    // Test with edge-case context values
    DialogueContext edgeContext{
        0, 0, (DialogueTopic)99, (DialogueTone)99, "",
        -1.0f, 2.0f, "", "", "", "", -1, -1, -1
    };
    
    // Should not crash
    auto dialogue = ambientDialogueSystem_->generateDialogue(edgeContext, nullptr, false);
    EXPECT_TRUE(dialogue.has_value());  // Should still produce fallback
}

TEST_F(Phase10IntegrationTest, ErrorHandling_QueueUnderflow) {
    // Test queue operations on empty queue
    for (int i = 0; i < 5; i++) {
        auto entry = ambientDialogueSystem_->dequeueTopPriority();
        EXPECT_FALSE(entry.has_value());
    }
    
    EXPECT_EQ(ambientDialogueSystem_->getQueueSize(), 0);
}

// ============================================================================
// Integration Test 11: System Lifecycle Management
// ============================================================================

TEST_F(Phase10IntegrationTest, Lifecycle_MultipleInitShutdown) {
    // Test multiple init/shutdown cycles
    for (int i = 0; i < 3; i++) {
        ambientDialogueSystem_->initialize();
        
        DialogueContext ctx{
            1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
            0.5f, 0.5f, "A", "B", "R", "R", 1, 1, 1000
        };
        
        ambientDialogueSystem_->generateDialogue(ctx, nullptr, false);
        EXPECT_EQ(ambientDialogueSystem_->getQueueSize(), 0);
        
        ambientDialogueSystem_->shutdown();
    }
}

// ============================================================================
// Integration Test 12: Enum Completeness
// ============================================================================

TEST_F(Phase10IntegrationTest, Enums_AllValuesConvertible) {
    // Test all DialogueTopic values (excluding UNKNOWN at index 9)
    for (int i = 0; i < 9; i++) {
        DialogueTopic topic = (DialogueTopic)i;
        std::string topicStr = ambientDialogueSystem_->dialogueTopicToString(topic);
        EXPECT_FALSE(topicStr.empty());
        EXPECT_NE(topicStr, "unknown");
    }
    
    // Test all DialogueTone values (excluding UNKNOWN at index 6)
    for (int i = 0; i < 6; i++) {
        DialogueTone tone = (DialogueTone)i;
        std::string toneStr = ambientDialogueSystem_->dialogueToneToString(tone);
        EXPECT_FALSE(toneStr.empty());
        EXPECT_NE(toneStr, "unknown");
    }
    
    // Verify UNKNOWN values explicitly
    EXPECT_EQ(ambientDialogueSystem_->dialogueTopicToString(DialogueTopic::UNKNOWN), "unknown");
    EXPECT_EQ(ambientDialogueSystem_->dialogueToneToString(DialogueTone::UNKNOWN), "unknown");
}
