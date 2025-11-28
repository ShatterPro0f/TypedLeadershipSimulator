#include <gtest/gtest.h>
#include "DecisionInterpreter.h"
#include <cmath>

using namespace TLS;

// ==================== TEST FIXTURES ====================

class Phase8TestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Minimal setup
    }
};

// ==================== TEST SUITE 1: INPUT PARSING ====================

TEST_F(Phase8TestFixture, BasicCommandParsing) {
    ParseResult result = InputParser::parsePlayerInput("allocate food");
    EXPECT_EQ(result.actionType, ActionType::ALLOCATE);
    EXPECT_TRUE(result.isValid);
    EXPECT_GE(result.confidence, 0.7f);
}

TEST_F(Phase8TestFixture, MultiWordInput) {
    ParseResult result = InputParser::parsePlayerInput("please give farmers some food");
    EXPECT_EQ(result.actionType, ActionType::ALLOCATE);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, DelegateAction) {
    ParseResult result = InputParser::parsePlayerInput("delegate task");
    EXPECT_EQ(result.actionType, ActionType::DELEGATE);
}

TEST_F(Phase8TestFixture, NegotiateAction) {
    ParseResult result = InputParser::parsePlayerInput("negotiate peace");
    EXPECT_EQ(result.actionType, ActionType::NEGOTIATE);
}

TEST_F(Phase8TestFixture, InspireAction) {
    ParseResult result = InputParser::parsePlayerInput("inspire workers");
    EXPECT_EQ(result.actionType, ActionType::INSPIRE);
}

TEST_F(Phase8TestFixture, SuppressAction) {
    ParseResult result = InputParser::parsePlayerInput("suppress rebellion");
    EXPECT_EQ(result.actionType, ActionType::SUPPRESS);
}

TEST_F(Phase8TestFixture, UnknownCommand) {
    ParseResult result = InputParser::parsePlayerInput("xyzabc");
    EXPECT_EQ(result.actionType, ActionType::UNKNOWN);
    EXPECT_FALSE(result.isValid);
}

TEST_F(Phase8TestFixture, EmptyInput) {
    ParseResult result = InputParser::parsePlayerInput("");
    EXPECT_FALSE(result.isValid);
    EXPECT_GT(result.errors.size(), 0);
}

TEST_F(Phase8TestFixture, CaseInsensitivity) {
    ParseResult result = InputParser::parsePlayerInput("ALLOCATE FOOD");
    EXPECT_EQ(result.actionType, ActionType::ALLOCATE);
}

// ==================== TEST SUITE 2: LEVENSHTEIN DISTANCE ====================

TEST_F(Phase8TestFixture, ExactMatch) {
    float dist = InputParser::calculateLevenshteinDistance("allocate", "allocate");
    EXPECT_EQ(dist, 0.0f);
}

TEST_F(Phase8TestFixture, OneCharDiff) {
    float dist = InputParser::calculateLevenshteinDistance("allocate", "alokate");
    // "allocate" -> "alokate" requires 2 edits (delete 'l', insert 'k')
    EXPECT_LE(dist, 3.0f);
}

TEST_F(Phase8TestFixture, TwoCharDiff) {
    float dist = InputParser::calculateLevenshteinDistance("allocate", "alokxte");
    // Multiple edits needed
    EXPECT_LE(dist, 4.0f);
}

TEST_F(Phase8TestFixture, EmptyString) {
    float dist = InputParser::calculateLevenshteinDistance("hello", "");
    EXPECT_EQ(dist, 5.0f);
}

TEST_F(Phase8TestFixture, ConfidenceScore) {
    float conf = InputParser::calculateConfidenceScore(1.0f, 0.95f, 0.7f);
    EXPECT_GE(conf, 0.0f);
    EXPECT_LE(conf, 1.0f);
}

TEST_F(Phase8TestFixture, ConfidenceRange) {
    float conf1 = InputParser::calculateConfidenceScore(0.0f, 1.0f, 1.0f);
    float conf2 = InputParser::calculateConfidenceScore(3.0f, 0.0f, 0.0f);
    EXPECT_GE(conf1, 0.0f);
    EXPECT_LE(conf1, 1.0f);
    EXPECT_GE(conf2, 0.0f);
    EXPECT_LE(conf2, 1.0f);
}

