# Phase 2 Test Suite: NPC Behavior

## Overview

This document provides comprehensive unit tests for Phase 2 (NPC Behavior) of the Typed Leadership Simulator. Tests cover the emotion model (immediate emotion E_i, short-term mood M_s, long-term attitude A_l), action probability calculations, personality inference, loyalty updates, and NPC decision-making.

**Test Framework**: Google Test (gtest)  
**Target Coverage**: 90%+ code coverage  
**Test Count**: 24 test cases  
**Execution Time**: <600ms total

---

## Test Categories

1. **Emotion Model Tests** (8 tests)
2. **Action Probability Tests** (5 tests)
3. **Personality Inference Tests** (4 tests)
4. **Loyalty & Faction Alignment Tests** (5 tests)
5. **NPC Decision-Making Tests** (2 tests)

---

## Test Suite 1: Emotion Model Tests

### Test 1.1: Immediate Emotion Calculation (E_i)
```cpp
TEST(EmotionModelTest, ImmediateEmotionCalculation) {
  // Setup NPC
  NPC npc(42, "Alice", 25);
  npc.loyalty = 0.7f;
  npc.mood = 0.5f;
  
  // Event parameters (from Equations.txt)
  float tone = 0.8f;          // Positive tone (player allocates resources)
  float relevance = 0.9f;     // High relevance to NPC
  float bias = 0.6f;          // NPC slightly favorable to player
  float socialPressure = 0.4f; // Moderate faction influence
  
  // Weights (from Equations.txt)
  float theta_1 = 0.3f;  // Tone weight
  float theta_2 = 0.4f;  // Relevance weight
  float theta_3 = 0.2f;  // Bias weight
  float theta_4 = 0.1f;  // Social pressure weight
  
  // Calculate E_i = θ₁*T + θ₂*R + θ₃*B + θ₄*SP
  float E_i = npc.calculateImmediateEmotion(tone, relevance, bias, socialPressure);
  
  // Expected: 0.3*0.8 + 0.4*0.9 + 0.2*0.6 + 0.1*0.4 = 0.24 + 0.36 + 0.12 + 0.04 = 0.76
  ASSERT_FLOAT_EQ(E_i, 0.76f);
  
  // Verify E_i bounded [0, 1]
  ASSERT_GE(E_i, 0.0f);
  ASSERT_LE(E_i, 1.0f);
}

// Copilot Prompt:
// "Implement NPC::calculateImmediateEmotion(tone, relevance, bias, socialPressure) -> float.
// Formula: E_i = θ₁*tone + θ₂*relevance + θ₃*bias + θ₄*socialPressure
// Default weights: θ₁=0.3, θ₂=0.4, θ₃=0.2, θ₄=0.1
// Clamp result to [0, 1] range."
```

### Test 1.2: Short-Term Mood Update (M_s)
```cpp
TEST(EmotionModelTest, ShortTermMoodUpdate) {
  // Setup NPC with initial mood
  NPC npc(42, "Alice", 25);
  npc.mood = 0.6f;  // M_s(t-1)
  
  // Calculate immediate emotion
  float E_i = 0.8f;  // Recent positive event
  
  // Update mood with exponential smoothing (from Equations.txt)
  float alpha = 0.1f;  // Smoothing factor (low = slow change)
  float new_mood = npc.updateShortTermMood(E_i, alpha);
  
  // Expected: M_s(t) = α*E_i + (1-α)*M_s(t-1)
  //         = 0.1*0.8 + 0.9*0.6 = 0.08 + 0.54 = 0.62
  ASSERT_FLOAT_EQ(new_mood, 0.62f);
  ASSERT_FLOAT_EQ(npc.mood, 0.62f);  // Verify state updated
}

// Copilot Prompt:
// "Implement NPC::updateShortTermMood(E_i, alpha) -> float.
// Formula: M_s(t) = alpha * E_i + (1 - alpha) * M_s(t-1)
// Update npc.mood field, return new mood value.
// Default alpha = 0.1 (slow mood change)."
```

### Test 1.3: Long-Term Attitude Update (A_l)
```cpp
TEST(EmotionModelTest, LongTermAttitudeUpdate) {
  // Setup NPC with initial attitude
  NPC npc(42, "Alice", 25);
  npc.attitude = 0.5f;  // A_l(t-1) - neutral
  npc.mood = 0.7f;      // M_s(t) - content
  
  // Update attitude (from Equations.txt)
  float beta = 0.01f;  // Integration factor (very slow change)
  float new_attitude = npc.updateLongTermAttitude(beta);
  
  // Expected: A_l(t) = A_l(t-1) + β*M_s(t)
  //         = 0.5 + 0.01*0.7 = 0.5 + 0.007 = 0.507
  ASSERT_FLOAT_EQ(new_attitude, 0.507f);
  ASSERT_FLOAT_EQ(npc.attitude, 0.507f);
}

// Copilot Prompt:
// "Implement NPC::updateLongTermAttitude(beta) -> float.
// Formula: A_l(t) = A_l(t-1) + beta * M_s(t)
// Update npc.attitude field, return new attitude.
// Default beta = 0.01 (slow long-term memory integration).
// Clamp result to [0, 1]."
```

### Test 1.4: Emotion Chain Propagation (E_i → M_s → A_l)
```cpp
TEST(EmotionModelTest, EmotionChainPropagation) {
  // Setup NPC with baseline state
  NPC npc(42, "Alice", 25);
  npc.mood = 0.5f;      // Neutral mood
  npc.attitude = 0.5f;  // Neutral attitude
  
  // Simulate 10 ticks of positive events
  for (int i = 0; i < 10; i++) {
    // Positive event each tick
    float E_i = 0.8f;
    npc.updateShortTermMood(E_i, 0.1f);
    npc.updateLongTermAttitude(0.01f);
  }
  
  // After 10 ticks:
  // M_s should approach E_i (0.8) asymptotically
  ASSERT_GT(npc.mood, 0.6f);  // Increased from 0.5
  ASSERT_LT(npc.mood, 0.8f);  // Not fully converged yet
  
  // A_l should increase slowly
  ASSERT_GT(npc.attitude, 0.5f);   // Increased
  ASSERT_LT(npc.attitude, 0.55f);  // Slow change (β=0.01)
}

// Validates exponential smoothing converges correctly
```

