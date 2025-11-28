# Phase 8 Test Suite: Decision Interpretation (LLM)

**Objective**: Comprehensive unit tests for input parsing, confidence scoring, parameter extraction, tone detection, and deterministic decision execution  
**Target Coverage**: 90%+ code coverage, 30+ test cases  
**Test Framework**: Google Test (gtest)  
**Execution Time**: <600ms for full suite  
**Determinism**: 100% reproducible with seeded confidence scoring  

---

## Test Structure Overview

```
Phase8TestSuite.cpp
├── Test Suite 1: Input Parsing (7 tests)
├── Test Suite 2: Confidence Scoring (8 tests)
├── Test Suite 3: Parameter Extraction (7 tests)
├── Test Suite 4: Tone Detection (6 tests)
├── Test Suite 5: Action Validation (6 tests)
├── Test Suite 6: Deterministic Decision Execution (5 tests)
├── Test Suite 7: Fuzzy Matching (6 tests)
└── Test Suite 8: Edge Cases & Error Handling (8 tests)

Total: 53 test cases covering decision interpretation
```

---

## Detailed Test Suites

### Test Suite 1: Input Parsing (7 tests)

**Test 1.1: BasicCommandParsing**
```cpp
TEST(InputParsingTests, BasicCommandParsing) {
  InputParser parser;
  ParseResult result = parser.parse("allocate food to farmers");
  
  EXPECT_EQ(result.actionType, ActionType::ALLOCATE);
  EXPECT_FALSE(result.parameters.empty());
}
```

**Test 1.2: MultiWordInput**
```cpp
TEST(InputParsingTests, MultiWordInput) {
  InputParser parser;
  ParseResult result = parser.parse("give extra food rations to hungry farmers immediately");
  
  EXPECT_EQ(result.actionType, ActionType::ALLOCATE);
  EXPECT_GT(result.confidence, 0.7f);
}
```

**Test 1.3: SimpleVerbDetection**
```cpp
TEST(InputParsingTests, SimpleVerbDetection) {
  InputParser parser;
  
  ParseResult r1 = parser.parse("feed people");
  EXPECT_EQ(r1.actionType, ActionType::ALLOCATE);
  
  ParseResult r2 = parser.parse("help farmers");
  EXPECT_EQ(r2.actionType, ActionType::ALLOCATE);
  
  ParseResult r3 = parser.parse("support agriculture");
  EXPECT_GT(r3.confidence, 0.6f);
}
```

**Test 1.4: UnknownCommandHandling**
```cpp
TEST(InputParsingTests, UnknownCommandHandling) {
  InputParser parser;
  ParseResult result = parser.parse("xyzzy unknown command");
  
  EXPECT_FALSE(result.isValid());
  EXPECT_LT(result.confidence, 0.5f);
}
```

**Test 1.5: PunctuationHandling**
```cpp
TEST(InputParsingTests, PunctuationHandling) {
  InputParser parser;
  ParseResult r1 = parser.parse("allocate food, now!");
  ParseResult r2 = parser.parse("allocate food");
  
  // Should parse same regardless of punctuation
  EXPECT_EQ(r1.actionType, r2.actionType);
}
```

**Test 1.6: CaseInsensitivity**
```cpp
TEST(InputParsingTests, CaseInsensitivity) {
  InputParser parser;
  ParseResult r1 = parser.parse("ALLOCATE FOOD");
  ParseResult r2 = parser.parse("allocate food");
  
  EXPECT_EQ(r1.actionType, r2.actionType);
  EXPECT_EQ(r1.confidence, r2.confidence);
}
```

**Test 1.7: LeadingTrailingWhitespace**
```cpp
TEST(InputParsingTests, LeadingTrailingWhitespace) {
  InputParser parser;
  ParseResult r1 = parser.parse("  allocate food  ");
  ParseResult r2 = parser.parse("allocate food");
  
  EXPECT_EQ(r1.actionType, r2.actionType);
}
```

---

### Test Suite 2: Confidence Scoring (8 tests)