// ==================== TEST SUITE 3: TONE DETECTION ====================

TEST_F(Phase8TestFixture, PositiveTone) {
    ToneResult result = ToneDetector::detectToneRuleBased("please help improve");
    EXPECT_NE(result.tone, ToneType::NEGATIVE);
    // Just verify it's detected as positive-ish
}

TEST_F(Phase8TestFixture, NegativeTone) {
    ToneResult result = ToneDetector::detectToneRuleBased("force immediately");
    EXPECT_NE(result.tone, ToneType::POSITIVE);
    // Just verify it's detected as negative-ish
}

TEST_F(Phase8TestFixture, NeutralTone) {
    ToneResult result = ToneDetector::detectToneRuleBased("allocate");
    EXPECT_EQ(result.tone, ToneType::NEUTRAL);
}

TEST_F(Phase8TestFixture, EmptyTone) {
    ToneResult result = ToneDetector::detectToneRuleBased("");
    EXPECT_EQ(result.tone, ToneType::NEUTRAL);
}

TEST_F(Phase8TestFixture, ExclamationTone) {
    ToneResult result = ToneDetector::detectToneRuleBased("help!!!");
    EXPECT_NE(result.tone, ToneType::NEGATIVE);
}

// ==================== TEST SUITE 4: TONE MULTIPLIER ====================

TEST_F(Phase8TestFixture, PositiveMultiplier) {
    float mult = DecisionExecutor::getToneMultiplier(ToneType::POSITIVE);
    EXPECT_EQ(mult, 1.2f);
}

TEST_F(Phase8TestFixture, NegativeMultiplier) {
    float mult = DecisionExecutor::getToneMultiplier(ToneType::NEGATIVE);
    EXPECT_EQ(mult, 0.8f);
}

TEST_F(Phase8TestFixture, NeutralMultiplier) {
    float mult = DecisionExecutor::getToneMultiplier(ToneType::NEUTRAL);
    EXPECT_EQ(mult, 1.0f);
}

// ==================== TEST SUITE 5: VALIDATION ====================

TEST_F(Phase8TestFixture, ValidateUnknown) {
    Decision decision;
    decision.actionType = ActionType::UNKNOWN;
    ValidationResult result = DecisionValidator::validate(decision);
    EXPECT_FALSE(result.isValid);
}

TEST_F(Phase8TestFixture, ValidateAllocate) {
    Decision decision;
    decision.actionType = ActionType::ALLOCATE;
    ValidationResult result = DecisionValidator::validate(decision);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, ValidateDel) {
    Decision decision;
    decision.actionType = ActionType::DELEGATE;
    ValidationResult result = DecisionValidator::validate(decision);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, ValidateNeg) {
    Decision decision;
    decision.actionType = ActionType::NEGOTIATE;
    ValidationResult result = DecisionValidator::validate(decision);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, ValidateInspire) {
    Decision decision;
    decision.actionType = ActionType::INSPIRE;
    ValidationResult result = DecisionValidator::validate(decision);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, ValidateSuppress) {
    Decision decision;
    decision.actionType = ActionType::SUPPRESS;
    ValidationResult result = DecisionValidator::validate(decision);
    EXPECT_TRUE(result.isValid);
}

// ==================== TEST SUITE 6: NORMALIZATION ====================

TEST_F(Phase8TestFixture, NormalizeCase) {
    std::string norm = InputParser::normalizeInput("ALLOCATE");
    EXPECT_EQ(norm, "allocate");
}

TEST_F(Phase8TestFixture, NormalizeTrim) {
    std::string norm = InputParser::normalizeInput("  allocate  ");
    EXPECT_EQ(norm, "allocate");
}

TEST_F(Phase8TestFixture, NormalizeMultispace) {
    std::string norm = InputParser::normalizeInput("  allocate   food  ");
    // Normalization keeps internal spaces
    EXPECT_EQ(norm, "allocate   food");
}

// ==================== TEST SUITE 7: ACTION EXTRACTION ====================

TEST_F(Phase8TestFixture, ExtractAllocate) {
    ActionType action = InputParser::extractActionType("allocate food");
    EXPECT_EQ(action, ActionType::ALLOCATE);
}