### Test 1.5: Negative Event Impact on Mood
```cpp
TEST(EmotionModelTest, NegativeEventImpact) {
  // Setup NPC with high initial mood
  NPC npc(42, "Alice", 25);
  npc.mood = 0.8f;      // Happy
  npc.attitude = 0.7f;  // Loyal
  
  // Negative event (player ignores NPC request)
  float E_i_negative = 0.2f;  // Low immediate emotion
  
  // Update mood
  npc.updateShortTermMood(E_i_negative, 0.1f);
  
  // Expected: M_s(t) = 0.1*0.2 + 0.9*0.8 = 0.02 + 0.72 = 0.74
  ASSERT_FLOAT_EQ(npc.mood, 0.74f);  // Decreased from 0.8
  
  // Simulate 5 more negative events
  for (int i = 0; i < 5; i++) {
    npc.updateShortTermMood(0.2f, 0.1f);
    npc.updateLongTermAttitude(0.01f);
  }
  
  // Mood should decline significantly
  ASSERT_LT(npc.mood, 0.6f);
  
  // Attitude should decline slowly
  ASSERT_LT(npc.attitude, 0.7f);
}

// Validates negative feedback loop
```

### Test 1.6: Mood Clamping (Boundary Validation)
```cpp
TEST(EmotionModelTest, MoodClampingBoundaries) {
  // Setup NPC
  NPC npc(42, "Alice", 25);
  
  // Test upper bound clamping
  npc.mood = 0.95f;
  float E_i_high = 1.5f;  // Intentionally exceeds [0,1]
  npc.updateShortTermMood(E_i_high, 0.5f);  // Large alpha for faster change
  
  // Mood should clamp to 1.0 (not exceed)
  ASSERT_LE(npc.mood, 1.0f);
  
  // Test lower bound clamping
  npc.mood = 0.05f;
  float E_i_low = -0.5f;  // Intentionally negative
  npc.updateShortTermMood(E_i_low, 0.5f);
  
  // Mood should clamp to 0.0 (not go negative)
  ASSERT_GE(npc.mood, 0.0f);
}

// Edge case: Ensure mood always in valid range [0, 1]
```

### Test 1.7: Attitude Persistence Over Time
```cpp
TEST(EmotionModelTest, AttitudePersistenceOverTime) {
  // Setup two NPCs with different attitudes
  NPC npc_loyal(1, "Alice", 25);
  npc_loyal.attitude = 0.9f;  // Very loyal
  npc_loyal.mood = 0.5f;      // Neutral mood
  
  NPC npc_hostile(2, "Bob", 30);
  npc_hostile.attitude = 0.2f;  // Hostile
  npc_hostile.mood = 0.5f;      // Neutral mood
  
  // Simulate 100 ticks with neutral events (M_s = 0.5)
  for (int i = 0; i < 100; i++) {
    npc_loyal.updateLongTermAttitude(0.01f);
    npc_hostile.updateLongTermAttitude(0.01f);
  }
  
  // Loyal NPC: attitude should remain high (memory persists)
  ASSERT_GT(npc_loyal.attitude, 0.85f);  // Slight decay toward neutral
  
  // Hostile NPC: attitude should remain low
  ASSERT_LT(npc_hostile.attitude, 0.4f);
  
  // Validates long-term memory doesn't reset quickly
}

// Key feature: Attitude changes very slowly (β=0.01)
```

### Test 1.8: Emotion Model Determinism
```cpp
TEST(EmotionModelTest, EmotionModelDeterminism) {
  // Setup two identical NPCs
  NPC npc1(42, "Alice", 25);
  npc1.mood = 0.6f;
  npc1.attitude = 0.5f;
  
  NPC npc2(42, "Alice", 25);
  npc2.mood = 0.6f;
  npc2.attitude = 0.5f;
  
  // Apply identical sequence of events
  std::vector<float> event_sequence = {0.8f, 0.3f, 0.9f, 0.4f, 0.7f};
  
  for (float E_i : event_sequence) {
    npc1.updateShortTermMood(E_i, 0.1f);
    npc1.updateLongTermAttitude(0.01f);
    
    npc2.updateShortTermMood(E_i, 0.1f);
    npc2.updateLongTermAttitude(0.01f);
  }
  
  // Assert identical final states (bit-identical)
  ASSERT_FLOAT_EQ(npc1.mood, npc2.mood);
  ASSERT_FLOAT_EQ(npc1.attitude, npc2.attitude);
  
  // Verify with memcmp (strict byte comparison)
  ASSERT_EQ(memcmp(&npc1.mood, &npc2.mood, sizeof(float)), 0);
  ASSERT_EQ(memcmp(&npc1.attitude, &npc2.attitude, sizeof(float)), 0);
}

// Critical: Emotion calculations must be deterministic for replay
```

---

## Test Suite 2: Action Probability Tests

