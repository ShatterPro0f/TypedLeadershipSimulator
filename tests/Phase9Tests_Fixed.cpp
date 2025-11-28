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
    
    LLMProviderType getProviderType() const override { return LLMProviderType::OFFLINE_FALLBACK; }
    
    std::string getProviderName() const override { return "MockProvider"; }
    
    LLMUsage getTokenUsage() const override { 
        LLMUsage usage;
        usage.totalInputTokens = 0;
        usage.totalCompletionTokens = 0;
        usage.totalRequests = 0;
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

// ============================================================================
// Basic NarrativeGeneration Compilation Test
// ============================================================================

TEST(Phase9NarrativeGenerationTests, NarrativeGenerationCompiles) {
    // This test verifies that NarrativeGeneration.h and related classes compile
    // without errors and instantiate correctly
    
    // Verify we can create the structures from NarrativeGeneration.h
    NarrativeIssue issue(1, "Test Crisis", "A test crisis description", 
                         "Action to take", IssuePriority::CRISIS, 
                         IssueType::RESOURCE_SCARCITY, 0);
    EXPECT_EQ(issue.type, IssueType::RESOURCE_SCARCITY);
    
    // Verify we can create the queue
    NarrativeIssueQueue queue;
    int issue_id = queue.addIssue(issue);
    EXPECT_GT(issue_id, 0);
    EXPECT_EQ(queue.getIssueCount(), 1);
}

TEST(Phase9NarrativeGenerationTests, MockLLMProviderWorks) {
    MockLLMProvider provider;
    
    EXPECT_TRUE(provider.isAvailable());
    
    auto response = provider.callLLM("test prompt");
    EXPECT_TRUE(response.wasSuccessful);
    EXPECT_FALSE(response.text.empty());
}

TEST(Phase9NarrativeGenerationTests, NarrativeIssueQueueAddMultiple) {
    NarrativeIssueQueue queue;
    
    NarrativeIssue issue1(1, "Crisis 1", "Desc 1", "Action 1", 
                          IssuePriority::CRISIS, IssueType::FACTION_CONFLICT, 0);
    NarrativeIssue issue2(2, "Crisis 2", "Desc 2", "Action 2", 
                          IssuePriority::WARNING, IssueType::RESOURCE_SCARCITY, 0);
    
    int id1 = queue.addIssue(issue1);
    int id2 = queue.addIssue(issue2);
    
    EXPECT_NE(id1, id2);
    EXPECT_EQ(queue.getIssueCount(), 2);
}

TEST(Phase9NarrativeGenerationTests, IssuePriorityEnum) {
    // Test that IssuePriority enum values exist
    IssuePriority crisis = IssuePriority::CRISIS;
    IssuePriority opportunity = IssuePriority::OPPORTUNITY;
    IssuePriority warning = IssuePriority::WARNING;
    
    EXPECT_NE(crisis, opportunity);
    EXPECT_NE(opportunity, warning);
}

TEST(Phase9NarrativeGenerationTests, IssueTypeEnum) {
    // Test that IssueType enum values exist
    IssueType faction = IssueType::FACTION_CONFLICT;
    IssueType resource = IssueType::RESOURCE_SCARCITY;
    IssueType npc = IssueType::NPC_CRISIS;
    
    EXPECT_NE(faction, resource);
    EXPECT_NE(resource, npc);
}

TEST(Phase9NarrativeGenerationTests, NarrativeIssueProperties) {
    NarrativeIssue issue(1, "Test", "Desc", "Act", 
                         IssuePriority::CRISIS, IssueType::RESOURCE_SCARCITY, 0);
    
    EXPECT_EQ(issue.title, "Test");
    EXPECT_EQ(issue.description, "Desc");
    EXPECT_EQ(issue.suggestion, "Act");
    EXPECT_EQ(issue.priority, IssuePriority::CRISIS);
    EXPECT_EQ(issue.type, IssueType::RESOURCE_SCARCITY);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