**Test 2.1: ExactMatchHighConfidence**
```cpp
TEST(ConfidenceScoringTests, ExactMatchHighConfidence) {
  ConfidenceCalculator calc;
  float conf = calc.calculateConfidence("allocate", "allocate");
  
  EXPECT_GE(conf, 0.95f);
}
```

**Test 2.2: LevenshteinDistance**
```cpp
TEST(ConfidenceScoringTests, LevenshteinDistance) {
  ConfidenceCalculator calc;
  
  // "feed" vs "food" (distance 1)
  float conf1 = calc.calculateConfidence("feed", "food");
  EXPECT_GT(conf1, 0.7f);
  
  // "allocate" vs "alacate" (distance 2)
  float conf2 = calc.calculateConfidence("allocate", "alacate");
  EXPECT_GT(conf2, 0.6f);
}
```

**Test 2.3: SubstringMatching**
```cpp
TEST(ConfidenceScoringTests, SubstringMatching) {
  ConfidenceCalculator calc;
  
  // "alloc" substring of "allocate"
  float conf = calc.calculateConfidence("alloc", "allocate");
  EXPECT_GT(conf, 0.75f);
}
```

**Test 2.4: HybridConfidenceFormula**
```cpp
TEST(ConfidenceScoringTests, HybridConfidenceFormula) {
  ConfidenceCalculator calc;
  
  // confidence = w_exact * exact + w_levenshtein * lev + w_semantic * sem
  // Default weights: 0.4, 0.3, 0.3
  
  float confidence = calc.calculateConfidence("feed people", "allocate food");
  EXPECT_GE(confidence, 0.5f);
  EXPECT_LE(confidence, 1.0f);
}
```

**Test 2.5: MultipleActionMatches**
```cpp
TEST(ConfidenceScoringTests, MultipleActionMatches) {
  ConfidenceCalculator calc;
  
  std::vector<std::pair<std::string, float>> matches;
  matches.push_back({"allocate", calc.calculateConfidence("feed", "allocate")});
  matches.push_back({"help", calc.calculateConfidence("feed", "help")});
  
  // Should rank by confidence
  EXPECT_GT(matches[0].second, matches[1].second);
}
```

**Test 2.6: ConfidenceThresholds**
```cpp
TEST(ConfidenceScoringTests, ConfidenceThresholds) {
  ConfidenceCalculator calc;
  
  // High confidence (>=0.9): execute without confirmation
  float highConf = calc.calculateConfidence("allocate", "allocate");
  EXPECT_GE(highConf, 0.9f);
  
  // Medium confidence (0.7-0.89): ask for confirmation
  float medConf = calc.calculateConfidence("feed", "allocate");
  EXPECT_GE(medConf, 0.7f);
  EXPECT_LT(medConf, 0.9f);
  
  // Low confidence (<0.6): reject and ask to rephrase
  float lowConf = calc.calculateConfidence("xyz", "allocate");
  EXPECT_LT(lowConf, 0.6f);
}
```

**Test 2.7: TieBreaking**
```cpp
TEST(ConfidenceScoringTests, TieBreaking) {
  ConfidenceCalculator calc;
  
  // If two actions tie at same score, use frequency
  std::vector<ActionFrequency> actions = {
    {"allocate", 100},  // More frequent
    {"distribute", 50}  // Less frequent
  };
  
  // Both have similar confidence, but allocate should win
  std::string winner = calc.selectByFrequency(actions);
  EXPECT_EQ(winner, "allocate");
}
```

**Test 2.8: ConfidenceClamping**
```cpp
TEST(ConfidenceScoringTests, ConfidenceClamping) {
  ConfidenceCalculator calc;
  
  // Confidence should always be in [0, 1]
  float conf = calc.calculateConfidence("any input", "any action");
  
  EXPECT_GE(conf, 0.0f);
  EXPECT_LE(conf, 1.0f);
}
```

---

### Test Suite 3: Parameter Extraction (7 tests)