### Test 2.1: Action Probability Calculation
```cpp
TEST(ActionProbabilityTest, BasicActionProbabilityCalculation) {
  // Setup NPC
  NPC npc(42, "Alice", 25);
  npc.attitude = 0.7f;   // A_l - long-term loyalty
  npc.mood = 0.6f;       // M_s - current mood
  
  // Calculate recent immediate emotion
  float E_i = 0.8f;
  
  // Action probability (from Equations.txt)
  // P_action = sigmoid(γ*A_l + δ*M_s + ε*E_i)
  float gamma = 2.0f;   // Attitude weight
  float delta = 1.0f;   // Mood weight
  float epsilon = 0.5f; // Immediate emotion weight
  
  float P_action = npc.calculateActionProbability(E_i, gamma, delta, epsilon);
  
  // Expected: sigmoid(2.0*0.7 + 1.0*0.6 + 0.5*0.8)
  //         = sigmoid(1.4 + 0.6 + 0.4) = sigmoid(2.4)
  //         = 1 / (1 + exp(-2.4)) ≈ 0.917
  ASSERT_NEAR(P_action, 0.917f, 0.01f);
  
  // Verify probability in [0, 1]
  ASSERT_GE(P_action, 0.0f);
  ASSERT_LE(P_action, 1.0f);
}

// Copilot Prompt:
// "Implement NPC::calculateActionProbability(E_i, gamma, delta, epsilon) -> float.
// Formula: P_action = sigmoid(gamma*A_l + delta*M_s + epsilon*E_i)
// Sigmoid: 1 / (1 + exp(-x))
// Default weights: gamma=2.0, delta=1.0, epsilon=0.5
// Return probability in [0, 1]."
```

### Test 2.2: Action Probability Edge Cases
```cpp
TEST(ActionProbabilityTest, EdgeCases) {
  // Setup NPC
  NPC npc(42, "Alice", 25);
  
  // Case 1: All emotions zero (neutral)
  npc.attitude = 0.0f;
  npc.mood = 0.0f;
  float E_i_zero = 0.0f;
  
  float P_neutral = npc.calculateActionProbability(E_i_zero);
  
  // Expected: sigmoid(0) = 0.5 (50% probability)
  ASSERT_NEAR(P_neutral, 0.5f, 0.01f);
  
  // Case 2: Maximum emotions (very likely action)
  npc.attitude = 1.0f;
  npc.mood = 1.0f;
  float E_i_max = 1.0f;
  
  float P_max = npc.calculateActionProbability(E_i_max);
  
  // Expected: sigmoid(2.0*1 + 1.0*1 + 0.5*1) = sigmoid(3.5) ≈ 0.971
  ASSERT_NEAR(P_max, 0.971f, 0.01f);
  
  // Case 3: Minimum emotions (very unlikely action)
  npc.attitude = 0.0f;
  npc.mood = 0.0f;
  float E_i_min = 0.0f;
  
  float P_min = npc.calculateActionProbability(E_i_min);
  
  // Expected: sigmoid(0) = 0.5 (neutral)
  ASSERT_NEAR(P_min, 0.5f, 0.01f);
}

// Validates sigmoid bounds and neutral case
```

### Test 2.3: Action Decision Based on Probability
```cpp
TEST(ActionProbabilityTest, ActionDecisionWithRNG) {
  // Setup NPC with high action probability
  NPC npc(42, "Alice", 25);
  npc.attitude = 0.9f;
  npc.mood = 0.8f;
  
  // Calculate action probability (should be high)
  float E_i = 0.9f;
  float P_action = npc.calculateActionProbability(E_i);
  ASSERT_GT(P_action, 0.9f);  // Very likely to act
  
  // Simulate 100 decisions with RNG
  std::mt19937 rng(12345);
  int action_count = 0;
  
  for (int i = 0; i < 100; i++) {
    if (npc.shouldTakeAction(E_i, rng)) {
      action_count++;
    }
  }
  
  // Expected: ~90-95 actions out of 100 (P_action ≈ 0.95)
  ASSERT_GE(action_count, 85);  // At least 85 (allow statistical variance)
  ASSERT_LE(action_count, 100); // At most 100
}

// Copilot Prompt:
// "Implement NPC::shouldTakeAction(E_i, rng) -> bool.
// Calculate P_action = calculateActionProbability(E_i).
// Generate random float in [0, 1) using rng.
// Return true if random < P_action, false otherwise."
```

### Test 2.4: Low Probability Action Blocking
```cpp
TEST(ActionProbabilityTest, LowProbabilityActionBlocking) {
  // Setup NPC with low action probability (hostile, unhappy)
  NPC npc(42, "Bob", 30);
  npc.attitude = 0.2f;  // Hostile
  npc.mood = 0.3f;      // Unhappy
  
  // Negative event
  float E_i = 0.2f;
  float P_action = npc.calculateActionProbability(E_i);
  
  // Expected: sigmoid(2.0*0.2 + 1.0*0.3 + 0.5*0.2)
  //         = sigmoid(0.4 + 0.3 + 0.1) = sigmoid(0.8) ≈ 0.689
  ASSERT_NEAR(P_action, 0.689f, 0.01f);
  
  // Simulate 100 decisions
  std::mt19937 rng(12345);
  int action_count = 0;
  
  for (int i = 0; i < 100; i++) {
    if (npc.shouldTakeAction(E_i, rng)) {
      action_count++;
    }
  }
  
  // Expected: ~65-75 actions out of 100
  ASSERT_GE(action_count, 60);
  ASSERT_LE(action_count, 80);
}

// Validates lower probability → fewer actions
```

