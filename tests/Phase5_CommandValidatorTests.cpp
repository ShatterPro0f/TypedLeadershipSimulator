#include <gtest/gtest.h>
#include "CommandValidator.h"
#include "ParameterExtractor.h"
#include "Registries.h"
#include "Core.h"
#include <iostream>

using namespace TLS;

/**
 * Phase 5.4 Command Validator Tests
 * 
 * Tests for:
 *  - Entity existence validation (NPC, Faction, Resource)
 *  - Quantity bounds validation
 *  - Resource availability checking
 *  - Permission validation
 *  - Error message generation
 *  - Suggestion generation
 *  - Dry-run validation
 */

class CommandValidatorTest : public ::testing::Test {
protected:
    NPCRegistry* npcRegistry;
    FactionRegistry* factionRegistry;
    ResourceRegistry* resourceRegistry;
    
    void SetUp() override {
        // Get singleton registry instances
        npcRegistry = &NPCRegistry::getInstance();
        factionRegistry = &FactionRegistry::getInstance();
        resourceRegistry = &ResourceRegistry::getInstance();
        
        // Clear any existing data
        npcRegistry->clear();
        factionRegistry->clear();
        resourceRegistry->clear();
        
        // Add test NPCs
        auto alice = std::make_shared<NPC>();
        alice->setName("Alice");
        alice->setId(1);
        npcRegistry->registerNPC(alice);
        
        auto bob = std::make_shared<NPC>();
        bob->setName("Bob");
        bob->setId(2);
        npcRegistry->registerNPC(bob);
        
        // Add test factions
        auto farmers = std::make_shared<Faction>();
        farmers->setName("Farmers");
        farmers->setId(100);
        factionRegistry->registerFaction(farmers);
        
        auto warriors = std::make_shared<Faction>();
        warriors->setName("Warriors");
        warriors->setId(101);
        factionRegistry->registerFaction(warriors);
        
        // Add test resources
        auto food = std::make_shared<Resource>();
        food->setName("Food");
        food->setId(200);
        food->setQuantity(100);
        resourceRegistry->registerResource(food);
        
        auto wood = std::make_shared<Resource>();
        wood->setName("Wood");
        wood->setId(201);
        wood->setQuantity(50);
        resourceRegistry->registerResource(wood);
    }
    
    void TearDown() override {
        npcRegistry->clear();
        factionRegistry->clear();
        resourceRegistry->clear();
    }
    
    ExtractedParameters createValidParameters() {
        ExtractedParameters params;
        params.confidenceScore = 0.95f;
        params.validCount = 0;
        params.invalidCount = 0;
        params.tone = "neutral";
        params.extractionReasoning = "Test parameters";
        return params;
    }
};

// ============================================================================
// NPC Validation Tests
// ============================================================================

TEST_F(CommandValidatorTest, ValidateNPCExists_ValidNPC) {
    ValidationError error = CommandValidator::validateNPCExists(1, *npcRegistry);
    EXPECT_EQ(error.severity, ValidationError::INFO);
    EXPECT_EQ(error.code, "NPC_VALID");
}

TEST_F(CommandValidatorTest, ValidateNPCExists_InvalidNPC) {
    ValidationError error = CommandValidator::validateNPCExists(999, *npcRegistry);
    EXPECT_EQ(error.severity, ValidationError::ERROR);
    EXPECT_EQ(error.code, CommandValidator::ERR_NPC_NOT_FOUND);
    EXPECT_FALSE(error.suggestions.empty());
}

// ============================================================================
// Faction Validation Tests
// ============================================================================

TEST_F(CommandValidatorTest, ValidateFactionExists_ValidFaction) {
    ValidationError error = CommandValidator::validateFactionExists(100, *factionRegistry);
    EXPECT_EQ(error.severity, ValidationError::INFO);
    EXPECT_EQ(error.code, "FACTION_VALID");
}

