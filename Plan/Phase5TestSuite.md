# Phase 5 Test Suite: Faction & Diplomacy Systems

## Overview
This test suite validates the Faction & Diplomacy Systems implementation for the Typed Leadership Simulator. Phase 5 focuses on faction relationship modeling, diplomatic actions, treaty management, alliance mechanics, and conflict resolution. The tests ensure 90%+ code coverage, deterministic behavior, and performance under 600ms execution time.

**Test Targets:**
- Faction relationship modeling (loyalty, strength, alignment)
- Diplomatic actions (negotiation, alliance formation, treaty signing)
- Treaty management (enforcement, violation detection, expiration)
- Alliance mechanics (coalition formation, shared objectives, betrayal)
- Conflict resolution (mediation, escalation prevention, faction wars)

**Key Dependencies:**
- Phase 1: WorldState for faction registry and RNG
- Phase 2: NPC emotion system for faction member loyalty
- Phase 3: Pathfinding for faction movement and territory control
- Phase 4: Resource management for diplomatic leverage

**Performance Targets:**
- <600ms total execution time
- 90%+ code coverage
- Deterministic results with seeded RNG
- Memory usage <50MB during tests

## Test Suite Structure

### 1. Faction Relationship Modeling Tests (6 tests)
Tests for faction loyalty calculations, strength assessment, and alignment dynamics.

### 2. Diplomatic Actions Tests (5 tests)
Tests for negotiation mechanics, alliance formation, and treaty signing.

### 3. Treaty Management Tests (4 tests)
Tests for treaty enforcement, violation detection, and expiration handling.

### 4. Alliance Mechanics Tests (4 tests)
Tests for coalition formation, shared objectives, and betrayal scenarios.

### 5. Conflict Resolution Tests (4 tests)
Tests for mediation systems, escalation prevention, and faction war mechanics.

### 6. Integration Tests (3 tests)
Tests for cross-system interactions and full diplomatic scenarios.

### 7. Performance & Determinism Tests (2 tests)
Tests for execution speed and reproducible results.

## 1. Faction Relationship Modeling Tests

### Test 1.1: Faction Loyalty Calculation
**Purpose:** Validate faction loyalty calculation from member NPC attitudes.

**Code Example:**
```cpp
TEST(FactionRelationshipTest, LoyaltyCalculation) {
    // Setup
    WorldState state(42); // Seeded RNG
    NPCRegistry registry;
    
    // Create faction with 3 members
    Faction faction(1, "TestFaction");
    NPC* npc1 = registry.createNPC("Alice", 25, FEMALE, "Warrior");
    npc1->setAttitude(0.8f); // High loyalty
    faction.addMember(npc1->getId());
    
    NPC* npc2 = registry.createNPC("Bob", 30, MALE, "Merchant");
    npc2->setAttitude(0.4f); // Medium loyalty
    faction.addMember(npc2->getId());
    
    NPC* npc3 = registry.createNPC("Charlie", 35, MALE, "Priest");
    npc3->setAttitude(0.1f); // Low loyalty
    faction.addMember(npc3->getId());
    
    // Execute
    faction.updateLoyalty();
    
    // Verify: Average loyalty = (0.8 + 0.4 + 0.1) / 3 = 0.433...
    EXPECT_NEAR(faction.getLoyalty(), 0.433f, 0.01f);
}
```

### Test 1.2: Faction Strength Assessment
**Purpose:** Validate faction strength calculation using Equations.txt formula.

**Code Example:**
```cpp
TEST(FactionRelationshipTest, StrengthCalculation) {
    // Setup
    WorldState state(42);
    NPCRegistry registry;
    
    Faction faction(1, "Warriors");
    NPC* leader = registry.createNPC("Leader", 40, MALE, "Warrior");
    leader->setCapability(0.9f); // High capability
    leader->setAttitude(0.8f);
    faction.addMember(leader->getId());
    
    NPC* member = registry.createNPC("Member", 25, MALE, "Warrior");
    member->setCapability(0.6f); // Medium capability
    member->setAttitude(0.6f);
    faction.addMember(member->getId());
    
    // Execute: S_f = Σ(L_f_i * C_i)
    faction.updateStrength();
    
    // Verify: (0.8*0.9) + (0.6*0.6) = 0.72 + 0.36 = 1.08
    EXPECT_NEAR(faction.getStrength(), 1.08f, 0.01f);
}
```

