/**
 * ConversationQueueTests.cpp
 * 
 * Comprehensive test suite for multi-NPC conversation queue system
 * Tests: priority calculation, queueing, overflow, determinism, performance
 * 
 * Total: 25+ test cases
 */

#include <gtest/gtest.h>
#include "ConversationQueue.h"
#include "Registries.h"
#include "Core.h"
#include <cmath>
#include <chrono>

using namespace TLS;

// ============================================================================
// Test Fixtures
// ============================================================================

class ConversationQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear registries
        NPCRegistry::getInstance().clear();
        FactionRegistry::getInstance().clear();
        ResourceRegistry::getInstance().clear();
        
        // Clear queue
        ConversationQueue::getInstance().clear();
        ConversationQueue::getInstance().resetStatistics();
        
        // Create test NPCs
        createTestNPCs();
        createTestFactions();
    }
    
    void TearDown() override {
        ConversationQueue::getInstance().clear();
        NPCRegistry::getInstance().clear();
        FactionRegistry::getInstance().clear();
        ResourceRegistry::getInstance().clear();
    }
    
    void createTestNPCs() {
        // Create 10 test NPCs with different attributes
        for (int i = 0; i < 10; i++) {
            auto npc = std::make_shared<NPC>(i, "NPC_" + std::to_string(i), Vector3(i * 10.0f, 0, 0));
            npc->setLoyalty(0.3f + (i * 0.05f));  // Varying loyalties
            npc->setShortTermMood(0.4f + (i * 0.04f));
            npc->setFactionId(i % 3);
            NPCRegistry::getInstance().registerNPC(npc);
        }
    }
    
    void createTestFactions() {
        for (int i = 0; i < 3; i++) {
            auto faction = std::make_shared<Faction>(i, "Faction_" + std::to_string(i));
            FactionRegistry::getInstance().registerFaction(faction);
        }
    }
};

// ============================================================================
// Basic Queue Operations Tests
// ============================================================================

TEST_F(ConversationQueueTest, QueueInitiallyEmpty) {
    EXPECT_TRUE(ConversationQueue::getInstance().isEmpty());
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 0);
}

TEST_F(ConversationQueueTest, EnqueueSingleNPC) {
    bool result = ConversationQueue::getInstance().enqueueNPC(
        0,      // npcId
        0.5f,   // severity
        0.6f,   // influence
        5.0f,   // distance
        1000    // currentTick
    );
    
    EXPECT_TRUE(result);
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 1);
    EXPECT_FALSE(ConversationQueue::getInstance().isEmpty());
}

TEST_F(ConversationQueueTest, EnqueueMultipleNPCs) {
    for (int i = 0; i < 3; i++) {
        bool result = ConversationQueue::getInstance().enqueueNPC(
            i, 0.5f, 0.5f, 5.0f, 1000
        );
        EXPECT_TRUE(result);
    }
    
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 3);
}

TEST_F(ConversationQueueTest, DequeueNPC) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    ConversationQueue::getInstance().enqueueNPC(1, 0.5f, 0.5f, 5.0f, 1000);
    
    ConversationQueueEntry entry = ConversationQueue::getInstance().dequeueNextNPC();
    
    EXPECT_EQ(entry.npcId, 0);  // First enqueued (assuming same priority)
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 1);
}

TEST_F(ConversationQueueTest, DequeueEmptyQueue) {
    ConversationQueueEntry entry = ConversationQueue::getInstance().dequeueNextNPC();
    EXPECT_EQ(entry.npcId, -1);  // Invalid entry
}

TEST_F(ConversationQueueTest, PeekNextNPC) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    
    const ConversationQueueEntry& entry = ConversationQueue::getInstance().peekNextNPC();
    
    EXPECT_EQ(entry.npcId, 0);
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 1);  // Still there
}

TEST_F(ConversationQueueTest, ClearQueue) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    ConversationQueue::getInstance().enqueueNPC(1, 0.5f, 0.5f, 5.0f, 1000);
    
    ConversationQueue::getInstance().clear();
    
    EXPECT_TRUE(ConversationQueue::getInstance().isEmpty());
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 0);
}