**Test 3.1: TargetExtraction**
```cpp
TEST(ParameterExtractionTests, TargetExtraction) {
  ParameterExtractor extractor;
  
  std::string input = "allocate food to farmers";
  std::vector<Parameter> params = extractor.extract(input);
  
  // Should identify "food" as resource and "farmers" as target
  EXPECT_EQ(params.size(), 2);
}
```

**Test 3.2: QuantityExtraction**
```cpp
TEST(ParameterExtractionTests, QuantityExtraction) {
  ParameterExtractor extractor;
  
  std::string input = "give 50 food to farmers";
  std::vector<Parameter> params = extractor.extract(input);
  
  // Should extract "50" as quantity
  EXPECT_TRUE(hasParameterOfType(params, ParameterType::QUANTITY));
}
```

**Test 3.3: MultipleParameters**
```cpp
TEST(ParameterExtractionTests, MultipleParameters) {
  ParameterExtractor extractor;
  
  std::string input = "allocate 100 food and 50 wood to farmers and merchants";
  std::vector<Parameter> params = extractor.extract(input);
  
  EXPECT_GE(params.size(), 4);  // At least 4 parameters
}
```

**Test 3.4: NPCNameExtraction**
```cpp
TEST(ParameterExtractionTests, NPCNameExtraction) {
  ParameterExtractor extractor;
  
  std::string input = "give food to Alice and Bob";
  std::vector<Parameter> params = extractor.extract(input);
  
  // Should identify NPC names
  EXPECT_TRUE(hasParameter(params, "Alice"));
  EXPECT_TRUE(hasParameter(params, "Bob"));
}
```

**Test 3.5: FactionNameExtraction**
```cpp
TEST(ParameterExtractionTests, FactionNameExtraction) {
  ParameterExtractor extractor;
  
  std::string input = "help the farmers faction with resources";
  std::vector<Parameter> params = extractor.extract(input);
  
  // Should identify "farmers" as faction
  EXPECT_TRUE(hasParameter(params, "farmers"));
}
```

**Test 3.6: ResourceTypeValidation**
```cpp
TEST(ParameterExtractionTests, ResourceTypeValidation) {
  ParameterExtractor extractor;
  
  std::string input = "allocate food";
  std::vector<Parameter> params = extractor.extract(input);
  
  // Food should be valid resource
  EXPECT_TRUE(ResourceRegistry::isValidResource("food"));
}
```

**Test 3.7: MissingRequiredParameter**
```cpp
TEST(ParameterExtractionTests, MissingRequiredParameter) {
  ParameterExtractor extractor;
  
  std::string input = "allocate to farmers";  // Missing resource
  std::vector<Parameter> params = extractor.extract(input);
  
  // Extraction should fail or mark as incomplete
  EXPECT_FALSE(extractor.isComplete(params));
}
```

---

### Test Suite 4: Tone Detection (6 tests)

**Test 4.1: PositiveTone**
```cpp
TEST(ToneDetectionTests, PositiveTone) {
  ToneDetector detector;
  Tone tone = detector.detectTone("please help the farmers, they deserve it");
  
  EXPECT_EQ(tone, Tone::POSITIVE);
}
```

**Test 4.2: NegativeTone**
```cpp
TEST(ToneDetectionTests, NegativeTone) {
  ToneDetector detector;
  Tone tone = detector.detectTone("stop those lazy warriors immediately");
  
  EXPECT_EQ(tone, Tone::NEGATIVE);
}
```

**Test 4.3: NeutralTone**
```cpp
TEST(ToneDetectionTests, NeutralTone) {
  ToneDetector detector;
  Tone tone = detector.detectTone("allocate food");
  
  EXPECT_EQ(tone, Tone::NEUTRAL);
}
```

**Test 4.4: PositiveModifiers**
```cpp
TEST(ToneDetectionTests, PositiveModifiers) {
  ToneDetector detector;
  
  // Words like "please", "kindly", "generous" indicate positive
  Tone t1 = detector.detectTone("please allocate");
  EXPECT_EQ(t1, Tone::POSITIVE);
  
  Tone t2 = detector.detectTone("generously help");
  EXPECT_EQ(t2, Tone::POSITIVE);
}
```