### Test 1.3: Faction Alignment Dynamics
**Purpose:** Test faction alignment changes based on player actions.

**Code Example:**
```cpp
TEST(FactionRelationshipTest, AlignmentDynamics) {
    WorldState state(42);
    NPCRegistry registry;
    
    Faction faction(1, "Merchants", NEUTRAL);
    NPC* merchant = registry.createNPC("Trader", 35, MALE, "Merchant");
    merchant->setAttitude(0.5f);
    faction.addMember(merchant->getId());
    
    // Player favors faction
    faction.receiveFavor(0.2f);
    
    // Verify alignment improves
    EXPECT_EQ(faction.getAlignment(), FRIENDLY);
    EXPECT_NEAR(faction.getLoyalty(), 0.7f, 0.01f);
}
```

### Test 1.4: Faction Emergence Probability
**Purpose:** Validate emergence calculation using sigmoid function.

**Code Example:**
```cpp
TEST(FactionRelationshipTest, EmergenceProbability) {
    WorldState state(42);
    
    Faction faction(1, "Rebels", HOSTILE);
    faction.setLoyalty(0.2f); // Low loyalty triggers emergence
    
    faction.updateEmergenceProbability();
    
    // P_emerge = sigmoid(k * (1 - avg(L_f)))
    // With k=2.0: sigmoid(2.0 * (1-0.2)) = sigmoid(1.6) ≈ 0.832
    EXPECT_NEAR(faction.getEmergenceProbability(), 0.832f, 0.01f);
}
```

### Test 1.5: Faction Member Recruitment
**Purpose:** Test adding/removing members and loyalty recalculation.

**Code Example:**
```cpp
TEST(FactionRelationshipTest, MemberRecruitment) {
    WorldState state(42);
    NPCRegistry registry;
    
    Faction faction(1, "Warriors");
    NPC* warrior1 = registry.createNPC("Warrior1", 25, MALE, "Warrior");
    warrior1->setAttitude(0.8f);
    faction.addMember(warrior1->getId());
    
    faction.updateLoyalty();
    float initialLoyalty = faction.getLoyalty();
    
    // Add high-loyalty member
    NPC* warrior2 = registry.createNPC("Warrior2", 30, MALE, "Warrior");
    warrior2->setAttitude(0.9f);
    faction.addMember(warrior2->getId());
    
    faction.updateLoyalty();
    
    // Verify loyalty increases
    EXPECT_GT(faction.getLoyalty(), initialLoyalty);
}
```

### Test 1.6: Faction Relationship Network
**Purpose:** Test relationships between multiple factions.

**Code Example:**
```cpp
TEST(FactionRelationshipTest, RelationshipNetwork) {
    WorldState state(42);
    
    Faction warriors(1, "Warriors");
    Faction merchants(2, "Merchants");
    
    // Set up alliance
    warriors.addAlly(merchants.getId());
    merchants.addAlly(warriors.getId());
    
    // Test relationship queries
    EXPECT_TRUE(warriors.isAlliedWith(merchants.getId()));
    EXPECT_TRUE(merchants.isAlliedWith(warriors.getId()));
    
    // Break alliance
    warriors.breakAlliance(merchants.getId());
    EXPECT_FALSE(warriors.isAlliedWith(merchants.getId()));
}
```

## 2. Diplomatic Actions Tests

### Test 2.1: Negotiation Success Probability
**Purpose:** Validate negotiation outcomes based on faction strength and player influence.

