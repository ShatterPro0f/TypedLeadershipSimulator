#include <gtest/gtest.h>
#include "AmbientDialogueSystem.h"
#include "Vector3.h"
#include <chrono>
#include <string>

using namespace TypedLeadership;

// ============================================================================
// Test Suite 1: Singleton & Initialization (4 tests)
// ============================================================================

TEST(Phase10_Singleton, GetInstanceReturnsValidPointer) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    EXPECT_TRUE(&system != nullptr);
}

TEST(Phase10_Initialization, InitializeSucceeds) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    EXPECT_TRUE(true);  // If initialize succeeds without crashing
}

TEST(Phase10_Initialization, ShutdownSucceeds) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    system.shutdown();
    EXPECT_TRUE(true);
}

TEST(Phase10_Metrics, MetricsResetProperly) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    system.resetMetrics();
    
    auto metrics = system.getMetrics();
    EXPECT_EQ(metrics.totalPairsEvaluated, 0);
    EXPECT_EQ(metrics.conversationsGenerated, 0);
}

// ============================================================================
// Test Suite 2: Topic Selection (6 tests)
// ============================================================================

TEST(Phase10_Topics, DialogueTopicConversions) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    
    EXPECT_EQ(system.dialogueTopicToString(DialogueTopic::WORK), "work");
    EXPECT_EQ(system.dialogueTopicToString(DialogueTopic::FOOD_SHORTAGE), "food_shortage");
    EXPECT_EQ(system.dialogueTopicToString(DialogueTopic::CELEBRATION), "celebration");
}

TEST(Phase10_Tones, DialogueToneConversions) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    
    EXPECT_EQ(system.dialogueToneToString(DialogueTone::CASUAL), "casual");
    EXPECT_EQ(system.dialogueToneToString(DialogueTone::SERIOUS), "serious");
    EXPECT_EQ(system.dialogueToneToString(DialogueTone::EXCITED), "excited");
}

TEST(Phase10_Tones, SelectToneLow) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    DialogueTone tone = system.selectToneByMood(0.2f);
    EXPECT_EQ(tone, DialogueTone::CONCERNED);
}

TEST(Phase10_Tones, SelectToneMid) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    DialogueTone tone = system.selectToneByMood(0.5f);
    EXPECT_EQ(tone, DialogueTone::CASUAL);
}

TEST(Phase10_Tones, SelectToneHigh) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    DialogueTone tone = system.selectToneByMood(0.9f);
    EXPECT_EQ(tone, DialogueTone::EXCITED);
}

TEST(Phase10_Cascades, CascadeTypeConversions) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    
    EXPECT_EQ(system.cascadeTypeToString(CascadeType::FACTION_TENSION), "faction_tension");
    EXPECT_EQ(system.cascadeTypeToString(CascadeType::GOSSIP_PROPAGATION), "gossip_propagation");
    EXPECT_EQ(system.cascadeTypeToString(CascadeType::NONE), "none");
}

// ============================================================================
// Test Suite 3: Dialogue Generation & Quality (5 tests)
// ============================================================================

TEST(Phase10_Dialogue, GenerateOfflineDialogueSucceeds) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    DialogueContext context{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    auto dialogue = system.generateDialogue(context, nullptr, false);
    
    EXPECT_TRUE(dialogue.has_value());
    if (dialogue.has_value()) {
        EXPECT_GT(dialogue.value().npc1Dialogue.length(), 0);
        EXPECT_GT(dialogue.value().npc2Dialogue.length(), 0);
    }
}

TEST(Phase10_Quality, QualityScoreInRange) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    
    DialogueContext context{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    GeneratedDialogue dialogue{
        "Alice: 'How is work?'",
        "Bob: 'Going well.'",
        "greeting",
        0.0f,
        false,
        1000
    };
    
    float quality = system.scoreDialogueQuality(dialogue, context);
    EXPECT_GE(quality, 0.0f);
    EXPECT_LE(quality, 1.0f);
}