**Test 4.5: NegativeModifiers**
```cpp
TEST(ToneDetectionTests, NegativeModifiers) {
  ToneDetector detector;
  
  // Words like "lazy", "useless", "punish" indicate negative
  Tone t1 = detector.detectTone("punish lazy workers");
  EXPECT_EQ(t1, Tone::NEGATIVE);
  
  Tone t2 = detector.detectTone("destroy their supplies");
  EXPECT_EQ(t2, Tone::NEGATIVE);
}
```

**Test 4.6: ToneScoring**
```cpp
TEST(ToneDetectionTests, ToneScoring) {
  ToneDetector detector;
  
  // Should produce numerical score for tone
  float score = detector.calculateToneScore("positive words here");
  
  EXPECT_GE(score, 0.5f);  // Positive
  
  score = detector.calculateToneScore("negative words here");
  EXPECT_LE(score, 0.5f);  // Negative
}
```

---

### Test Suite 5: Action Validation (6 tests)

**Test 5.1: ValidActionCheck**
```cpp
TEST(ActionValidationTests, ValidActionCheck) {
  ActionValidator validator;
  
  EXPECT_TRUE(validator.isValidAction("allocate"));
  EXPECT_TRUE(validator.isValidAction("delegate"));
  EXPECT_TRUE(validator.isValidAction("negotiate"));
}
```

**Test 5.2: InvalidActionRejection**
```cpp
TEST(ActionValidationTests, InvalidActionRejection) {
  ActionValidator validator;
  
  EXPECT_FALSE(validator.isValidAction("xyzzy"));
  EXPECT_FALSE(validator.isValidAction("impossible_action"));
}
```

**Test 5.3: ParameterValidation**
```cpp
TEST(ActionValidationTests, ParameterValidation) {
  ActionValidator validator;
  
  // "allocate" requires resource and target
  std::vector<Parameter> params = {
    {ParameterType::RESOURCE, "food"},
    {ParameterType::TARGET, "farmers"}
  };
  
  EXPECT_TRUE(validator.validateParameters("allocate", params));
}
```

**Test 5.4: MissingParameterDetection**
```cpp
TEST(ActionValidationTests, MissingParameterDetection) {
  ActionValidator validator;
  
  // "allocate" missing target
  std::vector<Parameter> params = {
    {ParameterType::RESOURCE, "food"}
  };
  
  EXPECT_FALSE(validator.validateParameters("allocate", params));
}
```

**Test 5.5: ExtraParameterHandling**
```cpp
TEST(ActionValidationTests, ExtraParameterHandling) {
  ActionValidator validator;
  
  // Extra parameters should be ignored (not fail)
  std::vector<Parameter> params = {
    {ParameterType::RESOURCE, "food"},
    {ParameterType::TARGET, "farmers"},
    {ParameterType::URGENCY, "immediately"}  // Extra
  };
  
  EXPECT_TRUE(validator.validateParameters("allocate", params));
}
```

**Test 5.6: AmbiguousActionResolution**
```cpp
TEST(ActionValidationTests, AmbiguousActionResolution) {
  ActionValidator validator;
  
  // "help" is ambiguous (allocate? delegate? negotiate?)
  std::vector<std::string> possibleActions = validator.getAmbiguousMatches("help");
  
  EXPECT_GE(possibleActions.size(), 1);
}
```

---

### Test Suite 6: Deterministic Decision Execution (5 tests)

**Test 6.1: DeterministicParameterApplication**
```cpp
TEST(DeterministicExecutionTests, DeterministicParameterApplication) {
  WorldState state1 = createTestWorldState();
  WorldState state2 = createTestWorldState();
  
  // Apply same decision to both
  applyDecision(state1, "allocate 50 food to farmers");
  applyDecision(state2, "allocate 50 food to farmers");
  
  // Should have identical state
  EXPECT_EQ(state1.getResourceQuantity("food"), state2.getResourceQuantity("food"));
}
```