TEST_F(CommandValidatorTest, ValidateFactionExists_InvalidFaction) {
    ValidationError error = CommandValidator::validateFactionExists(999, *factionRegistry);
    EXPECT_EQ(error.severity, ValidationError::ERROR);
    EXPECT_EQ(error.code, CommandValidator::ERR_FACTION_NOT_FOUND);
    EXPECT_FALSE(error.suggestions.empty());
}

// ============================================================================
// Resource Validation Tests
// ============================================================================

TEST_F(CommandValidatorTest, ValidateResourceAvailable_ValidResource) {
    ValidationError error = CommandValidator::validateResourceAvailable(200, 50, *resourceRegistry);
    EXPECT_EQ(error.severity, ValidationError::INFO);
    EXPECT_EQ(error.code, "RESOURCE_VALID");
}

TEST_F(CommandValidatorTest, ValidateResourceAvailable_InvalidResourceId) {
    ValidationError error = CommandValidator::validateResourceAvailable(999, 10, *resourceRegistry);
    EXPECT_EQ(error.severity, ValidationError::ERROR);
    EXPECT_EQ(error.code, CommandValidator::ERR_RESOURCE_NOT_FOUND);
}

TEST_F(CommandValidatorTest, ValidateResourceAvailable_InsufficientQuantity) {
    // Food has 100, ask for 150
    ValidationError error = CommandValidator::validateResourceAvailable(200, 150, *resourceRegistry);
    EXPECT_EQ(error.severity, ValidationError::ERROR);
    EXPECT_EQ(error.code, CommandValidator::ERR_INSUFFICIENT_RESOURCES);
}

// ============================================================================
// Quantity Validation Tests
// ============================================================================

TEST_F(CommandValidatorTest, ValidateQuantityBounds_ValidQuantity) {
    ValidationError error = CommandValidator::validateQuantityBounds(50, "allocate", 1, 1000);
    EXPECT_EQ(error.severity, ValidationError::INFO);
    EXPECT_EQ(error.code, "QUANTITY_VALID");
}

TEST_F(CommandValidatorTest, ValidateQuantityBounds_TooLow) {
    ValidationError error = CommandValidator::validateQuantityBounds(0, "allocate", 1, 1000);
    EXPECT_EQ(error.severity, ValidationError::ERROR);
    EXPECT_EQ(error.code, CommandValidator::ERR_QUANTITY_OUT_OF_BOUNDS);
}

TEST_F(CommandValidatorTest, ValidateQuantityBounds_TooHigh) {
    ValidationError error = CommandValidator::validateQuantityBounds(2000, "allocate", 1, 1000);
    EXPECT_EQ(error.severity, ValidationError::ERROR);
    EXPECT_EQ(error.code, CommandValidator::ERR_QUANTITY_OUT_OF_BOUNDS);
}

// ============================================================================
// Self-targeting Validation
// ============================================================================

TEST_F(CommandValidatorTest, ValidateNotSelf_ValidTarget) {
    ValidationError error = CommandValidator::validateNotSelf(1);
    EXPECT_EQ(error.severity, ValidationError::INFO);
    EXPECT_EQ(error.code, "TARGET_VALID");
}

TEST_F(CommandValidatorTest, ValidateNotSelf_PlayerAsTarget) {
    ValidationError error = CommandValidator::validateNotSelf(0);
    EXPECT_EQ(error.severity, ValidationError::ERROR);
    EXPECT_EQ(error.code, CommandValidator::ERR_SELF_TARGET);
}

// ============================================================================
// Permission Validation Tests
// ============================================================================

TEST_F(CommandValidatorTest, ValidatePermission_GrantedByDefault) {
    ValidationError error = CommandValidator::validatePermission("allocate", "NPC", 1);
    EXPECT_EQ(error.severity, ValidationError::INFO);
    EXPECT_EQ(error.code, "PERMISSION_GRANTED");
}

// ============================================================================
// Error Message Formatting
// ============================================================================