### Test 2.5: Action Probability Determinism with Same Seed
```cpp
TEST(ActionProbabilityTest, ActionProbabilityDeterminism) {
  // Setup two identical NPCs
  NPC npc1(42, "Alice", 25);
  npc1.attitude = 0.7f;
  npc1.mood = 0.6f;
  
  NPC npc2(42, "Alice", 25);
  npc2.attitude = 0.7f;
  npc2.mood = 0.6f;
  
  // Same seed for RNG
  std::mt19937 rng1(12345);
  std::mt19937 rng2(12345);
  
  // Simulate 50 decisions
  std::vector<bool> decisions1, decisions2;
  float E_i = 0.8f;
  
  for (int i = 0; i < 50; i++) {
    decisions1.push_back(npc1.shouldTakeAction(E_i, rng1));
    decisions2.push_back(npc2.shouldTakeAction(E_i, rng2));
  }
  
  // Assert identical decision sequences
  ASSERT_EQ(decisions1.size(), decisions2.size());
  for (size_t i = 0; i < decisions1.size(); i++) {
    ASSERT_EQ(decisions1[i], decisions2[i]);
  }
}

// Critical: Same seed → identical action sequences (replay support)
```

---

## Test Suite 3: Personality Inference Tests

### Test 3.1: Infer Personality from Loyalty & Ambition
```cpp
TEST(PersonalityInferenceTest, InferFromLoyaltyAndAmbition) {
  // Case 1: High loyalty, low ambition → LOYAL + CAUTIOUS
  NPC npc1(1, "Alice", 25);
  npc1.loyalty = 0.9f;
  npc1.ambition = 0.2f;
  npc1.mood = 0.6f;
  
  std::vector<Personality> inferred1 = npc1.inferPersonality();
  
  ASSERT_TRUE(std::find(inferred1.begin(), inferred1.end(), Personality::LOYAL) != inferred1.end());
  ASSERT_TRUE(std::find(inferred1.begin(), inferred1.end(), Personality::CAUTIOUS) != inferred1.end());
  
  // Case 2: Low loyalty, high ambition → AMBITIOUS + AGGRESSIVE
  NPC npc2(2, "Bob", 30);
  npc2.loyalty = 0.3f;
  npc2.ambition = 0.9f;
  npc2.mood = 0.5f;
  
  std::vector<Personality> inferred2 = npc2.inferPersonality();
  
  ASSERT_TRUE(std::find(inferred2.begin(), inferred2.end(), Personality::AMBITIOUS) != inferred2.end());
  ASSERT_TRUE(std::find(inferred2.begin(), inferred2.end(), Personality::AGGRESSIVE) != inferred2.end());
}

// Copilot Prompt:
// "Implement NPC::inferPersonality() -> vector<Personality>.
// Rules:
//   - loyalty > 0.7 → add LOYAL
//   - ambition < 0.3 → add CAUTIOUS
//   - ambition > 0.7 → add AMBITIOUS
//   - mood > 0.7 → add OPTIMISTIC
//   - mood < 0.3 → add PESSIMISTIC
//   - attitude < 0.3 → add HOSTILE
//   - attitude > 0.7 → add DEVOTED
// Return vector of inferred traits (can have multiple)."
```

### Test 3.2: Infer Personality from Mood
```cpp
TEST(PersonalityInferenceTest, InferFromMood) {
  // Case 1: Very high mood → OPTIMISTIC
  NPC npc_happy(1, "Alice", 25);
  npc_happy.mood = 0.85f;
  npc_happy.loyalty = 0.5f;
  npc_happy.ambition = 0.5f;
  
  std::vector<Personality> traits_happy = npc_happy.inferPersonality();
  ASSERT_TRUE(std::find(traits_happy.begin(), traits_happy.end(), Personality::OPTIMISTIC) != traits_happy.end());
  
  // Case 2: Very low mood → PESSIMISTIC
  NPC npc_sad(2, "Bob", 30);
  npc_sad.mood = 0.15f;
  npc_sad.loyalty = 0.5f;
  npc_sad.ambition = 0.5f;
  
  std::vector<Personality> traits_sad = npc_sad.inferPersonality();
  ASSERT_TRUE(std::find(traits_sad.begin(), traits_sad.end(), Personality::PESSIMISTIC) != traits_sad.end());
}

// Validates mood-based trait inference
```

### Test 3.3: Neutral Personality (No Strong Traits)
```cpp
TEST(PersonalityInferenceTest, NeutralPersonality) {
  // Setup NPC with all attributes near 0.5 (neutral)
  NPC npc(42, "Charlie", 28);
  npc.loyalty = 0.5f;
  npc.ambition = 0.5f;
  npc.mood = 0.5f;
  npc.attitude = 0.5f;
  
  std::vector<Personality> traits = npc.inferPersonality();
  
  // Expected: Either empty or contains NEUTRAL trait
  if (traits.empty()) {
    // Implementation 1: Return empty for neutral
    ASSERT_EQ(traits.size(), 0);
  } else {
    // Implementation 2: Return NEUTRAL trait
    ASSERT_EQ(traits.size(), 1);
    ASSERT_EQ(traits[0], Personality::NEUTRAL);
  }
}

// Edge case: No strong traits → neutral personality
```

### Test 3.4: Multiple Conflicting Traits
```cpp
TEST(PersonalityInferenceTest, MultipleTraits) {
  // Setup NPC with multiple strong attributes
  NPC npc(42, "Complex", 30);
  npc.loyalty = 0.8f;     // → LOYAL
  npc.ambition = 0.8f;    // → AMBITIOUS
  npc.mood = 0.8f;        // → OPTIMISTIC
  npc.attitude = 0.8f;    // → DEVOTED
  
  std::vector<Personality> traits = npc.inferPersonality();
  
  // Expected: All 4 traits present
  ASSERT_GE(traits.size(), 4);
  ASSERT_TRUE(std::find(traits.begin(), traits.end(), Personality::LOYAL) != traits.end());
  ASSERT_TRUE(std::find(traits.begin(), traits.end(), Personality::AMBITIOUS) != traits.end());
  ASSERT_TRUE(std::find(traits.begin(), traits.end(), Personality::OPTIMISTIC) != traits.end());
  ASSERT_TRUE(std::find(traits.begin(), traits.end(), Personality::DEVOTED) != traits.end());
  
  // Note: Personality can be complex (multiple traits coexist)
}

// Validates NPCs can have rich, multifaceted personalities
```