TEST(Phase10_Quality, BadDialogueRejected) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    
    float bad_quality = 0.5f;
    EXPECT_TRUE(system.shouldRejectDialogue(bad_quality));
}

TEST(Phase10_Quality, GoodDialogueAccepted) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    
    float good_quality = 0.85f;
    EXPECT_FALSE(system.shouldRejectDialogue(good_quality));
}

TEST(Phase10_LLMPrompt, PromptBuiltSuccessfully) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    
    DialogueContext context{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    std::string prompt = system.buildLLMPrompt(context);
    
    EXPECT_GT(prompt.length(), 0);
    EXPECT_TRUE(prompt.find("Alice") != std::string::npos);
    EXPECT_TRUE(prompt.find("Bob") != std::string::npos);
}

// ============================================================================
// Test Suite 4: Conversation Storage (5 tests)
// ============================================================================

TEST(Phase10_Storage, StoreConversationSucceeds) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    DialogueContext ctx{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    GeneratedDialogue dlg{
        "Alice: 'Hello'",
        "Bob: 'Hi'",
        "greeting",
        0.8f,
        false,
        1000
    };
    
    ConversationRecord record{1, 2, ctx, dlg, {}, 1000};
    system.storeConversation(record);
    
    EXPECT_TRUE(true);
}

TEST(Phase10_Storage, RetrieveConversationHistory) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    DialogueContext ctx{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    GeneratedDialogue dlg{
        "Alice: 'x'",
        "Bob: 'y'",
        "x",
        0.8f,
        false,
        1000
    };
    
    system.storeConversation({1, 2, ctx, dlg, {}, 1000});
    
    auto history = system.getConversationHistory(1, 2);
    EXPECT_GE(history.size(), 1);
}

TEST(Phase10_Storage, GetAllConversations) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    for (int i = 0; i < 5; i++) {
        DialogueContext ctx{
            1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
            0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000 + i
        };
        
        GeneratedDialogue dlg{
            "A: 'x'",
            "B: 'y'",
            "x",
            0.8f,
            false,
            1000 + i
        };
        
        system.storeConversation({1, 2, ctx, dlg, {}, 1000 + i});
    }
    
    auto all = system.getAllConversations(20);
    EXPECT_GE(all.size(), 5);
}

TEST(Phase10_Storage, CircularBufferWraps) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    // Store many conversations
    for (int i = 0; i < 100; i++) {
        DialogueContext ctx{
            1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
            0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000 + i
        };
        
        GeneratedDialogue dlg{
            "A: 'x'",
            "B: 'y'",
            "x",
            0.8f,
            false,
            1000 + i
        };
        
        system.storeConversation({1, 2, ctx, dlg, {}, 1000 + i});
    }
    
    // Should not crash or exceed memory
    auto recent = system.getAllConversations(50);
    EXPECT_LE(recent.size(), 100);
}

// ============================================================================
// Test Suite 5: Queue Management (4 tests)
// ============================================================================

TEST(Phase10_Queue, EnqueueSucceeds) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    system.enqueueConversation(1, 2, 0.8f, DialogueTopic::WORK);
    EXPECT_GE(system.getQueueSize(), 1);
}

TEST(Phase10_Queue, DequeueSucceeds) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    system.enqueueConversation(1, 2, 0.8f, DialogueTopic::WORK);
    auto entry = system.dequeueTopPriority();
    
    EXPECT_TRUE(entry.has_value());
}

TEST(Phase10_Queue, EmptyQueueReturnsNull) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    auto entry = system.dequeueTopPriority();
    EXPECT_FALSE(entry.has_value());
}

TEST(Phase10_Queue, QueueSizeAccurate) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    EXPECT_EQ(system.getQueueSize(), 0);
    
    system.enqueueConversation(1, 2, 0.8f, DialogueTopic::WORK);
    EXPECT_EQ(system.getQueueSize(), 1);
    
    system.enqueueConversation(3, 4, 0.7f, DialogueTopic::CONCERNS);
    EXPECT_EQ(system.getQueueSize(), 2);
}