TEST_F(CommandValidatorTest, FormatErrorMessage_WithSuggestions) {
    ValidationError error;
    error.severity = ValidationError::ERROR;
    error.code = CommandValidator::ERR_NPC_NOT_FOUND;
    error.message = "NPC not found";
    error.field = "target_npc";
    error.invalidValue = "unknown_npc";
    error.suggestions = {"Check spelling", "List NPCs"};
    
    std::string formatted = CommandValidator::formatErrorMessage(error);
    EXPECT_NE(formatted.find("NPC not found"), std::string::npos);
    EXPECT_NE(formatted.find("target_npc"), std::string::npos);
    EXPECT_NE(formatted.find("Check spelling"), std::string::npos);
}

TEST_F(CommandValidatorTest, FormatErrorMessage_NoSuggestions) {
    ValidationError error;
    error.severity = ValidationError::ERROR;
    error.code = "TEST_ERROR";
    error.message = "Test error message";
    error.field = "test_field";
    error.invalidValue = "test_value";
    error.suggestions = {};
    
    std::string formatted = CommandValidator::formatErrorMessage(error);
    EXPECT_NE(formatted.find("Test error message"), std::string::npos);
}

// ============================================================================
// Suggestion Generation Tests
// ============================================================================

TEST_F(CommandValidatorTest, GenerateSuggestions_EmptyCandidates) {
    std::vector<std::string> candidates;
    auto suggestions = CommandValidator::generateSuggestions("NPC_NOT_FOUND", "alice", candidates);
    EXPECT_TRUE(suggestions.empty());
}

TEST_F(CommandValidatorTest, GenerateSuggestions_FewCandidates) {
    std::vector<std::string> candidates = {"Alice", "Bob"};
    auto suggestions = CommandValidator::generateSuggestions("NPC_NOT_FOUND", "alec", candidates);
    EXPECT_LE(suggestions.size(), 2u);
}

TEST_F(CommandValidatorTest, GenerateSuggestions_ManyCandidates) {
    std::vector<std::string> candidates = {"Alice", "Bob", "Charlie", "David", "Eve"};
    auto suggestions = CommandValidator::generateSuggestions("NPC_NOT_FOUND", "alice", candidates);
    EXPECT_LE(suggestions.size(), 3u);  // Should limit to 2-3 suggestions
}

// ============================================================================
// Dry-run Validation Tests
// ============================================================================

TEST_F(CommandValidatorTest, DryRun_NoSideEffects) {
    ExtractedParameters params = createValidParameters();
    
    // Get initial resource count
    auto foodBefore = resourceRegistry->getResourceById(200);
    int quantityBefore = foodBefore ? foodBefore->getQuantity() : 0;
    
    // Run dry-run validation
    ValidationResult result = CommandValidator::dryRun(
        "allocate",
        params,
        *npcRegistry,
        *factionRegistry,
        *resourceRegistry
    );
    
    // Verify no resource was actually consumed
    auto foodAfter = resourceRegistry->getResourceById(200);
    int quantityAfter = foodAfter ? foodAfter->getQuantity() : 0;
    EXPECT_EQ(quantityBefore, quantityAfter);
}

// ============================================================================
// Complete Command Validation Tests
// ============================================================================

TEST_F(CommandValidatorTest, ValidateCommand_ValidAllParameters) {
    ExtractedParameters params = createValidParameters();
    
    // Add valid NPC parameter
    ExtractedParameter npcParam;
    npcParam.type = ExtractedParameter::NPC_ENTITY;
    npcParam.rawValue = "Alice";
    npcParam.confidence = 0.95f;
    npcParam.resolvedName = "Alice";
    npcParam.entityPtr = npcRegistry->getNPCById(1).get();
    npcParam.isValid = true;
    params.parameters.push_back(npcParam);
    
    ValidationResult result = CommandValidator::validateCommand(
        "inspire",
        params,
        *npcRegistry,
        *factionRegistry,
        *resourceRegistry
    );
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.errorCount, 0);
}