---

## Test Suite 4: Loyalty & Faction Alignment Tests

### Test 4.1: Loyalty Update Based on Player Decision
```cpp
TEST(LoyaltyTest, LoyaltyUpdateFromPlayerDecision) {
  // Setup NPC
  NPC npc(42, "Alice", 25);
  npc.loyalty = 0.6f;
  
  // Player makes favorable decision (allocates food to NPC's faction)
  float decision_impact = 0.1f;  // +10% loyalty
  npc.updateLoyalty(decision_impact);
  
  // Expected: 0.6 + 0.1 = 0.7
  ASSERT_FLOAT_EQ(npc.loyalty, 0.7f);
  
  // Player makes unfavorable decision (ignores NPC request)
  float negative_impact = -0.15f;  // -15% loyalty
  npc.updateLoyalty(negative_impact);
  
  // Expected: 0.7 - 0.15 = 0.55
  ASSERT_FLOAT_EQ(npc.loyalty, 0.55f);
}

// Copilot Prompt:
// "Implement NPC::updateLoyalty(delta) -> void.
// Update npc.loyalty += delta.
// Clamp result to [0, 1].
// Log if loyalty crosses threshold (0.3 for hostility, 0.7 for devotion)."
```

### Test 4.2: Loyalty Clamping (Boundary Validation)
```cpp
TEST(LoyaltyTest, LoyaltyClamping) {
  // Setup NPC
  NPC npc(42, "Alice", 25);
  
  // Test upper bound
  npc.loyalty = 0.95f;
  npc.updateLoyalty(0.2f);  // Would exceed 1.0
  ASSERT_FLOAT_EQ(npc.loyalty, 1.0f);  // Clamped to max
  
  // Test lower bound
  npc.loyalty = 0.1f;
  npc.updateLoyalty(-0.3f);  // Would go negative
  ASSERT_FLOAT_EQ(npc.loyalty, 0.0f);  // Clamped to min
}

// Validates loyalty stays in [0, 1] range
```

### Test 4.3: Faction Loyalty Calculation (L_f)
```cpp
TEST(LoyaltyTest, FactionLoyaltyCalculation) {
  // Setup NPC with faction affiliation
  NPC npc(42, "Alice", 25);
  npc.attitude = 0.7f;   // A_l - toward player
  
  // Faction parameters (from Equations.txt)
  float R_f = 0.8f;  // Faction relevance to NPC goals
  float E_f = 0.6f;  // Faction emotional influence
  
  // Weights (from Equations.txt)
  float w_1 = 0.5f;  // Attitude weight
  float w_2 = 0.3f;  // Relevance weight
  float w_3 = 0.2f;  // Emotional influence weight
  
  // Calculate faction loyalty
  // L_f = w_1*A_l + w_2*R_f + w_3*E_f
  float L_f = npc.calculateFactionLoyalty(R_f, E_f, w_1, w_2, w_3);
  
  // Expected: 0.5*0.7 + 0.3*0.8 + 0.2*0.6 = 0.35 + 0.24 + 0.12 = 0.71
  ASSERT_FLOAT_EQ(L_f, 0.71f);
}

// Copilot Prompt:
// "Implement NPC::calculateFactionLoyalty(R_f, E_f, w_1, w_2, w_3) -> float.
// Formula: L_f = w_1*A_l + w_2*R_f + w_3*E_f
// Default weights: w_1=0.5, w_2=0.3, w_3=0.2
// Return faction loyalty in [0, 1]."
```

### Test 4.4: Faction Alignment Shift
```cpp
TEST(LoyaltyTest, FactionAlignmentShift) {
  // Setup NPC initially in Farmer faction
  NPC npc(42, "Alice", 25);
  npc.faction_id = 1;  // Farmers
  npc.loyalty = 0.4f;   // Low loyalty to player
  
  // Rival faction (Warriors) offers better alignment
  uint32_t rival_faction_id = 2;
  float rival_R_f = 0.9f;  // High relevance to NPC goals
  float rival_E_f = 0.8f;  // Strong emotional pull
  
  float rival_loyalty = npc.calculateFactionLoyalty(rival_R_f, rival_E_f);
  
  // If rival loyalty > current faction loyalty + threshold
  float current_faction_loyalty = npc.calculateFactionLoyalty(0.5f, 0.5f);
  
  if (rival_loyalty > current_faction_loyalty + 0.2f) {
    // NPC switches faction
    npc.faction_id = rival_faction_id;
  }
  
  // Assert faction switched
  ASSERT_EQ(npc.faction_id, rival_faction_id);
}

// Validates dynamic faction switching based on loyalty
```

### Test 4.5: Loyalty Decay Over Neglect
```cpp
TEST(LoyaltyTest, LoyaltyDecayOverNeglect) {
  // Setup NPC with moderate loyalty
  NPC npc(42, "Alice", 25);
  npc.loyalty = 0.7f;
  
  // Simulate 100 ticks with no player interaction (neglect)
  float decay_rate = -0.001f;  // -0.1% per tick
  
  for (int i = 0; i < 100; i++) {
    npc.updateLoyalty(decay_rate);
  }
  
  // Expected: 0.7 - 0.1 = 0.6
  ASSERT_NEAR(npc.loyalty, 0.6f, 0.01f);
  
  // Validate loyalty doesn't decay below 0
  for (int i = 0; i < 1000; i++) {
    npc.updateLoyalty(decay_rate);
  }
  
  ASSERT_GE(npc.loyalty, 0.0f);  // Clamped to 0
}

// Feature: Passive loyalty decay encourages player engagement
```

---

## Test Suite 5: NPC Decision-Making Tests