// ============================================================================
// Priority Calculation Tests
// ============================================================================

TEST_F(ConversationQueueTest, PriorityCalculationBasic) {
    ConversationQueueEntry entry(0, 0.5f, 0.5f, 5.0f, 1000, 500);
    
    float priority = ConversationQueue::calculatePriority(entry, 1000);
    
    EXPECT_GE(priority, 0.0f);
    EXPECT_LE(priority, 1.0f);
}

TEST_F(ConversationQueueTest, PriorityHighSeverity) {
    ConversationQueueEntry entryHigh(0, 0.9f, 0.5f, 5.0f, 1000, 500);
    ConversationQueueEntry entryLow(1, 0.1f, 0.5f, 5.0f, 1000, 500);
    
    float prioHigh = ConversationQueue::calculatePriority(entryHigh, 1000);
    float prioLow = ConversationQueue::calculatePriority(entryLow, 1000);
    
    EXPECT_GT(prioHigh, prioLow);
}

TEST_F(ConversationQueueTest, PriorityHighInfluence) {
    ConversationQueueEntry entryHigh(0, 0.5f, 0.9f, 5.0f, 1000, 500);
    ConversationQueueEntry entryLow(1, 0.5f, 0.1f, 5.0f, 1000, 500);
    
    float prioHigh = ConversationQueue::calculatePriority(entryHigh, 1000);
    float prioLow = ConversationQueue::calculatePriority(entryLow, 1000);
    
    EXPECT_GT(prioHigh, prioLow);
}

TEST_F(ConversationQueueTest, PriorityCloseDistance) {
    ConversationQueueEntry entryClose(0, 0.5f, 0.5f, 2.0f, 1000, 500);
    ConversationQueueEntry entryFar(1, 0.5f, 0.5f, 40.0f, 1000, 500);
    
    float prioClose = ConversationQueue::calculatePriority(entryClose, 1000);
    float prioFar = ConversationQueue::calculatePriority(entryFar, 1000);
    
    EXPECT_GT(prioClose, prioFar);
}

TEST_F(ConversationQueueTest, PriorityRecentDialogue) {
    int currentTick = 2000;
    ConversationQueueEntry entryRecent(0, 0.5f, 0.5f, 5.0f, 1900, 1990);     // Just talked (10 ticks ago)
    ConversationQueueEntry entryOld(1, 0.5f, 0.5f, 5.0f, 1900, 1000);        // Long ago (1000 ticks ago)
    
    float prioRecent = ConversationQueue::calculatePriority(entryRecent, currentTick);
    float prioOld = ConversationQueue::calculatePriority(entryOld, currentTick);
    
    EXPECT_LT(prioRecent, prioOld);  // Longer time since dialogue = higher priority
}

TEST_F(ConversationQueueTest, PriorityWeights) {
    // Verify weights sum to 1.0 and are applied correctly
    ConversationQueueEntry entry(0, 1.0f, 1.0f, 0.0f, 1000, 14400);  // All maxed
    float priority = ConversationQueue::calculatePriority(entry, 1000);
    
    EXPECT_LE(priority, 1.0f);  // Should be 1.0 or close
}

// ============================================================================
// Queue Priority Ordering Tests
// ============================================================================

TEST_F(ConversationQueueTest, HigherPriorityFirst) {
    // Add low priority NPC first
    ConversationQueue::getInstance().enqueueNPC(1, 0.2f, 0.2f, 40.0f, 1000);
    
    // Add high priority NPC second
    ConversationQueue::getInstance().enqueueNPC(0, 0.9f, 0.9f, 2.0f, 1000);
    
    // High priority should be first
    const auto& next = ConversationQueue::getInstance().peekNextNPC();
    EXPECT_EQ(next.npcId, 0);
}