**Code Example:**
```cpp
TEST(DiplomaticActionsTest, NegotiationSuccess) {
    WorldState state(42);
    
    Faction faction(1, "Merchants");
    faction.setStrength(0.8f);
    faction.setLoyalty(0.6f);
    
    Player player;
    player.setInfluence(0.7f);
    
    DiplomaticAction negotiation(TRADE_AGREEMENT);
    float successProb = negotiation.calculateSuccessProbability(faction, player);
    
    // Success based on combined strength and influence
    EXPECT_NEAR(successProb, 0.75f, 0.05f);
}
```

### Test 2.2: Alliance Formation
**Purpose:** Test creating alliances between compatible factions.

**Code Example:**
```cpp
TEST(DiplomaticActionsTest, AllianceFormation) {
    WorldState state(42);
    
    Faction warriors(1, "Warriors", FRIENDLY);
    Faction merchants(2, "Merchants", FRIENDLY);
    
    DiplomaticAction alliance(ALLIANCE_FORMATION);
    
    // Compatible factions can form alliance
    EXPECT_TRUE(alliance.canExecute(warriors, merchants));
    
    alliance.execute(warriors, merchants);
    
    EXPECT_TRUE(warriors.isAlliedWith(merchants.getId()));
    EXPECT_TRUE(merchants.isAlliedWith(warriors.getId()));
}
```

### Test 2.3: Treaty Signing Process
**Purpose:** Validate treaty creation with terms and conditions.

**Code Example:**
```cpp
TEST(DiplomaticActionsTest, TreatySigning) {
    WorldState state(42);
    
    Faction factionA(1, "FactionA");
    Faction factionB(2, "FactionB");
    
    Treaty treaty(TRADE_AGREEMENT);
    treaty.addTerm("Trade goods freely");
    treaty.addTerm("Mutual defense");
    treaty.setDuration(365); // Days
    
    DiplomaticAction signTreaty(SIGN_TREATY);
    signTreaty.executeWithTreaty(factionA, factionB, treaty);
    
    // Verify treaty is active
    EXPECT_TRUE(factionA.hasTreatyWith(factionB.getId()));
    EXPECT_TRUE(factionB.hasTreatyWith(factionA.getId()));
    EXPECT_EQ(treaty.getDuration(), 365);
}
```

### Test 2.4: Diplomatic Gift Exchange
**Purpose:** Test resource gifts to improve faction relations.

**Code Example:**
```cpp
TEST(DiplomaticActionsTest, GiftExchange) {
    WorldState state(42);
    ResourceRegistry resources;
    
    Faction faction(1, "Merchants");
    faction.setLoyalty(0.5f);
    
    Resource* gold = resources.createResource("Gold", 100);
    
    DiplomaticAction gift(GIFT_RESOURCES);
    gift.executeWithResource(faction, gold, 25);
    
    // Verify loyalty improvement
    EXPECT_GT(faction.getLoyalty(), 0.5f);
    EXPECT_EQ(gold->getQuantity(), 75); // 25 given away
}
```

### Test 2.5: Diplomatic Crisis Response
**Purpose:** Test handling diplomatic incidents and de-escalation.

**Code Example:**
```cpp
TEST(DiplomaticActionsTest, CrisisResponse) {
    WorldState state(42);
    
    Faction faction(1, "Warriors", NEUTRAL);
    
    // Create diplomatic incident
    DiplomaticIncident incident(BORDER_VIOLATION);
    incident.setSeverity(0.8f);
    faction.receiveIncident(incident);
    
    // Player responds with apology
    DiplomaticAction apology(DIPLOMATIC_APOLOGY);
    apology.execute(faction);
    
    // Verify tension reduction
    EXPECT_LT(faction.getTension(), 0.8f);
    EXPECT_EQ(faction.getAlignment(), FRIENDLY);
}
```

## 3. Treaty Management Tests

### Test 3.1: Treaty Enforcement
**Purpose:** Validate treaty terms are properly enforced.