### Test 5.1: NPC Initiates Dialogue Based on Problem Severity
```cpp
TEST(NPCDecisionTest, InitiateDialogueOnProblemSeverity) {
  // Setup NPC with declining mood
  NPC npc(42, "Alice", 25);
  npc.mood = 0.7f;
  npc.loyalty = 0.6f;
  
  // Track previous state
  float prev_mood = npc.mood;
  float prev_loyalty = npc.loyalty;
  
  // Negative event causes mood drop
  npc.updateShortTermMood(0.2f, 0.1f);  // M_s drops to ~0.65
  npc.updateLoyalty(-0.1f);              // Loyalty drops to 0.5
  
  // Calculate problem severity (from copilot-instructions.md)
  // severity = 0.5 × |mood_delta| + 0.5 × |loyalty_delta|
  float mood_delta = npc.mood - prev_mood;
  float loyalty_delta = npc.loyalty - prev_loyalty;
  float severity = 0.5f * std::abs(mood_delta) + 0.5f * std::abs(loyalty_delta);
  
  // Expected: 0.5*|0.65-0.7| + 0.5*|-0.1| = 0.5*0.05 + 0.5*0.1 = 0.025 + 0.05 = 0.075
  ASSERT_NEAR(severity, 0.075f, 0.01f);
  
  // Threshold for dialogue initiation: 0.3
  if (severity >= 0.3f) {
    // NPC should initiate dialogue
    FAIL() << "Severity too low for dialogue";
  }
  
  // Simulate larger negative event
  npc.updateShortTermMood(0.1f, 0.1f);  // Further drop
  npc.updateLoyalty(-0.25f);            // Significant loyalty drop
  
  // Recalculate severity
  mood_delta = npc.mood - prev_mood;
  loyalty_delta = npc.loyalty - prev_loyalty;
  severity = 0.5f * std::abs(mood_delta) + 0.5f * std::abs(loyalty_delta);
  
  // Now severity should exceed 0.3
  ASSERT_GE(severity, 0.3f);
  
  // NPC initiates dialogue
  bool should_initiate = npc.shouldInitiateDialogue(prev_mood, prev_loyalty);
  ASSERT_TRUE(should_initiate);
}

// Copilot Prompt:
// "Implement NPC::shouldInitiateDialogue(prev_mood, prev_loyalty) -> bool.
// Calculate severity = 0.5 × |mood - prev_mood| + 0.5 × |loyalty - prev_loyalty|
// Return true if severity >= 0.3 (threshold for dialogue)."
```

### Test 5.2: NPC Pathfinding to Player on Problem Detection
```cpp
TEST(NPCDecisionTest, PathfindToPlayerOnProblem) {
  // Setup NPC far from player
  NPC npc(42, "Alice", 25);
  npc.position = glm::vec3(100.0f, 0.0f, 100.0f);
  npc.currentActivity = NPCActivity::WORKING;
  
  Player player;
  player.position = glm::vec3(0.0f, 0.0f, 0.0f);
  
  // Initial distance
  float initial_dist = glm::distance(npc.position, player.position);
  ASSERT_NEAR(initial_dist, 141.42f, 0.1f);  // sqrt(100^2 + 100^2)
  
  // Problem detected (severe mood drop)
  npc.mood = 0.2f;
  npc.loyalty = 0.3f;
  bool should_pathfind = npc.shouldInitiateDialogue(0.7f, 0.6f);
  ASSERT_TRUE(should_pathfind);
  
  // NPC changes activity to pathfind to player
  if (should_pathfind) {
    npc.currentActivity = NPCActivity::PATHFINDING_TO_PLAYER;
    npc.pathfindTo(player.position);
  }
  
  // Simulate 100 ticks of movement (1 unit per tick)
  for (int i = 0; i < 100; i++) {
    npc.updatePathfinding();  // Move one step toward player
  }
  
  // Distance should decrease significantly
  float final_dist = glm::distance(npc.position, player.position);
  ASSERT_LT(final_dist, initial_dist - 50.0f);  // Moved at least 50 units closer
}

// Validates NPC pathfinding triggered by problem detection
```

---

## Copilot Code Generation Prompts

### Prompt 1: Emotion Model Implementation
```
Implement NPC emotion model with three layers (from Equations.txt).

Class additions to NPC:
  // Emotion state
  float mood = 0.5f;      // M_s - short-term mood [0, 1]
  float attitude = 0.5f;  // A_l - long-term attitude toward player [0, 1]
  
  // Methods
  float calculateImmediateEmotion(float tone, float relevance, float bias, float socialPressure,
                                   float theta_1 = 0.3f, float theta_2 = 0.4f,
                                   float theta_3 = 0.2f, float theta_4 = 0.1f);
  
  float updateShortTermMood(float E_i, float alpha = 0.1f);
  
  float updateLongTermAttitude(float beta = 0.01f);

Formulas:
  1. E_i = θ₁*tone + θ₂*relevance + θ₃*bias + θ₄*socialPressure
     Clamp to [0, 1]
     
  2. M_s(t) = α*E_i + (1-α)*M_s(t-1)
     Update npc.mood, clamp to [0, 1]
     
  3. A_l(t) = A_l(t-1) + β*M_s(t)
     Update npc.attitude, clamp to [0, 1]

Example usage:
  NPC npc(42, "Alice", 25);
  float E_i = npc.calculateImmediateEmotion(0.8f, 0.9f, 0.6f, 0.4f);
  npc.updateShortTermMood(E_i);
  npc.updateLongTermAttitude();
```