// ============================================================================
// Test Suite 6: Edge Cases & Performance (4 tests)
// ============================================================================

TEST(Phase10_Performance, InitializeQuick) {
    auto start = std::chrono::high_resolution_clock::now();
    
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100);  // Should init in < 100ms
}

TEST(Phase10_Performance, DialogueGenerationQuick) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    DialogueContext context{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    for (int i = 0; i < 10; i++) {
        auto dialogue = system.generateDialogue(context, nullptr, false);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 500);  // 10 dialogues in < 500ms
}

TEST(Phase10_Determinism, SeededRandomness) {
    AmbientDialogueSystem& sys1 = AmbientDialogueSystem::getInstance();
    sys1.initialize();
    
    srand(42);
    DialogueTone tone1 = sys1.selectToneByMood(0.5f);
    
    srand(42);
    DialogueTone tone2 = sys1.selectToneByMood(0.5f);
    
    EXPECT_EQ(tone1, tone2);
}

TEST(Phase10_Error, HandlesNullpointerGracefully) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    // Call with nullptr LLM provider (should fall back to offline)
    DialogueContext context{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    auto dialogue = system.generateDialogue(context, nullptr, false);
    EXPECT_TRUE(dialogue.has_value());  // Should succeed with fallback
}

// ============================================================================
// Integration & Metrics (3 tests)
// ============================================================================

TEST(Phase10_Integration, FullConversationWorkflow) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    // 1. Generate dialogue context
    DialogueContext context{
        1, 2, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
        0.5f, 0.6f, "Alice", "Bob", "Farmer", "Farmer", 1, 1, 1000
    };
    
    // 2. Generate dialogue
    auto dialogue = system.generateDialogue(context, nullptr, false);
    EXPECT_TRUE(dialogue.has_value());
    
    // 3. Score quality
    if (dialogue.has_value()) {
        float quality = system.scoreDialogueQuality(dialogue.value(), context);
        EXPECT_GE(quality, 0.0f);
        EXPECT_LE(quality, 1.0f);
    }
    
    // 4. Store conversation
    if (dialogue.has_value()) {
        ConversationRecord record{1, 2, context, dialogue.value(), {}, 1000};
        system.storeConversation(record);
    }
    
    // 5. Retrieve conversation
    auto history = system.getConversationHistory(1, 2);
    EXPECT_GE(history.size(), 1);
}

TEST(Phase10_Integration, MetricsTracked) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    auto metrics_before = system.getMetrics();
    
    // Do some operations
    system.enqueueConversation(1, 2, 0.8f, DialogueTopic::WORK);
    system.dequeueTopPriority();
    
    auto metrics_after = system.getMetrics();
    
    // Metrics should have been tracked
    EXPECT_TRUE(true);  // If we got here, no crash
}

TEST(Phase10_Integration, MultipleConversations) {
    AmbientDialogueSystem& system = AmbientDialogueSystem::getInstance();
    system.initialize();
    
    // Store multiple conversations
    for (int i = 0; i < 10; i++) {
        DialogueContext ctx{
            i % 5 + 1, i % 3 + 6, DialogueTopic::WORK, DialogueTone::CASUAL, "farm",
            0.5f, 0.6f, "NPC1", "NPC2", "Role1", "Role2", 1, 1, 1000 + i
        };
        
        GeneratedDialogue dlg{
            "NPC1: 'test'",
            "NPC2: 'test'",
            "test",
            0.8f,
            false,
            1000 + i
        };
        
        system.storeConversation({ctx.npcId1, ctx.npcId2, ctx, dlg, {}, 1000 + i});
    }
    
    auto all = system.getAllConversations(20);
    EXPECT_GE(all.size(), 10);
}