**Code Example:**
```cpp
TEST(TreatyManagementTest, TreatyEnforcement) {
    WorldState state(42);
    
    Faction factionA(1, "FactionA");
    Faction factionB(2, "FactionB");
    
    Treaty treaty(TRADE_AGREEMENT);
    treaty.addTerm("No tariffs");
    
    factionA.signTreaty(factionB.getId(), treaty);
    
    // Attempt violation
    bool violation = factionA.attemptViolation(factionB.getId(), "Impose tariffs");
    
    // Treaty prevents violation
    EXPECT_FALSE(violation);
    EXPECT_TRUE(treaty.isEnforced());
}
```

### Test 3.2: Treaty Violation Detection
**Purpose:** Test detection and consequences of treaty violations.

**Code Example:**
```cpp
TEST(TreatyManagementTest, ViolationDetection) {
    WorldState state(42);
    
    Faction violator(1, "Violator");
    Faction victim(2, "Victim");
    
    Treaty treaty(PEACE_TREATY);
    violator.signTreaty(victim.getId(), treaty);
    
    // Commit violation
    violator.commitViolation(victim.getId(), "Attack border");
    
    // Verify violation detected
    EXPECT_TRUE(treaty.isViolated());
    EXPECT_LT(violator.getLoyalty(), 0.5f); // Loyalty penalty
    EXPECT_EQ(violator.getAlignment(), HOSTILE);
}
```

### Test 3.3: Treaty Expiration
**Purpose:** Test treaty expiration and renewal mechanics.

**Code Example:**
```cpp
TEST(TreatyManagementTest, TreatyExpiration) {
    WorldState state(42);
    
    Faction factionA(1, "FactionA");
    Faction factionB(2, "FactionB");
    
    Treaty treaty(ALLIANCE);
    treaty.setDuration(30); // 30 days
    
    factionA.signTreaty(factionB.getId(), treaty);
    
    // Advance time
    for(int day = 0; day < 35; day++) {
        treaty.advanceDay();
    }
    
    // Verify treaty expired
    EXPECT_TRUE(treaty.isExpired());
    EXPECT_FALSE(factionA.hasTreatyWith(factionB.getId()));
}
```

### Test 3.4: Treaty Renewal Process
**Purpose:** Validate treaty renewal negotiations.

**Code Example:**
```cpp
TEST(TreatyManagementTest, TreatyRenewal) {
    WorldState state(42);
    
    Faction factionA(1, "FactionA");
    Faction factionB(2, "FactionB");
    
    Treaty treaty(TRADE_AGREEMENT);
    treaty.setDuration(30);
    factionA.signTreaty(factionB.getId(), treaty);
    
    // Advance to near expiration
    for(int day = 0; day < 25; day++) {
        treaty.advanceDay();
    }
    
    // Attempt renewal
    DiplomaticAction renewal(RENEW_TREATY);
    bool success = renewal.executeWithTreaty(factionA, factionB, treaty);
    
    EXPECT_TRUE(success);
    EXPECT_FALSE(treaty.isExpired());
    EXPECT_GT(treaty.getDuration(), 30); // Extended duration
}
```

## 4. Alliance Mechanics Tests

### Test 4.1: Coalition Formation
**Purpose:** Test forming coalitions of multiple factions.

**Code Example:**
```cpp
TEST(AllianceMechanicsTest, CoalitionFormation) {
    WorldState state(42);
    
    Faction leader(1, "LeaderFaction");
    Faction member1(2, "Member1");
    Faction member2(3, "Member2");
    
    Coalition coalition("GrandAlliance");
    coalition.addLeader(leader.getId());
    coalition.addMember(member1.getId());
    coalition.addMember(member2.getId());
    
    // Verify coalition structure
    EXPECT_TRUE(coalition.isLeader(leader.getId()));
    EXPECT_TRUE(coalition.hasMember(member1.getId()));
    EXPECT_TRUE(coalition.hasMember(member2.getId()));
    
    // Test coalition strength
    EXPECT_NEAR(coalition.getTotalStrength(), 
                leader.getStrength() + member1.getStrength() + member2.getStrength(), 
                0.01f);
}
```