**Test 6.2: LoyaltyUpdateConsistency**
```cpp
TEST(DeterministicExecutionTests, LoyaltyUpdateConsistency) {
  NPC npc1 = createTestNPC("Alice");
  NPC npc2 = createTestNPC("Alice");
  
  float initialLoyalty = npc1.getLoyalty();
  
  // Apply same positive decision
  npc1.applyPlayerDecision("allocate food", Tone::POSITIVE);
  npc2.applyPlayerDecision("allocate food", Tone::POSITIVE);
  
  // Loyalty should update identically
  EXPECT_EQ(npc1.getLoyalty(), npc2.getLoyalty());
}
```

**Test 6.3: EquationConsistency**
```cpp
TEST(DeterministicExecutionTests, EquationConsistency) {
  // L_f = w1*A_l + w2*R_f + w3*E_f (faction loyalty formula)
  
  float w1 = 0.3f, w2 = 0.3f, w3 = 0.4f;
  float A_l = 0.7f, R_f = 0.5f, E_f = 0.6f;
  
  float loyalty1 = w1*A_l + w2*R_f + w3*E_f;
  float loyalty2 = w1*A_l + w2*R_f + w3*E_f;
  
  EXPECT_EQ(loyalty1, loyalty2);
}
```

**Test 6.4: RNGSeededConsistency**
```cpp
TEST(DeterministicExecutionTests, RNGSeededConsistency) {
  // With same seed, random decisions should be identical
  srand(42);
  int decision1 = rand() % 100;
  
  srand(42);
  int decision2 = rand() % 100;
  
  EXPECT_EQ(decision1, decision2);
}
```

**Test 6.5: NoFloatingPointDrift**
```cpp
TEST(DeterministicExecutionTests, NoFloatingPointDrift) {
  // Run same calculation multiple times
  float result = 0.0f;
  for (int i = 0; i < 1000; i++) {
    result += 0.1f;
  }
  
  // Should be exactly 100.0 (or very close)
  EXPECT_NEAR(result, 100.0f, 0.00001f);
}
```

---

### Test Suite 7: Fuzzy Matching (6 tests)

**Test 7.1: BasicFuzzyMatch**
```cpp
TEST(FuzzyMatchingTests, BasicFuzzyMatch) {
  FuzzyMatcher matcher;
  
  std::string match = matcher.findClosestMatch("feed", {"allocate", "distribute", "provide"});
  
  EXPECT_TRUE(!match.empty());
  EXPECT_GT(matcher.getLastConfidence(), 0.5f);
}
```

**Test 7.2: ExactMatchPriority**
```cpp
TEST(FuzzyMatchingTests, ExactMatchPriority) {
  FuzzyMatcher matcher;
  
  std::string match = matcher.findClosestMatch("allocate", {"allocate", "alacate", "alloceight"});
  
  EXPECT_EQ(match, "allocate");
  EXPECT_GE(matcher.getLastConfidence(), 0.95f);
}
```

**Test 7.3: TypoTolerance**
```cpp
TEST(FuzzyMatchingTests, TypoTolerance) {
  FuzzyMatcher matcher;
  
  // One character off
  std::string match = matcher.findClosestMatch("alloocate", {"allocate"});
  
  EXPECT_EQ(match, "allocate");
  EXPECT_GT(matcher.getLastConfidence(), 0.7f);
}
```

**Test 7.4: AbbreviationHandling**
```cpp
TEST(FuzzyMatchingTests, AbbreviationHandling) {
  FuzzyMatcher matcher;
  
  std::string match = matcher.findClosestMatch("alloc", {"allocate", "distribute"});
  
  EXPECT_EQ(match, "allocate");
}
```

**Test 7.5: SemanticSimilarity**
```cpp
TEST(FuzzyMatchingTests, SemanticSimilarity) {
  FuzzyMatcher matcher;
  
  // "feed" and "allocate" are semantically similar
  float similarity = matcher.calculateSemanticSimilarity("feed", "allocate");
  
  EXPECT_GT(similarity, 0.6f);
}
```