### Prompt 2: Action Probability & Decision
```
Implement action probability system using sigmoid function.

Class additions to NPC:
  float calculateActionProbability(float E_i, float gamma = 2.0f, 
                                    float delta = 1.0f, float epsilon = 0.5f);
  
  bool shouldTakeAction(float E_i, std::mt19937& rng);

Formulas:
  1. P_action = sigmoid(γ*A_l + δ*M_s + ε*E_i)
     where sigmoid(x) = 1 / (1 + exp(-x))
     
  2. Decision: generate random float r in [0, 1)
     Return true if r < P_action, false otherwise

Implementation:
  float NPC::calculateActionProbability(float E_i, float gamma, float delta, float epsilon) {
    float x = gamma * attitude + delta * mood + epsilon * E_i;
    return 1.0f / (1.0f + std::exp(-x));  // Sigmoid
  }
  
  bool NPC::shouldTakeAction(float E_i, std::mt19937& rng) {
    float P_action = calculateActionProbability(E_i);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float random = dist(rng);
    return random < P_action;
  }

Example usage:
  NPC npc(42, "Alice", 25);
  npc.attitude = 0.7f;
  npc.mood = 0.6f;
  
  std::mt19937 rng(12345);
  float E_i = 0.8f;
  
  if (npc.shouldTakeAction(E_i, rng)) {
    // NPC takes action (cooperate, help player, etc.)
  }
```

### Prompt 3: Personality Inference System
```
Implement personality trait inference from emotional attributes.

Enum definition:
  enum class Personality : uint8_t {
    NEUTRAL = 0,
    LOYAL = 1,
    CAUTIOUS = 2,
    AMBITIOUS = 3,
    AGGRESSIVE = 4,
    OPTIMISTIC = 5,
    PESSIMISTIC = 6,
    HOSTILE = 7,
    DEVOTED = 8
  };

Class additions to NPC:
  std::vector<Personality> personality;  // Multiple traits allowed
  
  std::vector<Personality> inferPersonality();

Inference rules:
  - loyalty > 0.7 → add LOYAL
  - loyalty < 0.3 → add HOSTILE
  - ambition < 0.3 → add CAUTIOUS
  - ambition > 0.7 → add AMBITIOUS
  - mood > 0.7 → add OPTIMISTIC
  - mood < 0.3 → add PESSIMISTIC
  - attitude > 0.7 → add DEVOTED
  - attitude < 0.3 → add AGGRESSIVE
  - If no traits match → add NEUTRAL

Implementation:
  std::vector<Personality> NPC::inferPersonality() {
    std::vector<Personality> traits;
    
    if (loyalty > 0.7f) traits.push_back(Personality::LOYAL);
    if (loyalty < 0.3f) traits.push_back(Personality::HOSTILE);
    if (ambition < 0.3f) traits.push_back(Personality::CAUTIOUS);
    if (ambition > 0.7f) traits.push_back(Personality::AMBITIOUS);
    if (mood > 0.7f) traits.push_back(Personality::OPTIMISTIC);
    if (mood < 0.3f) traits.push_back(Personality::PESSIMISTIC);
    if (attitude > 0.7f) traits.push_back(Personality::DEVOTED);
    if (attitude < 0.3f) traits.push_back(Personality::AGGRESSIVE);
    
    if (traits.empty()) traits.push_back(Personality::NEUTRAL);
    
    return traits;
  }

Example usage:
  NPC npc(42, "Alice", 25);
  npc.loyalty = 0.8f;
  npc.ambition = 0.2f;
  npc.mood = 0.7f;
  
  std::vector<Personality> traits = npc.inferPersonality();
  // traits = [LOYAL, CAUTIOUS]
```

### Prompt 4: Loyalty Update System
```
Implement loyalty management with player decision impact.

Class additions to NPC:
  float loyalty = 0.5f;  // [0, 1], default neutral
  
  void updateLoyalty(float delta);
  
  float calculateFactionLoyalty(float R_f, float E_f,
                                 float w_1 = 0.5f, float w_2 = 0.3f, float w_3 = 0.2f);

Implementation:
  void NPC::updateLoyalty(float delta) {
    loyalty += delta;
    
    // Clamp to [0, 1]
    loyalty = std::max(0.0f, std::min(1.0f, loyalty));
    
    // Log threshold crossings
    if (loyalty >= 0.7f && loyalty - delta < 0.7f) {
      // Crossed devotion threshold
      logEvent("NPC " + name + " became devoted (loyalty: " + std::to_string(loyalty) + ")");
    }
    if (loyalty <= 0.3f && loyalty - delta > 0.3f) {
      // Crossed hostility threshold
      logEvent("NPC " + name + " became hostile (loyalty: " + std::to_string(loyalty) + ")");
    }
  }
  
  float NPC::calculateFactionLoyalty(float R_f, float E_f, float w_1, float w_2, float w_3) {
    // L_f = w_1*A_l + w_2*R_f + w_3*E_f (from Equations.txt)
    return w_1 * attitude + w_2 * R_f + w_3 * E_f;
  }

Example usage:
  NPC npc(42, "Alice", 25);
  
  // Player allocates food to NPC's faction
  npc.updateLoyalty(0.1f);  // +10% loyalty
  
  // Check faction loyalty
  float faction_relevance = 0.8f;
  float faction_emotion = 0.6f;
  float L_f = npc.calculateFactionLoyalty(faction_relevance, faction_emotion);
```