### Test 4.2: Shared Objectives
**Purpose:** Validate alliance objectives and progress tracking.

**Code Example:**
```cpp
TEST(AllianceMechanicsTest, SharedObjectives) {
    WorldState state(42);
    
    Coalition coalition("DefenseAlliance");
    coalition.addObjective("Defend territory");
    coalition.addObjective("Build fortifications");
    
    // Progress on objectives
    coalition.updateObjectiveProgress("Defend territory", 0.6f);
    coalition.updateObjectiveProgress("Build fortifications", 0.3f);
    
    // Verify progress tracking
    EXPECT_NEAR(coalition.getObjectiveProgress("Defend territory"), 0.6f, 0.01f);
    EXPECT_NEAR(coalition.getObjectiveProgress("Build fortifications"), 0.3f, 0.01f);
    
    // Overall coalition morale based on objective progress
    EXPECT_NEAR(coalition.getMorale(), 0.45f, 0.01f); // Average progress
}
```

### Test 4.3: Alliance Betrayal
**Purpose:** Test betrayal mechanics and consequences.

**Code Example:**
```cpp
TEST(AllianceMechanicsTest, AllianceBetrayal) {
    WorldState state(42);
    
    Coalition coalition("TradeAlliance");
    Faction traitor(1, "Traitor");
    Faction loyal(2, "Loyal");
    
    coalition.addMember(traitor.getId());
    coalition.addMember(loyal.getId());
    
    // Traitor betrays alliance
    traitor.betrayCoalition(coalition.getId());
    
    // Verify consequences
    EXPECT_FALSE(coalition.hasMember(traitor.getId()));
    EXPECT_LT(traitor.getLoyalty(), 0.3f); // Loyalty penalty
    EXPECT_EQ(traitor.getAlignment(), HOSTILE);
    
    // Loyal member unaffected
    EXPECT_TRUE(coalition.hasMember(loyal.getId()));
}
```

### Test 4.4: Alliance Resource Sharing
**Purpose:** Test resource distribution within alliances.

**Code Example:**
```cpp
TEST(AllianceMechanicsTest, ResourceSharing) {
    WorldState state(42);
    ResourceRegistry resources;
    
    Coalition coalition("ResourceAlliance");
    Faction factionA(1, "FactionA");
    Faction factionB(2, "FactionB");
    
    coalition.addMember(factionA.getId());
    coalition.addMember(factionB.getId());
    
    Resource* food = resources.createResource("Food", 100);
    factionA.addResource(food->getId(), 100);
    
    // Share resources
    coalition.shareResource(food->getId(), 30);
    
    // Verify distribution
    EXPECT_EQ(factionA.getResourceQuantity(food->getId()), 70);
    EXPECT_EQ(factionB.getResourceQuantity(food->getId()), 30);
}
```

## 5. Conflict Resolution Tests

### Test 5.1: Mediation System
**Purpose:** Test diplomatic mediation of conflicts.

**Code Example:**
```cpp
TEST(ConflictResolutionTest, MediationSystem) {
    WorldState state(42);
    
    Faction aggressor(1, "Aggressor", HOSTILE);
    Faction victim(2, "Victim", FRIENDLY);
    
    Conflict conflict(BORDER_DISPUTE);
    conflict.setAggressor(aggressor.getId());
    conflict.setVictim(victim.getId());
    conflict.setSeverity(0.8f);
    
    DiplomaticAction mediation(MEDIATE_CONFLICT);
    bool success = mediation.executeOnConflict(conflict);
    
    if(success) {
        EXPECT_LT(conflict.getSeverity(), 0.8f);
        EXPECT_EQ(aggressor.getAlignment(), NEUTRAL);
    }
}
```

