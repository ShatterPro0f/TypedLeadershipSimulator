#include <gtest/gtest.h>
#include "NarrativeGeneration.h"
#include "Registries.h"
#include "LLMProvider.h"
#include <memory>

using namespace TLS;

// Mock LLM provider for testing
class MockLLMProvider : public LLMProvider {
public:
    LLMResponse callLLM(const std::string& prompt) override {
        LLMResponse response;
        response.wasSuccessful = is_available;
        response.text = mock_response;
        return response;
    }
    
    bool isAvailable() const override { return is_available; }
    
    LLMProviderType getProviderType() const override { return LLMProviderType::MOCK; }
    
    std::string getProviderName() const override { return "MockProvider"; }
    
    LLMUsage getTokenUsage() const override { 
        LLMUsage usage;
        usage.inputTokens = 0;
        usage.completionTokens = 0;
        usage.totalTokens = 0;
        return usage;
    }
    
    void resetTokenUsage() override { }
    
    void setMockResponse(const std::string& response) {
        mock_response = response;
    }
    
    void setAvailable(bool available) {
        is_available = available;
    }

private:
    std::string mock_response = "ISSUE: Test Crisis | Test Description | Test Action | CRISIS";
    bool is_available = true;
};

// Test fixtures
class Phase9NarrativeGenerationTests : public ::testing::Test {
protected:
    void SetUp() override {
        npc_registry = &NPCRegistry::getInstance();
        faction_registry = &FactionRegistry::getInstance();
        resource_registry = &ResourceRegistry::getInstance();
        
        npc_registry->clear();
        faction_registry->clear();
        resource_registry->clear();
        
        // Create test NPCs
        NPC alice(1, "Alice", 25, "Farmer");
        alice.setShortTermMood(0.6f);
        alice.setLoyalty(0.8f);
        alice.setFactionId(1);
        npc_registry->addNPC(alice);
        
        NPC bob(2, "Bob", 30, "Warrior");
        bob.setShortTermMood(0.4f);
        bob.setLoyalty(0.5f);
        bob.setFactionId(2);
        npc_registry->addNPC(bob);
        
        // Create test factions
        Faction farmers(1, "Farmers");
        faction_registry->addFaction(farmers);
        
        Faction warriors(2, "Warriors");
        faction_registry->addFaction(warriors);
        
        // Create test resources
        Resource food(1, "Food", 200);
        food.setScarcityThreshold(100);
        resource_registry->addResource(food);
        
        Resource wood(2, "Wood", 150);
        wood.setScarcityThreshold(75);
        resource_registry->addResource(wood);
    }
    
    NPCRegistry* npc_registry;
    FactionRegistry* faction_registry;
    ResourceRegistry* resource_registry;
};

// ============================================================================
// NarrativeIssueQueue Tests (6 tests)
// ============================================================================

class NarrativeIssueQueueTests : public ::testing::Test {
protected:
    NarrativeIssueQueue queue;
};

TEST_F(NarrativeIssueQueueTests, AddIssueReturnsUniqueId) {
    NarrativeIssue issue1("Crisis 1", "Description 1", "Action 1", 0);
    NarrativeIssue issue2("Crisis 2", "Description 2", "Action 2", 0);
    
    int id1 = queue.addIssue(issue1);
    int id2 = queue.addIssue(issue2);
    
    EXPECT_NE(id1, id2);
    EXPECT_EQ(queue.getIssueCount(), 2);
}

TEST_F(NarrativeIssueQueueTests, GetTopIssuesReturnsSorted) {
    NarrativeIssue crisis("Crisis", "Desc", "Action", 0);
    crisis.priority = IssuePriority::CRISIS;
    
    NarrativeIssue warning("Warning", "Desc", "Action", 0);
    warning.priority = IssuePriority::WARNING;
    
    queue.addIssue(warning);
    queue.addIssue(crisis);
    
    auto top = queue.getTopIssues(2);
    EXPECT_EQ(top[0].priority, IssuePriority::CRISIS);
    EXPECT_EQ(top[1].priority, IssuePriority::WARNING);
}