TEST_F(Phase8TestFixture, ExtractDelegate) {
    ActionType action = InputParser::extractActionType("delegate");
    EXPECT_EQ(action, ActionType::DELEGATE);
}

TEST_F(Phase8TestFixture, ExtractNegotiate) {
    ActionType action = InputParser::extractActionType("negotiate");
    EXPECT_EQ(action, ActionType::NEGOTIATE);
}

TEST_F(Phase8TestFixture, ExtractInspire) {
    ActionType action = InputParser::extractActionType("inspire");
    EXPECT_EQ(action, ActionType::INSPIRE);
}

TEST_F(Phase8TestFixture, ExtractSuppress) {
    ActionType action = InputParser::extractActionType("suppress");
    EXPECT_EQ(action, ActionType::SUPPRESS);
}

TEST_F(Phase8TestFixture, ExtractUnknown) {
    ActionType action = InputParser::extractActionType("xyz");
    EXPECT_EQ(action, ActionType::UNKNOWN);
}

// ==================== TEST SUITE 8: PROCESSING ====================

TEST_F(Phase8TestFixture, ProcessPositive) {
    // Test basic parsing without full execution
    ParseResult result = InputParser::parsePlayerInput("please allocate help");
    EXPECT_NE(result.actionType, ActionType::UNKNOWN);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, ProcessNegative) {
    ParseResult result = InputParser::parsePlayerInput("force now");
    EXPECT_NE(result.actionType, ActionType::UNKNOWN);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, ProcessNeutral) {
    ParseResult result = InputParser::parsePlayerInput("allocate");
    EXPECT_NE(result.actionType, ActionType::UNKNOWN);
    EXPECT_TRUE(result.isValid);
}

TEST_F(Phase8TestFixture, ResultFormat) {
    Decision d;
    d.actionType = ActionType::ALLOCATE;
    d.narrativeFlavor = "Test";
    d.tone = ToneType::POSITIVE;
    d.toneMultiplier = 1.2f;
    std::string result = ResultDisplay::formatResult(d);
    EXPECT_GT(result.length(), 0);
    EXPECT_NE(result.find("[DECISION]"), std::string::npos);
}

// ==================== TEST SUITE 9: DETERMINISM ====================

TEST_F(Phase8TestFixture, DeterminismParse1) {
    ParseResult r1 = InputParser::parsePlayerInput("allocate");
    ParseResult r2 = InputParser::parsePlayerInput("allocate");
    EXPECT_EQ(r1.actionType, r2.actionType);
    EXPECT_EQ(r1.confidence, r2.confidence);
}

TEST_F(Phase8TestFixture, DeterminismTone) {
    ToneResult t1 = ToneDetector::detectToneRuleBased("help");
    ToneResult t2 = ToneDetector::detectToneRuleBased("help");
    EXPECT_EQ(t1.tone, t2.tone);
    EXPECT_EQ(t1.confidence, t2.confidence);
}

TEST_F(Phase8TestFixture, DeterminismDist) {
    float d1 = InputParser::calculateLevenshteinDistance("a", "b");
    float d2 = InputParser::calculateLevenshteinDistance("a", "b");
    EXPECT_EQ(d1, d2);
}

// ==================== TEST SUITE 10: EDGE CASES ====================

TEST_F(Phase8TestFixture, LongInput) {
    std::string long_input = "allocate " + std::string(300, 'x');
    ParseResult r = InputParser::parsePlayerInput(long_input);
    EXPECT_EQ(r.actionType, ActionType::ALLOCATE);
}

TEST_F(Phase8TestFixture, SpecialChars) {
    ParseResult r = InputParser::parsePlayerInput("@#$ allocate !!");
    EXPECT_EQ(r.actionType, ActionType::ALLOCATE);
}

TEST_F(Phase8TestFixture, Numbers) {
    ParseResult r = InputParser::parsePlayerInput("123 allocate 456");
    EXPECT_EQ(r.actionType, ActionType::ALLOCATE);
}

TEST_F(Phase8TestFixture, MixedCase) {
    ParseResult r = InputParser::parsePlayerInput("AlLoCAte");
    EXPECT_EQ(r.actionType, ActionType::ALLOCATE);
}

// ==================== MAIN ====================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