### Test 5.2: Escalation Prevention
**Purpose:** Validate preventing conflict escalation.

**Code Example:**
```cpp
TEST(ConflictResolutionTest, EscalationPrevention) {
    WorldState state(42);
    
    Faction factionA(1, "FactionA");
    Faction factionB(2, "FactionB");
    
    Conflict conflict(TRADE_DISPUTE);
    conflict.setSeverity(0.6f); // Moderate severity
    
    // Attempt de-escalation
    DiplomaticAction deescalate(DEESCALATE_CONFLICT);
    deescalate.executeOnConflict(conflict);
    
    // Verify escalation prevented
    EXPECT_LT(conflict.getSeverity(), 0.6f);
    EXPECT_FALSE(conflict.isEscalated());
}
```

### Test 5.3: Faction War Mechanics
**Purpose:** Test full-scale faction warfare.

**Code Example:**
```cpp
TEST(ConflictResolutionTest, FactionWar) {
    WorldState state(42);
    
    Faction attacker(1, "Attacker");
    Faction defender(2, "Defender");
    
    War war(attacker.getId(), defender.getId());
    war.setCasualties(50);
    
    // Execute war tick
    war.advanceTick();
    
    // Verify war effects
    EXPECT_LT(attacker.getStrength(), 1.0f); // Strength reduced
    EXPECT_LT(defender.getStrength(), 1.0f);
    
    // Check for war resolution
    if(war.getDuration() > 100) { // Long war
        EXPECT_TRUE(war.isResolved());
    }
}
```

### Test 5.4: Peace Negotiation
**Purpose:** Test negotiating peace treaties during war.

**Code Example:**
```cpp
TEST(ConflictResolutionTest, PeaceNegotiation) {
    WorldState state(42);
    
    Faction factionA(1, "FactionA");
    Faction factionB(2, "FactionB");
    
    War war(factionA.getId(), factionB.getId());
    
    // Negotiate peace
    DiplomaticAction peace(PEACE_NEGOTIATION);
    Treaty peaceTreaty(PEACE_TREATY);
    
    bool success = peace.executeWithTreaty(factionA, factionB, peaceTreaty);
    
    if(success) {
        EXPECT_TRUE(war.isResolved());
        EXPECT_TRUE(factionA.hasTreatyWith(factionB.getId()));
        EXPECT_EQ(factionA.getAlignment(), NEUTRAL);
    }
}
```

## 6. Integration Tests

### Test 6.1: Full Diplomatic Scenario
**Purpose:** Test complete diplomatic interaction from negotiation to treaty.

**Code Example:**
```cpp
TEST(IntegrationTest, FullDiplomaticScenario) {
    WorldState state(42);
    NPCRegistry registry;
    ResourceRegistry resources;
    
    // Setup factions
    Faction warriors(1, "Warriors");
    Faction merchants(2, "Merchants");
    
    // Add members
    NPC* warrior = registry.createNPC("Warrior", 30, MALE, "Warrior");
    warrior->setAttitude(0.7f);
    warriors.addMember(warrior->getId());
    
    NPC* merchant = registry.createNPC("Merchant", 35, MALE, "Merchant");
    merchant->setAttitude(0.6f);
    merchants.addMember(merchant->getId());
    
    // Diplomatic process
    DiplomaticAction negotiation(TRADE_NEGOTIATION);
    negotiation.execute(warriors, merchants);
    
    DiplomaticAction alliance(ALLIANCE_FORMATION);
    alliance.execute(warriors, merchants);
    
    Treaty treaty(TRADE_AGREEMENT);
    warriors.signTreaty(merchants.getId(), treaty);
    
    // Verify full integration
    EXPECT_TRUE(warriors.isAlliedWith(merchants.getId()));
    EXPECT_TRUE(warriors.hasTreatyWith(merchants.getId()));
    EXPECT_GT(warriors.getLoyalty(), 0.7f); // Improved relations
}
```