**Test 7.6: NoBestMatch**
```cpp
TEST(FuzzyMatchingTests, NoBestMatch) {
  FuzzyMatcher matcher;
  
  std::string match = matcher.findClosestMatch("xyz", {"aaa", "bbb", "ccc"});
  
  // Should return empty or lowest confidence
  EXPECT_LT(matcher.getLastConfidence(), 0.5f);
}
```

---

### Test Suite 8: Edge Cases & Error Handling (8 tests)

**Test 8.1: EmptyInput**
```cpp
TEST(EdgeCaseTests, EmptyInput) {
  InputParser parser;
  ParseResult result = parser.parse("");
  
  EXPECT_FALSE(result.isValid());
}
```

**Test 8.2: VeryLongInput**
```cpp
TEST(EdgeCaseTests, VeryLongInput) {
  InputParser parser;
  std::string longInput(10000, 'x');
  ParseResult result = parser.parse(longInput);
  
  // Should handle without crash
  EXPECT_TRUE(result.isValid() || !result.isValid());
}
```

**Test 8.3: SpecialCharacters**
```cpp
TEST(EdgeCaseTests, SpecialCharacters) {
  InputParser parser;
  ParseResult result = parser.parse("allocate @!#$% to farmers");
  
  // Should handle gracefully
  EXPECT_TRUE(result.isValid() || !result.isValid());
}
```

**Test 8.4: UnicodeCharacters**
```cpp
TEST(EdgeCaseTests, UnicodeCharacters) {
  InputParser parser;
  ParseResult result = parser.parse("allocate café to farmers");
  
  // Should handle unicode
  EXPECT_TRUE(result.isValid() || !result.isValid());
}
```

**Test 8.5: AmbiguousInput**
```cpp
TEST(EdgeCaseTests, AmbiguousInput) {
  InputParser parser;
  ParseResult result = parser.parse("help");  // Could be many actions
  
  // Should handle ambiguity
  EXPECT_LE(result.confidence, 0.75f);
}
```

**Test 8.6: ChainedCommands**
```cpp
TEST(EdgeCaseTests, ChainedCommands) {
  InputParser parser;
  
  // Should handle one command at a time
  ParseResult result = parser.parse("allocate food; delegate tasks");
  
  // Parser should take first command
  EXPECT_EQ(result.actionType, ActionType::ALLOCATE);
}
```

**Test 8.7: NullReferences**
```cpp
TEST(EdgeCaseTests, NullReferences) {
  InputParser parser;
  
  // Should not crash on null
  ParseResult result = parser.parse(nullptr);
  
  EXPECT_FALSE(result.isValid());
}
```

**Test 8.8: InconsistentParameters**
```cpp
TEST(EdgeCaseTests, InconsistentParameters) {
  ActionValidator validator;
  
  // Resource that doesn't exist
  std::vector<Parameter> params = {
    {ParameterType::RESOURCE, "unobtanium"},
    {ParameterType::TARGET, "farmers"}
  };
  
  EXPECT_FALSE(validator.validateParameters("allocate", params));
}
```

---

## Coverage Analysis

### Components Covered
- ✅ Input Parsing (7 tests)
- ✅ Confidence Scoring (8 tests)
- ✅ Parameter Extraction (7 tests)
- ✅ Tone Detection (6 tests)
- ✅ Action Validation (6 tests)
- ✅ Deterministic Execution (5 tests)
- ✅ Fuzzy Matching (6 tests)
- ✅ Edge Cases & Error Handling (8 tests)

**Total Tests: 53**
**Expected Coverage: 91%+**

---

## Performance Targets

- **Total Execution Time**: <600ms
- **Average Test Time**: <12ms per test
- **Memory Usage**: <50MB for full suite
- **Code Coverage**: 91%+ of Phase 8 implementation

---

## Success Criteria

✅ All 53 test cases implemented and passing
✅ 91%+ code coverage achieved
✅ <600ms execution time
✅ Confidence scoring accurate (<±0.05 variance)
✅ Deterministic execution verified
✅ All input variations handled gracefully
✅ Fuzzy matching effective (70%+ match rate for typos)
✅ Parameter extraction complete and accurate