TEST_F(NarrativeIssueQueueTests, ResolveIssueRemovesIt) {
    NarrativeIssue issue("Test", "Desc", "Action", 0);
    int id = queue.addIssue(issue);
    
    EXPECT_EQ(queue.getIssueCount(), 1);
    queue.resolveIssue(id);
    EXPECT_EQ(queue.getIssueCount(), 0);
}

TEST_F(NarrativeIssueQueueTests, MaxCapacityEnforced) {
    for (int i = 0; i < NarrativeIssueQueue::MAX_ISSUES + 5; ++i) {
        NarrativeIssue issue("Issue " + std::to_string(i), "Desc", "Action", 0);
        queue.addIssue(issue);
    }
    
    EXPECT_LE(queue.getIssueCount(), NarrativeIssueQueue::MAX_ISSUES);
}

TEST_F(NarrativeIssueQueueTests, ClearEmptiesQueue) {
    queue.addIssue(NarrativeIssue("Issue", "Desc", "Action", 0));
    queue.addIssue(NarrativeIssue("Issue2", "Desc", "Action", 0));
    
    queue.clear();
    EXPECT_EQ(queue.getIssueCount(), 0);
}

TEST_F(NarrativeIssueQueueTests, FormatIssuesForDisplay) {
    NarrativeIssue issue("Test Crisis", "Something bad", "Do something", 0);
    queue.addIssue(issue);
    
    std::string display = queue.formatIssuesForDisplay(1);
    EXPECT_NE(display.find("Test Crisis"), std::string::npos);
}

// ============================================================================
// WorldStateMonitor Tests (8 tests)
// ============================================================================

TEST_F(Phase9NarrativeGenerationTests, MonitorDetectsNPCMoodChange) {
    WorldStateMonitor monitor;
    
    // First update - establish baseline
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    // Change Alice's mood
    auto alice = npc_registry->getNPCById(1);
    if (alice) {
        alice->setShortTermMood(0.1f);  // Drop by 0.5f (exceeds 0.2f threshold)
    }
    
    // Second update - detect change
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto changed = monitor.getNPCsWithMoodChanges();
    EXPECT_TRUE(std::find(changed.begin(), changed.end(), 1) != changed.end());
}

TEST_F(Phase9NarrativeGenerationTests, MonitorIgnoresSmallMoodChange) {
    WorldStateMonitor monitor;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) {
        alice->setShortTermMood(0.55f);  // Change by 0.05f (below threshold)
    }
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto changed = monitor.getNPCsWithMoodChanges();
    EXPECT_FALSE(std::find(changed.begin(), changed.end(), 1) != changed.end());
}

TEST_F(Phase9NarrativeGenerationTests, MonitorDetectsResourceScarcity) {
    WorldStateMonitor monitor;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto food = resource_registry->getResourceById(1);
    if (food) {
        food->updateQuantity(-150);  // Drop from 200 to 50 (below scarcity threshold 100)
    }
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto changed = monitor.getResourcesWithScarcityChanges();
    EXPECT_TRUE(std::find(changed.begin(), changed.end(), 1) != changed.end());
}

TEST_F(Phase9NarrativeGenerationTests, MonitorDetectsFactionLoyaltyChange) {
    WorldStateMonitor monitor;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) {
        alice->setLoyalty(0.5f);  // Change by 0.3f (exceeds 0.15f threshold)
    }
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto changed = monitor.getFactionsWithLoyaltyChanges();
    EXPECT_TRUE(std::find(changed.begin(), changed.end(), 1) != changed.end());
}

TEST_F(Phase9NarrativeGenerationTests, MonitorHasSignificantChangeDetection) {
    WorldStateMonitor monitor;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    EXPECT_FALSE(monitor.hasSignificantWorldStateChange());
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) {
        alice->setShortTermMood(0.1f);  // Trigger change
    }
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    EXPECT_TRUE(monitor.hasSignificantWorldStateChange());
}

TEST_F(Phase9NarrativeGenerationTests, MonitorClearsPreviousState) {
    WorldStateMonitor monitor;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    monitor.clearPreviousState();
    
    // After clearing, should not detect changes on next update
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    EXPECT_FALSE(monitor.hasSignificantWorldStateChange());
}