### Test 6.2: Multi-Faction Diplomacy
**Purpose:** Test diplomacy involving multiple factions simultaneously.

**Code Example:**
```cpp
TEST(IntegrationTest, MultiFactionDiplomacy) {
    WorldState state(42);
    
    Faction faction1(1, "Faction1");
    Faction faction2(2, "Faction2");
    Faction faction3(3, "Faction3");
    
    // Create complex relationships
    faction1.addAlly(faction2.getId());
    faction2.addAlly(faction3.getId());
    faction1.addRival(faction3.getId());
    
    // Coalition forms against common rival
    Coalition coalition("AntiFaction3");
    coalition.addMember(faction1.getId());
    coalition.addMember(faction2.getId());
    
    // Verify relationship complexity
    EXPECT_TRUE(faction1.isAlliedWith(faction2.getId()));
    EXPECT_TRUE(faction2.isAlliedWith(faction3.getId()));
    EXPECT_TRUE(faction1.isRivalOf(faction3.getId()));
    
    // Coalition should strengthen alliance
    EXPECT_GT(coalition.getTotalStrength(), faction1.getStrength() + faction2.getStrength());
}
```

### Test 6.3: Crisis Response Integration
**Purpose:** Test integrated crisis response with multiple systems.

**Code Example:**
```cpp
TEST(IntegrationTest, CrisisResponseIntegration) {
    WorldState state(42);
    NPCRegistry registry;
    
    Faction rebels(1, "Rebels", HOSTILE);
    Faction loyalists(2, "Loyalists", FRIENDLY);
    
    // Create crisis
    DiplomaticCrisis crisis(REVOLT);
    crisis.setSeverity(0.9f);
    rebels.triggerCrisis(crisis);
    
    // Multi-pronged response
    DiplomaticAction mediation(MEDIATE_CONFLICT);
    mediation.execute(rebels);
    
    DiplomaticAction gift(GIFT_RESOURCES);
    Resource* gold = state.getResources().createResource("Gold", 1000);
    gift.executeWithResource(loyalists, gold, 200);
    
    DiplomaticAction alliance(ALLIANCE_FORMATION);
    alliance.execute(loyalists, rebels);
    
    // Verify integrated response
    EXPECT_LT(crisis.getSeverity(), 0.9f);
    EXPECT_TRUE(loyalists.isAlliedWith(rebels.getId()));
    EXPECT_LT(rebels.getAlignment(), HOSTILE);
}
```

## 7. Performance & Determinism Tests

### Test 7.1: Performance Benchmark
**Purpose:** Ensure test suite runs within performance targets.

**Code Example:**
```cpp
TEST(PerformanceTest, ExecutionTime) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run all faction/diplomacy operations
    WorldState state(42);
    runFullDiplomaticSimulation(state, 100); // 100 diplomatic operations
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 600); // <600ms target
}
```

### Test 7.2: Determinism Validation
**Purpose:** Ensure reproducible results with seeded RNG.

**Code Example:**
```cpp
TEST(DeterminismTest, ReproducibleResults) {
    // Run simulation twice with same seed
    WorldState state1(12345);
    runDiplomaticScenario(state1);
    std::string hash1 = state1.getStateHash();
    
    WorldState state2(12345);
    runDiplomaticScenario(state2);
    std::string hash2 = state2.getStateHash();
    
    // Verify identical results
    EXPECT_EQ(hash1, hash2);
}
```

## Copilot Prompts

### Prompt 1: Faction Relationship Modeling
```
Create a Faction class with the following features:
- Properties: id, name, memberIds (vector<int>), strength (float), loyalty (float), alignment (enum)
- Methods: addMember(), removeMember(), updateStrength(), updateLoyalty(), isAlliedWith()
- Use Equations.txt formulas for strength and loyalty calculations
- Include binary serialization methods
- Add relationship tracking with other factions
```

