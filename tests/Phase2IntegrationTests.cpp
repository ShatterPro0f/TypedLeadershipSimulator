#include <gtest/gtest.h>
#include "LLM.h"
#include "EntityFactory.h"
#include "Registries.h"

using namespace TLS;

// ============================================================================
// Test Fixture for Phase 2 Integration Tests
// ============================================================================

class Phase2IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize LLM Manager for tests
        auto llmMgr = LLMManager::instance();
        LLMConfig config;
        config.provider = LLMConfig::OFFLINE_FALLBACK;
        llmMgr->initialize(config);
    }
    
    void TearDown() override {
        // Clean up if needed
        LLMManager::instance()->shutdown();
    }
};

// ============================================================================
// LLM Framework Tests
// ============================================================================

TEST_F(Phase2IntegrationTest, LLMManagerInitialization) {
    auto llmMgr = LLMManager::instance();
    ASSERT_NE(llmMgr, nullptr);
    
    LLMConfig config;
    config.provider = LLMConfig::OFFLINE_FALLBACK;
    bool initialized = llmMgr->initialize(config);
    
    EXPECT_TRUE(initialized);
    EXPECT_NE(llmMgr->getProvider(), nullptr);
}

TEST_F(Phase2IntegrationTest, LLMResponseGeneration) {
    auto mgr = LLMManager::instance();
    LLMResponse resp = mgr->interpretPlayerDecision("feed people", "Settlement needs food");
    
    EXPECT_TRUE(resp.success);
    EXPECT_FALSE(resp.content.empty());
}

TEST_F(Phase2IntegrationTest, LLMRequestQueue) {
    auto mgr = LLMManager::instance();
    LLMResponse resp = mgr->generateNarrative("Food is scarce");
    
    EXPECT_TRUE(resp.success);
}

TEST_F(Phase2IntegrationTest, WorldStateSnapshot) {
    WorldStateSnapshot snapshot;
    snapshot.tickNumber = 1000;
    snapshot.significantNPCIds = {1, 2, 3};
    snapshot.affectedFactionIds = {10};
    snapshot.changedResourceIds = {30};
    
    EXPECT_EQ(snapshot.tickNumber, 1000);
    EXPECT_EQ(snapshot.significantNPCIds.size(), static_cast<size_t>(3));
}

TEST_F(Phase2IntegrationTest, LLMUsageTracking) {
    auto mgr = LLMManager::instance();
    LLMResponse resp = mgr->generateNarrative("Track usage test");
    
    EXPECT_GE(resp.inputTokens, 0);
    EXPECT_GE(resp.completionTokens, 0);
}

// ============================================================================
// EntityFactory Tests
// ============================================================================

TEST_F(Phase2IntegrationTest, EntityFactoryNPCCreation) {
    auto factory = EntityFactory::instance();
    auto npc = factory->createNPC("Alice", 30, "female", "merchant", 1);
    
    ASSERT_NE(npc, nullptr);
    EXPECT_EQ(npc->getName(), "Alice");
    EXPECT_EQ(npc->getAge(), 30);
    EXPECT_GT(npc->getId(), 0);  // Should have valid ID
}

TEST_F(Phase2IntegrationTest, EntityFactoryAdvisorCreation) {
    auto factory = EntityFactory::instance();
    auto advisor = factory->createAdvisor("Lord Chancellor", Specialty::POLITICS, 1);
    
    ASSERT_NE(advisor, nullptr);
    EXPECT_EQ(advisor->getName(), "Lord Chancellor");
    EXPECT_EQ(advisor->getSpecialty(), Specialty::POLITICS);
    EXPECT_GT(advisor->getId(), 0);  // Should have valid ID
}

TEST_F(Phase2IntegrationTest, EntityFactoryResourceCreation) {
    auto factory = EntityFactory::instance();
    auto resource = factory->createResource("Food", 200);
    
    ASSERT_NE(resource, nullptr);
    EXPECT_EQ(resource->getName(), "Food");
    EXPECT_EQ(resource->getQuantity(), 200);
    EXPECT_GT(resource->getId(), 0);  // Should have valid ID
}

TEST_F(Phase2IntegrationTest, EntityFactoryFactionCreation) {
    auto factory = EntityFactory::instance();
    auto faction = factory->createFaction("Farmers");
    
    ASSERT_NE(faction, nullptr);
    EXPECT_EQ(faction->getName(), "Farmers");
    EXPECT_GT(faction->getId(), 0);  // Should have valid ID
}

TEST_F(Phase2IntegrationTest, EntityFactoryEventCreation) {
    auto factory = EntityFactory::instance();
    auto event = factory->createEvent("Famine", EventType::ENVIRONMENTAL, 8);
    
    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->getName(), "Famine");
    EXPECT_EQ(event->getType(), EventType::ENVIRONMENTAL);
    EXPECT_EQ(event->getImpactLevel(), 8);
    EXPECT_GT(event->getId(), 0);  // Should have valid ID
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