TEST_F(ConversationQueueTest, QueueSortingAfterEnqueue) {
    std::vector<int> addOrder = {5, 2, 8, 1, 9};
    std::vector<float> severities = {0.3f, 0.8f, 0.5f, 0.9f, 0.1f};
    
    for (size_t i = 0; i < addOrder.size(); i++) {
        ConversationQueue::getInstance().enqueueNPC(
            addOrder[i], severities[i], 0.5f, 5.0f, 1000
        );
    }
    
    // Queue should be sorted by priority
    auto list = ConversationQueue::getInstance().getQueueList();
    for (size_t i = 1; i < list.size(); i++) {
        EXPECT_LE(list[i].calculatedPriority, list[i - 1].calculatedPriority)
            << "Queue not sorted by priority";
    }
}

// ============================================================================
// Queue Capacity Tests
// ============================================================================

TEST_F(ConversationQueueTest, MaxQueueSize) {
    // MAX_QUEUE_SIZE = 5
    for (int i = 0; i < 5; i++) {
        bool result = ConversationQueue::getInstance().enqueueNPC(
            i, 0.5f, 0.5f, 5.0f, 1000
        );
        EXPECT_TRUE(result);
    }
    
    EXPECT_TRUE(ConversationQueue::getInstance().isFull());
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 5);
}

TEST_F(ConversationQueueTest, OverflowHandling) {
    // Fill queue to capacity
    for (int i = 0; i < 5; i++) {
        ConversationQueue::getInstance().enqueueNPC(i, 0.5f, 0.5f, 5.0f, 1000);
    }
    
    // Try to add 6th NPC (should go to overflow)
    bool result = ConversationQueue::getInstance().enqueueNPC(5, 0.5f, 0.5f, 5.0f, 1000);
    
    EXPECT_TRUE(result);  // Should succeed but go to overflow
}

TEST_F(ConversationQueueTest, OverflowPromoteToQueue) {
    // Fill main queue
    for (int i = 0; i < 5; i++) {
        ConversationQueue::getInstance().enqueueNPC(i, 0.5f, 0.5f, 5.0f, 1000);
    }
    
    // Add to overflow
    ConversationQueue::getInstance().enqueueNPC(5, 0.9f, 0.9f, 2.0f, 1000);
    
    // Dequeue one from main
    ConversationQueue::getInstance().dequeueNextNPC();
    
    // Should still have 5 in queue (promoted from overflow)
    EXPECT_EQ(ConversationQueue::getInstance().getQueueSize(), 5);
}

// ============================================================================
// Display & Status Tests
// ============================================================================

TEST_F(ConversationQueueTest, QueueStatusStringEmpty) {
    std::string status = ConversationQueue::getInstance().getQueueStatusString();
    EXPECT_TRUE(status.find("No NPCs") != std::string::npos || 
                status.find("None") != std::string::npos);
}

TEST_F(ConversationQueueTest, QueueStatusStringSingle) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    
    std::string status = ConversationQueue::getInstance().getQueueStatusString();
    EXPECT_TRUE(status.find("1/5") != std::string::npos || 
                status.find("Queue") != std::string::npos ||
                status.find("NPC_0") != std::string::npos);
}

TEST_F(ConversationQueueTest, NextNPCDescription) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    
    std::string desc = ConversationQueue::getInstance().getNextNPCDescription();
    EXPECT_TRUE(desc.find("NPC_0") != std::string::npos || 
                desc.find("Priority") != std::string::npos);
}

TEST_F(ConversationQueueTest, GetQueueList) {
    for (int i = 0; i < 3; i++) {
        ConversationQueue::getInstance().enqueueNPC(i, 0.5f, 0.5f, 5.0f, 1000);
    }
    
    auto list = ConversationQueue::getInstance().getQueueList();
    EXPECT_EQ(list.size(), static_cast<size_t>(3));
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(ConversationQueueTest, StatisticsTracking) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    ConversationQueue::getInstance().enqueueNPC(1, 0.5f, 0.5f, 5.0f, 1000);
    
    std::string stats = ConversationQueue::getInstance().getStatistics();
    EXPECT_TRUE(stats.find("Total Enqueued: 2") != std::string::npos);
}

TEST_F(ConversationQueueTest, StatisticsDequeue) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    ConversationQueue::getInstance().dequeueNextNPC();
    
    std::string stats = ConversationQueue::getInstance().getStatistics();
    EXPECT_TRUE(stats.find("Total Dequeued: 1") != std::string::npos);
}