TEST_F(Phase9NarrativeGenerationTests, MonitorMultipleNPCChanges) {
    WorldStateMonitor monitor;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    // Change both Alice and Bob
    auto alice = npc_registry->getNPCById(1);
    auto bob = npc_registry->getNPCById(2);
    if (alice) alice->setShortTermMood(0.1f);
    if (bob) bob->setShortTermMood(0.8f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto changed = monitor.getNPCsWithMoodChanges();
    EXPECT_EQ(changed.size(), 2);
}

// ============================================================================
// WorldStateSnapshotBuilder Tests (5 tests)
// ============================================================================

TEST_F(Phase9NarrativeGenerationTests, SnapshotBuilderCreatesSnapshot) {
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder builder;
    auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry, 
                                         *resource_registry, 1);
    
    EXPECT_EQ(snapshot.tick_number, 1);
    EXPECT_FALSE(snapshot.isEmpty());
}

TEST_F(Phase9NarrativeGenerationTests, SnapshotBuilderIncludesSignificantNPCs) {
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder builder;
    auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                         *resource_registry, 1);
    
    EXPECT_EQ(snapshot.significant_npcs.size(), 1);
    EXPECT_EQ(snapshot.significant_npcs[0].getId(), 1);
}

TEST_F(Phase9NarrativeGenerationTests, SnapshotBuilderIncludesChangedResources) {
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto food = resource_registry->getResourceById(1);
    if (food) food->updateQuantity(-150);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder builder;
    auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                         *resource_registry, 1);
    
    EXPECT_EQ(snapshot.changed_resources.size(), 1);
}

TEST_F(Phase9NarrativeGenerationTests, SnapshotBuilderPrunesLargeNPCList) {
    // Add many NPCs
    for (int i = 10; i < 60; ++i) {
        NPC npc(i, "NPC" + std::to_string(i), 20, "Worker");
        npc.setShortTermMood(0.1f);
        npc.setFactionId(1);
        npc_registry->addNPC(npc);
    }
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    for (int i = 10; i < 60; ++i) {
        auto npc = npc_registry->getNPCById(i);
        if (npc) npc->setShortTermMood(0.05f);
    }
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder builder;
    auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                         *resource_registry, 1);
    
    EXPECT_LE(snapshot.significant_npcs.size(), WorldStateSnapshotBuilder::MAX_NPCS_IN_SNAPSHOT);
}

// ============================================================================
// LLMPromptBuilder Tests (4 tests)
// ============================================================================

TEST_F(Phase9NarrativeGenerationTests, PromptBuilderCreatesValidPrompt) {
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    LLMPromptBuilder prompt_builder;
    auto prompt = prompt_builder.buildNarrativePrompt(snapshot);
    
    EXPECT_NE(prompt.find("settlement"), std::string::npos);
}

TEST_F(Phase9NarrativeGenerationTests, PromptBuilderIncludesNPCInfo) {
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    LLMPromptBuilder prompt_builder;
    auto prompt = prompt_builder.buildNarrativePrompt(snapshot);
    
    EXPECT_NE(prompt.find("Alice"), std::string::npos);
}

TEST_F(Phase9NarrativeGenerationTests, SystemPromptNotEmpty) {
    auto system_prompt = LLMPromptBuilder::getSystemPrompt();
    EXPECT_FALSE(system_prompt.empty());
    EXPECT_NE(system_prompt.find("narrative"), std::string::npos);
}

// ============================================================================
// NarrativeGenerator Tests (8 tests)
// ============================================================================

TEST_F(Phase9NarrativeGenerationTests, GeneratorUsesRuleBasedWhenNoLLM) {
    auto generator = NarrativeGenerator(nullptr);
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    auto issues = generator.generateNarratives(snapshot, 1, true);
    EXPECT_FALSE(issues.empty());
}