TEST_F(CommandValidatorTest, ValidateCommand_InvalidNPC) {
    ExtractedParameters params = createValidParameters();
    
    // Add invalid NPC parameter
    ExtractedParameter npcParam;
    npcParam.type = ExtractedParameter::NPC_ENTITY;
    npcParam.rawValue = "UnknownNPC";
    npcParam.confidence = 0.5f;
    npcParam.resolvedName = "UnknownNPC";
    npcParam.entityPtr = nullptr;  // Not found
    npcParam.isValid = false;
    params.parameters.push_back(npcParam);
    
    ValidationResult result = CommandValidator::validateCommand(
        "inspire",
        params,
        *npcRegistry,
        *factionRegistry,
        *resourceRegistry
    );
    
    EXPECT_FALSE(result.isValid);
    EXPECT_GT(result.errorCount, 0);
}

TEST_F(CommandValidatorTest, ValidateCommand_MultipleErrors) {
    ExtractedParameters params = createValidParameters();
    
    // Add multiple invalid parameters
    ExtractedParameter npcParam;
    npcParam.type = ExtractedParameter::NPC_ENTITY;
    npcParam.entityPtr = nullptr;
    npcParam.isValid = false;
    params.parameters.push_back(npcParam);
    
    ExtractedParameter quantityParam;
    quantityParam.type = ExtractedParameter::QUANTITY;
    quantityParam.rawValue = "invalid_number";
    quantityParam.isValid = false;
    params.parameters.push_back(quantityParam);
    
    ValidationResult result = CommandValidator::validateCommand(
        "allocate",
        params,
        *npcRegistry,
        *factionRegistry,
        *resourceRegistry
    );
    
    EXPECT_FALSE(result.isValid);
    EXPECT_GE(result.errorCount, 2);
}

TEST_F(CommandValidatorTest, ValidateCommand_Summary) {
    ExtractedParameters params = createValidParameters();
    
    ValidationResult result = CommandValidator::validateCommand(
        "allocate",
        params,
        *npcRegistry,
        *factionRegistry,
        *resourceRegistry
    );
    
    EXPECT_FALSE(result.summary.empty());
    if (result.isValid) {
        EXPECT_NE(result.summary.find("passed"), std::string::npos);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(CommandValidatorTest, IntegrationTest_AllocateToNPC) {
    ExtractedParameters params = createValidParameters();
    
    // Simulate: allocate 25 food to alice
    ExtractedParameter resourceParam;
    resourceParam.type = ExtractedParameter::RESOURCE_ENTITY;
    resourceParam.rawValue = "Food";
    resourceParam.entityPtr = resourceRegistry->getResourceById(200).get();
    params.parameters.push_back(resourceParam);
    
    ExtractedParameter npcParam;
    npcParam.type = ExtractedParameter::NPC_ENTITY;
    npcParam.rawValue = "Alice";
    npcParam.entityPtr = npcRegistry->getNPCById(1).get();
    params.parameters.push_back(npcParam);
    
    ExtractedParameter quantityParam;
    quantityParam.type = ExtractedParameter::QUANTITY;
    quantityParam.rawValue = "25";
    params.parameters.push_back(quantityParam);
    
    ValidationResult result = CommandValidator::validateCommand(
        "allocate",
        params,
        *npcRegistry,
        *factionRegistry,
        *resourceRegistry
    );
    
    EXPECT_TRUE(result.isValid);
    EXPECT_EQ(result.errorCount, 0);
}

TEST_F(CommandValidatorTest, IntegrationTest_DelegateToNPC) {
    ExtractedParameters params = createValidParameters();
    
    // Simulate: delegate task to bob
    ExtractedParameter npcParam;
    npcParam.type = ExtractedParameter::NPC_ENTITY;
    npcParam.rawValue = "Bob";
    npcParam.entityPtr = npcRegistry->getNPCById(2).get();
    params.parameters.push_back(npcParam);
    
    ExtractedParameter toneParam;
    toneParam.type = ExtractedParameter::TONE;
    toneParam.rawValue = "positive";
    params.parameters.push_back(toneParam);
    
    ValidationResult result = CommandValidator::validateCommand(
        "delegate",
        params,
        *npcRegistry,
        *factionRegistry,
        *resourceRegistry
    );
    
    EXPECT_TRUE(result.isValid);
}