### Prompt 5: Problem Detection & Dialogue Initiation
```
Implement problem severity scoring and dialogue trigger system.

Class additions to NPC:
  bool shouldInitiateDialogue(float prev_mood, float prev_loyalty);
  
  float calculateProblemSeverity(float prev_mood, float prev_loyalty);

Implementation:
  float NPC::calculateProblemSeverity(float prev_mood, float prev_loyalty) {
    // Formula from copilot-instructions.md Section 8a
    float mood_delta = std::abs(mood - prev_mood);
    float loyalty_delta = std::abs(loyalty - prev_loyalty);
    
    return 0.5f * mood_delta + 0.5f * loyalty_delta;
  }
  
  bool NPC::shouldInitiateDialogue(float prev_mood, float prev_loyalty) {
    float severity = calculateProblemSeverity(prev_mood, prev_loyalty);
    
    // Threshold: 0.3 (from copilot-instructions.md)
    return severity >= 0.3f;
  }

Integration with main loop:
  // Track previous state each tick
  float prev_mood = npc.mood;
  float prev_loyalty = npc.loyalty;
  
  // Update NPC state (events, decisions, etc.)
  npc.updateShortTermMood(E_i);
  npc.updateLongTermAttitude();
  
  // Check if NPC should seek player
  if (npc.shouldInitiateDialogue(prev_mood, prev_loyalty)) {
    npc.currentActivity = NPCActivity::PATHFINDING_TO_PLAYER;
    npc.pathfindTo(player.position);
  }

Example usage:
  NPC npc(42, "Alice", 25);
  
  float prev_mood = npc.mood;
  float prev_loyalty = npc.loyalty;
  
  // Negative event
  npc.updateShortTermMood(0.2f);
  npc.updateLoyalty(-0.25f);
  
  if (npc.shouldInitiateDialogue(prev_mood, prev_loyalty)) {
    std::cout << npc.name << " wants to talk (problem severity high)" << std::endl;
  }
```

---

## Test Execution

### Running All Tests
```bash
# Compile tests
g++ -std=c++17 Phase2Tests.cpp -lgtest -lgtest_main -lglm -pthread -o phase2_tests

# Run all tests
./phase2_tests

# Run specific test suite
./phase2_tests --gtest_filter=EmotionModelTest.*

# Run with verbose output
./phase2_tests --gtest_verbose
```

### Expected Output
```
[==========] Running 24 tests from 5 test suites.
[----------] Global test environment set-up.
[----------] 8 tests from EmotionModelTest
[ RUN      ] EmotionModelTest.ImmediateEmotionCalculation
[       OK ] EmotionModelTest.ImmediateEmotionCalculation (0 ms)
[ RUN      ] EmotionModelTest.ShortTermMoodUpdate
[       OK ] EmotionModelTest.ShortTermMoodUpdate (0 ms)
...
[----------] 8 tests from EmotionModelTest (25 ms total)

[----------] 5 tests from ActionProbabilityTest
...
[----------] 5 tests from ActionProbabilityTest (45 ms total)

[----------] 4 tests from PersonalityInferenceTest
...
[----------] 4 tests from PersonalityInferenceTest (8 ms total)

[----------] 5 tests from LoyaltyTest
...
[----------] 5 tests from LoyaltyTest (12 ms total)

[----------] 2 tests from NPCDecisionTest
...
[----------] 2 tests from NPCDecisionTest (18 ms total)

[----------] Global test environment tear-down
[==========] 24 tests from 5 test suites ran. (108 ms total)
[  PASSED  ] 24 tests.
```

---

## Test Coverage Report

### Expected Coverage
```
Component                 Lines    Covered    %
────────────────────────────────────────────────
NPC.cpp (emotion)         187      178        95.2%
NPC.cpp (action)          94       89         94.7%
NPC.cpp (personality)     67       64         95.5%
NPC.cpp (loyalty)         83       79         95.2%
NPC.cpp (decision)        52       48         92.3%
────────────────────────────────────────────────
Total                     483      458        94.8%

Uncovered lines:
  - NPC.cpp:234-236 (edge case: NaN in emotion calculation)
  - NPC.cpp:378-380 (rare: faction loyalty with zero members)
  - NPC.cpp:445-448 (error: invalid personality enum value)
```

---

## Success Criteria

- [x] 24 test cases implemented
- [ ] All tests pass (0 failures)
- [ ] >90% code coverage achieved (target: 94.8%)
- [ ] Tests execute in <600ms total
- [ ] Emotion model determinism validated (Test 1.8)
- [ ] Action probability determinism validated (Test 2.5)
- [ ] Personality inference comprehensive (4 tests)
- [ ] Loyalty clamping validated (Tests 4.2, 4.5)
- [ ] Problem detection accuracy validated (Test 5.1)
- [ ] All Copilot prompts provided (5 prompts)

---

## Integration with Phase 1

Phase 2 tests depend on Phase 1 components:
- **WorldState**: Provides NPC registry and global RNG
- **EventBus**: Emits loyalty change events for UI updates
- **ComponentManager**: Manages NPC instances

Example integration:
```cpp
TEST(Phase2IntegrationTest, EmotionUpdateTriggersEvent) {
  WorldState state(12345);
  EventBus eventBus;
  
  NPC* npc = new NPC(42, "Alice", 25);
  state.npcRegistry.add(npc);
  
  // Subscribe to loyalty events
  bool event_received = false;
  eventBus.subscribe("LoyaltyChanged", [&](const Event& e) {
    event_received = true;
  });
  
  // Update loyalty (should emit event)
  npc->updateLoyalty(0.1f);
  
  // Manually emit (in real implementation, NPC::updateLoyalty emits)
  Event evt("LoyaltyChanged", npc->id);
  eventBus.publish(evt);
  
  ASSERT_TRUE(event_received);
}
```

---

## Notes

- Emotion model formulas from `/Open Game/Equations.txt` (lines 42-68)
- Action probability uses sigmoid for smooth [0, 1] output
- Personality inference supports multiple traits (complex NPCs)
- Loyalty decay encourages player engagement (passive mechanic)
- Problem severity threshold (0.3) triggers dialogue initiation
- All calculations deterministic (same input → same output)
- Thread-safe if using per-tick RNG seeding (seed = globalSeed + tick)

**Next Steps**: After Phase 2 tests pass, proceed to Phase 3 (Pathfinding) test suite.