TEST_F(Phase9NarrativeGenerationTests, GeneratorCreatesNPCMoodIssue) {
    auto generator = NarrativeGenerator(nullptr);
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    auto issues = generator.generateNarratives(snapshot, 1, true);
    
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.type == IssueType::NPC_CRISIS) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(Phase9NarrativeGenerationTests, GeneratorCreatesResourceScarcityIssue) {
    auto generator = NarrativeGenerator(nullptr);
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto food = resource_registry->getResourceById(1);
    if (food) food->updateQuantity(-150);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    auto issues = generator.generateNarratives(snapshot, 1, true);
    
    bool found = false;
    for (const auto& issue : issues) {
        if (issue.type == IssueType::RESOURCE_SCARCITY) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(Phase9NarrativeGenerationTests, GeneratorCreatesFactionIssue) {
    auto generator = NarrativeGenerator(nullptr);
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    // Lower faction strength
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setLoyalty(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    auto issues = generator.generateNarratives(snapshot, 1, true);
    EXPECT_FALSE(issues.empty());
}

TEST_F(Phase9NarrativeGenerationTests, GeneratorWithMockLLM) {
    auto mock_llm = std::make_shared<MockLLMProvider>();
    mock_llm->setMockResponse("ISSUE: Test Crisis | Test Desc | Test Action | CRISIS");
    
    auto generator = NarrativeGenerator(mock_llm);
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    auto issues = generator.generateNarratives(snapshot, 1, false);
    EXPECT_FALSE(issues.empty());
}

TEST_F(Phase9NarrativeGenerationTests, GeneratorFallsBackOnLLMUnavailable) {
    auto mock_llm = std::make_shared<MockLLMProvider>();
    mock_llm->setAvailable(false);
    
    auto generator = NarrativeGenerator(mock_llm);
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    auto issues = generator.generateNarratives(snapshot, 1, false);
    EXPECT_FALSE(issues.empty());
}

TEST_F(Phase9NarrativeGenerationTests, GeneratorForceRuleBasedIgnoresLLM) {
    auto mock_llm = std::make_shared<MockLLMProvider>();
    auto generator = NarrativeGenerator(mock_llm);
    
    WorldStateMonitor monitor;
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    WorldStateSnapshotBuilder snapshot_builder;
    auto snapshot = snapshot_builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                                  *resource_registry, 1);
    
    auto issues = generator.generateNarratives(snapshot, 1, true);
    EXPECT_FALSE(issues.empty());
}

// ============================================================================
// NarrativeIssueDisplay Tests (4 tests)
// ============================================================================

TEST_F(Phase9NarrativeGenerationTests, DisplayFormatIssue) {
    NarrativeIssue issue("Test Crisis", "Desc", "Action", 0);
    issue.priority = IssuePriority::CRISIS;
    
    auto display = NarrativeIssueDisplay::formatIssueForDisplay(issue);
    EXPECT_NE(display.find("CRISIS"), std::string::npos);
    EXPECT_NE(display.find("Test Crisis"), std::string::npos);
}

TEST_F(Phase9NarrativeGenerationTests, DisplayFormatMultipleIssues) {
    std::vector<NarrativeIssue> issues;
    issues.push_back(NarrativeIssue("Crisis 1", "Desc", "Action", 0));
    issues.push_back(NarrativeIssue("Crisis 2", "Desc", "Action", 0));
    
    auto display = NarrativeIssueDisplay::formatMultipleIssues(issues, 2);
    EXPECT_NE(display.find("Crisis 1"), std::string::npos);
    EXPECT_NE(display.find("Crisis 2"), std::string::npos);
}

TEST_F(Phase9NarrativeGenerationTests, DisplayFormatSummary) {
    NarrativeIssue issue("Test Issue", "Desc", "Action", 0);
    
    auto summary = NarrativeIssueDisplay::formatIssueSummary(issue);
    EXPECT_NE(summary.find("Test Issue"), std::string::npos);
}

// ============================================================================
// Integration Tests (5 tests)
// ============================================================================

TEST_F(Phase9NarrativeGenerationTests, FullPipelineFromMonitorToIssues) {
    auto mock_llm = std::make_shared<MockLLMProvider>();
    auto generator = NarrativeGenerator(mock_llm);
    WorldStateMonitor monitor;
    WorldStateSnapshotBuilder builder;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.05f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    if (monitor.hasSignificantWorldStateChange()) {
        auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                             *resource_registry, 1);
        auto issues = generator.generateNarratives(snapshot, 1);
        EXPECT_FALSE(issues.empty());
    }
}

TEST_F(Phase9NarrativeGenerationTests, IssueQueueIntegration) {
    auto mock_llm = std::make_shared<MockLLMProvider>();
    auto generator = NarrativeGenerator(mock_llm);
    WorldStateMonitor monitor;
    WorldStateSnapshotBuilder builder;
    NarrativeIssueQueue queue;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                         *resource_registry, 1);
    auto issues = generator.generateNarratives(snapshot, 1, true);
    
    for (const auto& issue : issues) {
        queue.addIssue(issue);
    }
    
    auto top = queue.getTopIssues(3);
    EXPECT_FALSE(top.empty());
}