### Prompt 2: Diplomatic Actions System
```
Implement a DiplomaticAction class hierarchy:
- Base class: DiplomaticAction with execute() method
- Derived classes: NegotiationAction, AllianceAction, TreatyAction, GiftAction
- Each action should calculate success probability based on faction strength and player influence
- Include cooldown periods between diplomatic actions
- Add consequence tracking for successful/failed actions
```

### Prompt 3: Treaty Management System
```
Create a Treaty class with:
- Properties: id, type, parties (vector<int>), terms (vector<string>), duration, expirationDate
- Methods: sign(), violate(), enforce(), renew(), isExpired()
- Include violation detection and consequence application
- Add treaty relationship queries (isAtWar, hasTradeAgreement, etc.)
- Implement treaty cascading effects on faction relationships
```

### Prompt 4: Alliance Mechanics
```
Implement Coalition class for alliance mechanics:
- Properties: id, name, leaderId, memberIds, objectives (map<string, float>), morale
- Methods: addMember(), removeMember(), updateMorale(), shareResources()
- Include betrayal mechanics with loyalty penalties
- Add objective progress tracking and completion rewards
- Implement coalition dissolution when morale drops too low
```

### Prompt 5: Conflict Resolution System
```
Create Conflict and War classes:
- Conflict: severity, parties, type, resolutionStatus
- War: inherits from Conflict, adds casualties, duration, battleOutcomes
- Methods: escalate(), deescalate(), mediate(), resolve()
- Include war exhaustion mechanics (morale drops over time)
- Add peace negotiation system with treaty generation
```

## Edge Cases & Validation

### Mathematical Edge Cases
- **Empty Faction:** Faction with 0 members (strength/loyalty calculations)
- **Single Member Faction:** Faction with 1 member (averages equal individual values)
- **Maximum Loyalty:** NPC with loyalty = 1.0 (clamping behavior)
- **Minimum Strength:** Faction with all low-capability members
- **Alliance Cycles:** A allied with B, B with C, C with A (relationship transitivity)

### Logic Edge Cases
- **Simultaneous Treaty Violations:** Multiple factions violate same treaty
- **Coalition Betrayal Chain:** Member betrays, causing cascade of betrayals
- **War Declaration During Mediation:** Conflict escalates during peace talks
- **Resource Gift to Hostile Faction:** Gift improves relations despite hostility
- **Treaty Expiration During War:** Treaty expires while war is active

### Performance Edge Cases
- **1000 Factions:** Memory usage and lookup performance
- **Complex Alliance Web:** 50+ interconnected factions
- **Rapid Diplomatic Actions:** 100+ actions per tick
- **Long-Running Wars:** Wars lasting 1000+ ticks
- **Resource-Intensive Treaties:** Treaties affecting 50+ resources

## Determinism Validation

### RNG Seeding Strategy
- Global seed: 42 for all tests
- Per-operation seeds: seed + operation_index
- Hash validation: FNV-1a hash of faction states after operations
- Bit-identical comparison: memcmp() on serialized faction data

### State Consistency Checks
- Pre/post operation state hashes
- Relationship network integrity
- Resource conservation (gifts don't create/destroy resources)
- Time progression consistency

### Replay Validation
- Record all diplomatic actions in chronological log
- Replay log produces identical faction states
- LLM responses cached and replayed identically

## Summary

**Test Coverage:** 26 tests across 7 categories
**Code Coverage Target:** 90%+ for faction and diplomacy systems
**Performance Target:** <600ms total execution time
**Determinism:** 100% reproducible with seeded RNG
**Integration:** Full cross-system validation with Phase 1-4 dependencies

**Key Validation Points:**
- Faction relationship calculations use Equations.txt formulas
- Diplomatic actions have realistic success probabilities
- Treaty system enforces agreements and detects violations
- Alliance mechanics support complex coalition dynamics
- Conflict resolution prevents unwanted escalation
- All systems integrate seamlessly with existing codebase

**Next Steps:** Phase 6 Test Suite (Narrative & Quest Systems)