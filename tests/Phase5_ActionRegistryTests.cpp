#include <gtest/gtest.h>
#include "ActionRegistry.h"
#include <iostream>

/**
 * Phase 5 Test Suite 1: Action Registry
 * 
 * Tests for:
 *  - Loading actions from JSON
 *  - Name lookup
 *  - Alias lookup
 *  - Validation
 *  - All actions present and accessible
 */

class ActionRegistryTests : public ::testing::Test {
protected:
    void SetUp() override {
        ActionRegistry::initialize();
    }
    
    void TearDown() override {
        // ActionRegistry::shutdown();  // Keep alive for next tests
    }
};

/**
 * Test 1: Load action registry from JSON
 */
TEST_F(ActionRegistryTests, LoadFromJSON) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    bool success = registry.loadFromJSON("data/action_registry.json");
    
    EXPECT_TRUE(success);
    EXPECT_GE(registry.getAllActions().size(), 8);
}

/**
 * Test 2: Lookup action by primary name
 */
TEST_F(ActionRegistryTests, LookupByName) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* action = registry.getActionByName("allocate");
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->name, "allocate");
    EXPECT_GE(action->aliases.size(), 1);
}

/**
 * Test 3: Lookup action by alias
 */
TEST_F(ActionRegistryTests, LookupByAlias_Give) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* action = registry.getActionByAlias("give");
    EXPECT_NE(action, nullptr);
    EXPECT_EQ(action->name, "allocate");
}

/**
 * Test 4: Lookup by different aliases
 */
TEST_F(ActionRegistryTests, LookupByAlias_Multiple) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* action1 = registry.getActionByAlias("distribute");
    ActionDefinition* action2 = registry.getActionByAlias("provide");
    ActionDefinition* action3 = registry.getActionByAlias("support");
    
    EXPECT_NE(action1, nullptr);
    EXPECT_NE(action2, nullptr);
    EXPECT_NE(action3, nullptr);
    
    // All should point to same action
    EXPECT_EQ(action1->name, "allocate");
    EXPECT_EQ(action2->name, "allocate");
    EXPECT_EQ(action3->name, "allocate");
}

/**
 * Test 5: Validate action exists
 */
TEST_F(ActionRegistryTests, ValidateActionExists) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    EXPECT_TRUE(registry.isValidAction("allocate"));
    EXPECT_TRUE(registry.isValidAction("delegate"));
    EXPECT_TRUE(registry.isValidAction("negotiate"));
    EXPECT_FALSE(registry.isValidAction("invalid_action"));
}

/**
 * Test 6: Validate alias exists
 */
TEST_F(ActionRegistryTests, ValidateAliasExists) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    EXPECT_TRUE(registry.isValidAlias("give"));
    EXPECT_TRUE(registry.isValidAlias("motivate"));
    EXPECT_FALSE(registry.isValidAlias("invalid_alias"));
}

/**
 * Test 7: Case-insensitive lookup
 */
TEST_F(ActionRegistryTests, CaseInsensitiveLookupsforName) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* action1 = registry.getActionByName("allocate");
    ActionDefinition* action2 = registry.getActionByName("ALLOCATE");
    ActionDefinition* action3 = registry.getActionByName("Allocate");
    
    EXPECT_NE(action1, nullptr);
    EXPECT_NE(action2, nullptr);
    EXPECT_NE(action3, nullptr);
    EXPECT_EQ(action1, action2);
    EXPECT_EQ(action2, action3);
}

/**
 * Test 8: Case-insensitive lookup for aliases
 */
TEST_F(ActionRegistryTests, CaseInsensitiveLookupsForAlias) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* action1 = registry.getActionByAlias("give");
    ActionDefinition* action2 = registry.getActionByAlias("GIVE");
    ActionDefinition* action3 = registry.getActionByAlias("Give");
    
    EXPECT_NE(action1, nullptr);
    EXPECT_NE(action2, nullptr);
    EXPECT_NE(action3, nullptr);
    EXPECT_EQ(action1, action2);
    EXPECT_EQ(action2, action3);
}

/**
 * Test 9: Get all actions
 */
TEST_F(ActionRegistryTests, GetAllActions) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    const auto& actions = registry.getAllActions();
    EXPECT_GE(actions.size(), 8);
    
    // Verify we can access each action
    for (const auto& action : actions) {
        EXPECT_FALSE(action.name.empty());
        EXPECT_FALSE(action.aliases.empty());
        EXPECT_GE(action.priority, 1);
        EXPECT_LE(action.priority, 10);
    }
}

/**
 * Test 10: Action parameter types
 */
TEST_F(ActionRegistryTests, ActionParameterTypes) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* allocate = registry.getActionByName("allocate");
    EXPECT_NE(allocate, nullptr);
    EXPECT_EQ(allocate->parameterTypes.size(), 2);
    EXPECT_EQ(allocate->parameterTypes[0], ParameterType::RESOURCE);
    EXPECT_EQ(allocate->parameterTypes[1], ParameterType::NPC_OR_FACTION);
}

/**
 * Test 11: Action confidence thresholds
 */
TEST_F(ActionRegistryTests, ActionConfidenceThresholds) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* allocate = registry.getActionByName("allocate");
    ActionDefinition* suppress = registry.getActionByName("suppress");
    
    EXPECT_NE(allocate, nullptr);
    EXPECT_NE(suppress, nullptr);
    
    // Suppress is riskier, should have higher threshold
    EXPECT_GT(suppress->confidenceThreshold, allocate->confidenceThreshold);
}

/**
 * Test 12: Action confirmation requirements
 */
TEST_F(ActionRegistryTests, ActionConfirmationRequirements) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    ActionDefinition* allocate = registry.getActionByName("allocate");
    ActionDefinition* suppress = registry.getActionByName("suppress");
    
    EXPECT_NE(allocate, nullptr);
    EXPECT_NE(suppress, nullptr);
    
    // Suppress is dangerous, should require confirmation
    EXPECT_FALSE(allocate->requiresConfirmation);
    EXPECT_TRUE(suppress->requiresConfirmation);
}

/**
 * Test 13: Get all action names
 */
TEST_F(ActionRegistryTests, GetAllActionNames) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    auto names = registry.getAllActionNames();
    EXPECT_GE(names.size(), 8);
    
    // Should contain specific actions
    EXPECT_NE(std::find(names.begin(), names.end(), "allocate"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "delegate"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "negotiate"), names.end());
}

/**
 * Test 14: Get all aliases
 */
TEST_F(ActionRegistryTests, GetAllAliases) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    auto aliases = registry.getAllAliases();
    EXPECT_GE(aliases.size(), 20);  // At least 20 unique aliases
    
    // Should contain specific aliases
    EXPECT_NE(std::find(aliases.begin(), aliases.end(), "give"), aliases.end());
    EXPECT_NE(std::find(aliases.begin(), aliases.end(), "motivate"), aliases.end());
}

/**
 * Test 15: Debug print registry
 */
TEST_F(ActionRegistryTests, DebugPrintRegistry) {
    ActionRegistry& registry = ActionRegistry::getInstance();
    registry.loadFromJSON("data/action_registry.json");
    
    // This should print to console without crashing
    EXPECT_NO_THROW(registry.printRegistry());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