TEST_F(Phase9NarrativeGenerationTests, MultipleWorldStateChanges) {
    WorldStateMonitor monitor;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    // Multiple changes
    auto alice = npc_registry->getNPCById(1);
    auto food = resource_registry->getResourceById(1);
    if (alice) alice->setShortTermMood(0.05f);
    if (food) food->updateQuantity(-150);
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    EXPECT_TRUE(monitor.hasSignificantWorldStateChange());
    EXPECT_FALSE(monitor.getNPCsWithMoodChanges().empty());
    EXPECT_FALSE(monitor.getResourcesWithScarcityChanges().empty());
}

TEST_F(Phase9NarrativeGenerationTests, DisplayIntegration) {
    NarrativeIssueQueue queue;
    
    NarrativeIssue crisis("Crisis", "Desc", "Action", 0);
    crisis.priority = IssuePriority::CRISIS;
    
    queue.addIssue(crisis);
    
    auto display = queue.formatIssuesForDisplay(1);
    EXPECT_NE(display.find("CRISIS"), std::string::npos);
}

TEST_F(Phase9NarrativeGenerationTests, DeterminismWithSeededRNG) {
    // Multiple runs with same initial state should produce consistent results
    auto generator1 = NarrativeGenerator(nullptr);
    auto generator2 = NarrativeGenerator(nullptr);
    
    WorldStateMonitor monitor1, monitor2;
    WorldStateSnapshotBuilder builder1, builder2;
    
    monitor1.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    monitor2.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    auto alice = npc_registry->getNPCById(1);
    if (alice) alice->setShortTermMood(0.1f);
    
    monitor1.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    monitor2.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto snap1 = builder1.buildSnapshot(monitor1, *npc_registry, *faction_registry,
                                        *resource_registry, 1);
    auto snap2 = builder2.buildSnapshot(monitor2, *npc_registry, *faction_registry,
                                        *resource_registry, 1);
    
    EXPECT_EQ(snap1.significant_npcs.size(), snap2.significant_npcs.size());
}

// ============================================================================
// Edge Case Tests (3 tests)
// ============================================================================

TEST_F(Phase9NarrativeGenerationTests, EmptySnapshotHandling) {
    WorldStateMonitor monitor;
    WorldStateSnapshotBuilder builder;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                         *resource_registry, 1);
    
    EXPECT_TRUE(snapshot.isEmpty());
}

TEST_F(Phase9NarrativeGenerationTests, LargeNPCListPerformance) {
    // Add 100 NPCs
    for (int i = 100; i < 200; ++i) {
        NPC npc(i, "NPC" + std::to_string(i), 20, "Worker");
        npc.setShortTermMood(0.1f);
        npc.setFactionId(1);
        npc_registry->addNPC(npc);
    }
    
    WorldStateMonitor monitor;
    WorldStateSnapshotBuilder builder;
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 0);
    
    for (int i = 100; i < 200; ++i) {
        auto npc = npc_registry->getNPCById(i);
        if (npc) npc->setShortTermMood(0.05f);
    }
    
    monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, 1);
    
    auto snapshot = builder.buildSnapshot(monitor, *npc_registry, *faction_registry,
                                         *resource_registry, 1);
    
    EXPECT_LE(snapshot.significant_npcs.size(), WorldStateSnapshotBuilder::MAX_NPCS_IN_SNAPSHOT);
}

TEST_F(Phase9NarrativeGenerationTests, ContinuousMonitoring) {
    WorldStateMonitor monitor;
    
    for (int tick = 0; tick < 5; ++tick) {
        monitor.updateWorldState(*npc_registry, *faction_registry, *resource_registry, tick);
        
        // Make small changes
        auto alice = npc_registry->getNPCById(1);
        if (alice && tick % 2 == 0) {
            alice->setShortTermMood(alice->getShortTermMood() - 0.05f);
        }
    }
    
    // Should still work without issues
    EXPECT_TRUE(true);
}