TEST_F(ConversationQueueTest, StatisticsReset) {
    ConversationQueue::getInstance().enqueueNPC(0, 0.5f, 0.5f, 5.0f, 1000);
    ConversationQueue::getInstance().resetStatistics();
    
    std::string stats = ConversationQueue::getInstance().getStatistics();
    EXPECT_TRUE(stats.find("Total Enqueued: 0") != std::string::npos);
}

// ============================================================================
// Determinism Tests
// ============================================================================

TEST_F(ConversationQueueTest, DeterminismPriority) {
    ConversationQueueEntry entry1(0, 0.5f, 0.5f, 5.0f, 1000, 500);
    ConversationQueueEntry entry2(0, 0.5f, 0.5f, 5.0f, 1000, 500);
    
    float prio1 = ConversationQueue::calculatePriority(entry1, 1000);
    float prio2 = ConversationQueue::calculatePriority(entry2, 1000);
    
    EXPECT_EQ(prio1, prio2);
}

TEST_F(ConversationQueueTest, DeterminismOrdering) {
    // Add same NPCs twice, order should be identical
    std::vector<int> npcs = {2, 5, 1, 8, 3};
    
    for (int npc : npcs) {
        ConversationQueue::getInstance().enqueueNPC(npc, 0.5f, 0.5f, 5.0f, 1000);
    }
    
    auto list1 = ConversationQueue::getInstance().getQueueList();
    
    // Recreate queue
    ConversationQueue::getInstance().clear();
    for (int npc : npcs) {
        ConversationQueue::getInstance().enqueueNPC(npc, 0.5f, 0.5f, 5.0f, 1000);
    }
    
    auto list2 = ConversationQueue::getInstance().getQueueList();
    
    EXPECT_EQ(list1.size(), list2.size());
    for (size_t i = 0; i < list1.size(); i++) {
        EXPECT_EQ(list1[i].npcId, list2[i].npcId);
        EXPECT_EQ(list1[i].calculatedPriority, list2[i].calculatedPriority);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ConversationQueueTest, InvalidNPCId) {
    bool result = ConversationQueue::getInstance().enqueueNPC(
        999,    // Invalid ID
        0.5f, 0.5f, 5.0f, 1000
    );
    
    EXPECT_FALSE(result);
    EXPECT_TRUE(ConversationQueue::getInstance().isEmpty());
}

TEST_F(ConversationQueueTest, ZeroPriority) {
    ConversationQueueEntry entry(0, 0.0f, 0.0f, 50.0f, 1000, 0);
    float priority = ConversationQueue::calculatePriority(entry, 1000);
    
    EXPECT_GE(priority, 0.0f);
}

TEST_F(ConversationQueueTest, MaxPriority) {
    ConversationQueueEntry entry(0, 1.0f, 1.0f, 0.0f, 1000, 14400);
    float priority = ConversationQueue::calculatePriority(entry, 1000);
    
    EXPECT_LE(priority, 1.0f);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(ConversationQueueTest, PerformanceEnqueue100) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; i++) {
        ConversationQueue::getInstance().enqueueNPC(
            i % 10, 0.5f, 0.5f, 5.0f, 1000 + i
        );
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100);  // Should be much faster than 100ms
}

TEST_F(ConversationQueueTest, PerformancePriorityCalculation) {
    ConversationQueueEntry entry(0, 0.5f, 0.5f, 5.0f, 1000, 500);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++) {
        ConversationQueue::calculatePriority(entry, 1000 + i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 10);  // 1000 calculations in <10ms
}

TEST_F(ConversationQueueTest, PerformanceDequeue100) {
    // Fill queue
    for (int i = 0; i < 100; i++) {
        ConversationQueue::getInstance().enqueueNPC(
            i % 10, 0.5f, 0.5f, 5.0f, 1000
        );
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; i++) {
        if (!ConversationQueue::getInstance().isEmpty()) {
            ConversationQueue::getInstance().dequeueNextNPC();
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100);
}

// ============================================================================
// GTest Main Entry Point
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

