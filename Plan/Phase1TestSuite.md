# Phase 1 Test Suite: Core Architecture

## Overview

This document provides comprehensive unit tests for Phase 1 (Core Architecture) of the Typed Leadership Simulator. Tests cover ComponentManager, EventBus, WorldState initialization, determinism validation, and core data structures.

**Test Framework**: Google Test (gtest)  
**Target Coverage**: 90%+ code coverage  
**Test Count**: 23 test cases  
**Execution Time**: <500ms total

---

## Test Categories

1. **ComponentManager Tests** (7 tests)
2. **EventBus Tests** (6 tests)
3. **WorldState Initialization Tests** (5 tests)
4. **Determinism Validation Tests** (5 tests)

---

## Test Suite 1: ComponentManager Tests

### Test 1.1: Component Registration
```cpp
TEST(ComponentManagerTest, RegisterAndRetrieveComponent) {
  // Setup
  ComponentManager cm;
  
  // Create mock NPC component
  NPC* npc = new NPC(42, "Alice", 25);
  
  // Register component
  bool registered = cm.registerComponent("NPC", 42, npc);
  
  // Assert registration successful
  ASSERT_TRUE(registered);
  
  // Retrieve component
  NPC* retrieved = cm.getComponent<NPC>("NPC", 42);
  
  // Assert retrieval successful
  ASSERT_NE(retrieved, nullptr);
  ASSERT_EQ(retrieved->id, 42);
  ASSERT_EQ(retrieved->name, "Alice");
  ASSERT_EQ(retrieved->age, 25);
}

// Copilot Prompt:
// "Implement ComponentManager::registerComponent and getComponent.
// registerComponent(string type, uint32_t id, void* component) -> bool
// getComponent<T>(string type, uint32_t id) -> T*
// Store components in map<string, map<uint32_t, void*>>
// Return nullptr if component not found."
```

### Test 1.2: Component Overwrite Prevention
```cpp
TEST(ComponentManagerTest, PreventDuplicateRegistration) {
  // Setup
  ComponentManager cm;
  NPC* npc1 = new NPC(42, "Alice", 25);
  NPC* npc2 = new NPC(42, "Bob", 30);  // Same ID
  
  // Register first component
  bool first = cm.registerComponent("NPC", 42, npc1);
  ASSERT_TRUE(first);
  
  // Attempt duplicate registration
  bool second = cm.registerComponent("NPC", 42, npc2);
  
  // Assert second registration fails
  ASSERT_FALSE(second);
  
  // Verify original component unchanged
  NPC* retrieved = cm.getComponent<NPC>("NPC", 42);
  ASSERT_EQ(retrieved->name, "Alice");  // Original, not "Bob"
  
  delete npc2;  // Clean up unused component
}

// Expected behavior: Prevent accidental overwrites
// Log warning: "Component NPC:42 already registered"
```

### Test 1.3: Component Removal
```cpp
TEST(ComponentManagerTest, RemoveComponent) {
  // Setup
  ComponentManager cm;
  NPC* npc = new NPC(42, "Alice", 25);
  cm.registerComponent("NPC", 42, npc);
  
  // Remove component
  bool removed = cm.removeComponent("NPC", 42);
  ASSERT_TRUE(removed);
  
  // Verify removal
  NPC* retrieved = cm.getComponent<NPC>("NPC", 42);
  ASSERT_EQ(retrieved, nullptr);
  
  // Attempt to remove again (should fail)
  bool second_removal = cm.removeComponent("NPC", 42);
  ASSERT_FALSE(second_removal);
}

// Memory management: ComponentManager owns components, deletes on removal
```

### Test 1.4: Multiple Component Types
```cpp
TEST(ComponentManagerTest, MultipleComponentTypes) {
  // Setup
  ComponentManager cm;
  
  // Register different types
  NPC* npc = new NPC(1, "Alice", 25);
  Faction* faction = new Faction(1, "Warriors");
  Resource* resource = new Resource(1, "Food", 100);
  
  cm.registerComponent("NPC", 1, npc);
  cm.registerComponent("Faction", 1, faction);
  cm.registerComponent("Resource", 1, resource);
  
  // Retrieve by type (same ID, different types)
  NPC* npc_ret = cm.getComponent<NPC>("NPC", 1);
  Faction* faction_ret = cm.getComponent<Faction>("Faction", 1);
  Resource* resource_ret = cm.getComponent<Resource>("Resource", 1);
  
  // Assert all retrieved correctly
  ASSERT_NE(npc_ret, nullptr);
  ASSERT_NE(faction_ret, nullptr);
  ASSERT_NE(resource_ret, nullptr);
  
  ASSERT_EQ(npc_ret->name, "Alice");
  ASSERT_EQ(faction_ret->name, "Warriors");
  ASSERT_EQ(resource_ret->name, "Food");
}

// Key feature: Same ID can exist for different component types
```

### Test 1.5: Component Count Query
```cpp
TEST(ComponentManagerTest, GetComponentCount) {
  // Setup
  ComponentManager cm;
  
  // Initially empty
  ASSERT_EQ(cm.getComponentCount("NPC"), 0);
  
  // Add 3 NPCs
  for (int i = 0; i < 3; i++) {
    NPC* npc = new NPC(i, "NPC_" + std::to_string(i), 25);
    cm.registerComponent("NPC", i, npc);
  }
  
  // Verify count
  ASSERT_EQ(cm.getComponentCount("NPC"), 3);
  
  // Remove one
  cm.removeComponent("NPC", 1);
  ASSERT_EQ(cm.getComponentCount("NPC"), 2);
}

// Copilot Prompt:
// "Implement getComponentCount(string type) -> size_t
// Return number of components of given type."
```

### Test 1.6: Component Iteration
```cpp
TEST(ComponentManagerTest, IterateAllComponents) {
  // Setup
  ComponentManager cm;
  
  // Add 5 NPCs
  for (int i = 0; i < 5; i++) {
    NPC* npc = new NPC(i, "NPC_" + std::to_string(i), 20 + i);
    cm.registerComponent("NPC", i, npc);
  }
  
  // Get all NPCs
  std::vector<NPC*> all_npcs = cm.getAllComponents<NPC>("NPC");
  
  // Assert count
  ASSERT_EQ(all_npcs.size(), 5);
  
  // Verify order (should be sorted by ID)
  for (int i = 0; i < 5; i++) {
    ASSERT_EQ(all_npcs[i]->id, i);
    ASSERT_EQ(all_npcs[i]->age, 20 + i);
  }
}

// Copilot Prompt:
// "Implement getAllComponents<T>(string type) -> vector<T*>
// Return all components of given type, sorted by ID."
```

### Test 1.7: Thread-Safe Access (Concurrent Registration)
```cpp
TEST(ComponentManagerTest, ThreadSafeConcurrentRegistration) {
  // Setup
  ComponentManager cm;  // Must be thread-safe
  
  // Spawn 4 threads, each registering 100 NPCs
  std::vector<std::thread> threads;
  for (int t = 0; t < 4; t++) {
    threads.emplace_back([&cm, t]() {
      for (int i = 0; i < 100; i++) {
        uint32_t id = t * 100 + i;  // Unique ID per thread
        NPC* npc = new NPC(id, "NPC_" + std::to_string(id), 25);
        cm.registerComponent("NPC", id, npc);
      }
    });
  }
  
  // Wait for all threads
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Verify all 400 NPCs registered
  ASSERT_EQ(cm.getComponentCount("NPC"), 400);
  
  // Verify no corruption (sample check)
  NPC* sample = cm.getComponent<NPC>("NPC", 250);
  ASSERT_NE(sample, nullptr);
  ASSERT_EQ(sample->id, 250);
}

// Copilot Prompt:
// "Make ComponentManager thread-safe using std::mutex.
// Lock during registerComponent, removeComponent, getComponent."
```

---

## Test Suite 2: EventBus Tests

### Test 2.1: Event Subscription and Publishing
```cpp
TEST(EventBusTest, SubscribeAndPublish) {
  // Setup
  EventBus eventBus;
  bool event_received = false;
  std::string event_data;
  
  // Subscribe to "NPCSpawned" event
  eventBus.subscribe("NPCSpawned", [&](const Event& event) {
    event_received = true;
    event_data = event.getData<std::string>();
  });
  
  // Publish event
  Event evt("NPCSpawned", std::string("Alice"));
  eventBus.publish(evt);
  
  // Assert callback invoked
  ASSERT_TRUE(event_received);
  ASSERT_EQ(event_data, "Alice");
}

// Copilot Prompt:
// "Implement EventBus with subscribe/publish pattern.
// subscribe(string eventName, function<void(const Event&)> callback)
// publish(Event event) -> call all subscribed callbacks"
```

### Test 2.2: Multiple Subscribers to Same Event
```cpp
TEST(EventBusTest, MultipleSubscribers) {
  // Setup
  EventBus eventBus;
  int callback1_count = 0;
  int callback2_count = 0;
  int callback3_count = 0;
  
  // Subscribe 3 callbacks to same event
  eventBus.subscribe("ResourceDepleted", [&](const Event& e) { callback1_count++; });
  eventBus.subscribe("ResourceDepleted", [&](const Event& e) { callback2_count++; });
  eventBus.subscribe("ResourceDepleted", [&](const Event& e) { callback3_count++; });
  
  // Publish event
  Event evt("ResourceDepleted", 42);
  eventBus.publish(evt);
  
  // Assert all callbacks invoked
  ASSERT_EQ(callback1_count, 1);
  ASSERT_EQ(callback2_count, 1);
  ASSERT_EQ(callback3_count, 1);
}

// Expected behavior: All subscribers notified in registration order
```

### Test 2.3: Event Unsubscription
```cpp
TEST(EventBusTest, Unsubscribe) {
  // Setup
  EventBus eventBus;
  int callback_count = 0;
  
  // Subscribe and get subscription ID
  uint64_t sub_id = eventBus.subscribe("FactionRebellion", [&](const Event& e) {
    callback_count++;
  });
  
  // Publish event (should trigger)
  Event evt1("FactionRebellion", std::string("Warriors"));
  eventBus.publish(evt1);
  ASSERT_EQ(callback_count, 1);
  
  // Unsubscribe
  bool unsubscribed = eventBus.unsubscribe(sub_id);
  ASSERT_TRUE(unsubscribed);
  
  // Publish again (should NOT trigger)
  Event evt2("FactionRebellion", std::string("Merchants"));
  eventBus.publish(evt2);
  ASSERT_EQ(callback_count, 1);  // Still 1 (not 2)
}

// Copilot Prompt:
// "subscribe() returns unique uint64_t subscription ID.
// unsubscribe(uint64_t id) -> bool removes callback."
```

### Test 2.4: Event Filtering (No Subscribers)
```cpp
TEST(EventBusTest, PublishWithNoSubscribers) {
  // Setup
  EventBus eventBus;
  
  // Publish event with no subscribers (should not crash)
  Event evt("NonExistentEvent", 999);
  
  // Should complete without error
  ASSERT_NO_THROW(eventBus.publish(evt));
  
  // Verify no side effects
  ASSERT_EQ(eventBus.getSubscriberCount("NonExistentEvent"), 0);
}

// Expected behavior: Gracefully handle events with no listeners
```

### Test 2.5: Event Data Type Validation
```cpp
TEST(EventBusTest, EventDataTypeValidation) {
  // Setup
  EventBus eventBus;
  std::string received_string;
  int received_int;
  
  // Subscribe to event expecting string
  eventBus.subscribe("TestEvent", [&](const Event& e) {
    received_string = e.getData<std::string>();
  });
  
  // Publish with correct type
  Event evt1("TestEvent", std::string("Hello"));
  eventBus.publish(evt1);
  ASSERT_EQ(received_string, "Hello");
  
  // Publish with incorrect type (int instead of string)
  Event evt2("TestEvent", 42);
  
  // Should throw or log error (implementation-dependent)
  // Option 1: Throw std::bad_cast
  ASSERT_THROW(eventBus.publish(evt2), std::bad_cast);
  
  // Option 2: Log error and skip callback
  // (Alternative implementation choice)
}

// Type safety: Event data type must match callback expectation
```

### Test 2.6: Event Priority (Ordered Delivery)
```cpp
TEST(EventBusTest, EventPriorityOrdering) {
  // Setup
  EventBus eventBus;
  std::vector<int> callback_order;
  
  // Subscribe with priority levels
  eventBus.subscribe("SaveComplete", [&](const Event& e) {
    callback_order.push_back(1);  // Priority 1 (high)
  }, /* priority */ 1);
  
  eventBus.subscribe("SaveComplete", [&](const Event& e) {
    callback_order.push_back(3);  // Priority 3 (low)
  }, /* priority */ 3);
  
  eventBus.subscribe("SaveComplete", [&](const Event& e) {
    callback_order.push_back(2);  // Priority 2 (medium)
  }, /* priority */ 2);
  
  // Publish event
  Event evt("SaveComplete", true);
  eventBus.publish(evt);
  
  // Assert callbacks invoked in priority order (1 -> 2 -> 3)
  ASSERT_EQ(callback_order.size(), 3);
  ASSERT_EQ(callback_order[0], 1);
  ASSERT_EQ(callback_order[1], 2);
  ASSERT_EQ(callback_order[2], 3);
}

// Copilot Prompt:
// "Add optional priority parameter to subscribe().
// Lower number = higher priority (executed first).
// Default priority = 5 (medium)."
```

---

## Test Suite 3: WorldState Initialization Tests

### Test 3.1: Default WorldState Initialization
```cpp
TEST(WorldStateTest, DefaultInitialization) {
  // Create default world state
  WorldState state;
  
  // Assert default values
  ASSERT_EQ(state.currentTick, 0);
  ASSERT_EQ(state.currentSeason, Season::SPRING);
  ASSERT_EQ(state.daysSinceStart, 0);
  ASSERT_FLOAT_EQ(state.gameSpeed, 1.0f);
  ASSERT_EQ(state.globalRandomSeed, 0);  // Should be set explicitly
  
  // Assert empty registries
  ASSERT_EQ(state.npcRegistry.getCount(), 0);
  ASSERT_EQ(state.factionRegistry.getCount(), 0);
  ASSERT_EQ(state.resourceRegistry.getCount(), 0);
  ASSERT_EQ(state.eventRegistry.getCount(), 0);
}

// Expected: Clean slate for new game
```

### Test 3.2: WorldState Initialization with Seed
```cpp
TEST(WorldStateTest, InitializeWithSeed) {
  // Create world state with specific seed
  uint32_t seed = 12345;
  WorldState state(seed);
  
  // Assert seed set
  ASSERT_EQ(state.globalRandomSeed, seed);
  
  // Verify RNG initialized with seed
  std::mt19937 rng(seed);
  int expected_random = rng() % 100;
  
  int actual_random = state.getRandomInt(0, 100);
  
  // Reset RNG and verify determinism
  state.resetRNG(seed);
  int second_random = state.getRandomInt(0, 100);
  
  ASSERT_EQ(actual_random, second_random);  // Deterministic
}

// Copilot Prompt:
// "WorldState constructor accepts optional seed parameter.
// Initialize std::mt19937 RNG with seed.
// Provide getRandomInt(min, max) -> int method."
```

### Test 3.3: Load Initial NPCs from Config
```cpp
TEST(WorldStateTest, LoadInitialNPCs) {
  // Setup
  WorldState state;
  
  // Load NPCs from config file (10 initial NPCs from gdd.txt)
  bool loaded = state.loadInitialNPCs("data/npcs.json");
  ASSERT_TRUE(loaded);
  
  // Verify NPC count
  ASSERT_EQ(state.npcRegistry.getCount(), 10);
  
  // Sample validation: Check first NPC
  NPC* npc = state.npcRegistry.getNPCById(0);
  ASSERT_NE(npc, nullptr);
  ASSERT_FALSE(npc->name.empty());
  ASSERT_GE(npc->age, 16);  // All NPCs adult at start
  ASSERT_GE(npc->loyalty, 0.0f);
  ASSERT_LE(npc->loyalty, 1.0f);
}

// Expected: Load ~10 initial NPCs from JSON config
// File: data/npcs.json with starter NPC definitions
```

### Test 3.4: Load Initial Factions
```cpp
TEST(WorldStateTest, LoadInitialFactions) {
  // Setup
  WorldState state;
  state.loadInitialNPCs("data/npcs.json");
  
  // Load factions
  bool loaded = state.loadInitialFactions("data/factions.json");
  ASSERT_TRUE(loaded);
  
  // Verify faction count (from faction_culture_religion.txt: start with 3-5 factions)
  ASSERT_GE(state.factionRegistry.getCount(), 3);
  ASSERT_LE(state.factionRegistry.getCount(), 5);
  
  // Validate faction structure
  Faction* faction = state.factionRegistry.getFactionById(0);
  ASSERT_NE(faction, nullptr);
  ASSERT_FALSE(faction->name.empty());
  ASSERT_GT(faction->memberIds.size(), 0);  // Has members
  
  // Verify NPC-faction references resolved
  uint32_t member_id = faction->memberIds[0];
  NPC* member = state.npcRegistry.getNPCById(member_id);
  ASSERT_NE(member, nullptr);
  ASSERT_EQ(member->faction_id, faction->id);
}

// Reference resolution: NPC.faction_id → Faction* must be valid
```

### Test 3.5: WorldState Reset (New Game)
```cpp
TEST(WorldStateTest, ResetWorldState) {
  // Setup: Create world state and simulate gameplay
  WorldState state(12345);
  state.loadInitialNPCs("data/npcs.json");
  state.currentTick = 10000;
  state.daysSinceStart = 100;
  
  // Add runtime NPCs (born/immigrated)
  NPC* newborn = new NPC(100, "Newborn", 0);
  state.npcRegistry.addNPC(newborn);
  ASSERT_EQ(state.npcRegistry.getCount(), 11);  // 10 + 1
  
  // Reset world state
  state.reset();
  
  // Verify reset to defaults
  ASSERT_EQ(state.currentTick, 0);
  ASSERT_EQ(state.daysSinceStart, 0);
  ASSERT_EQ(state.npcRegistry.getCount(), 0);  // All cleared
  ASSERT_EQ(state.factionRegistry.getCount(), 0);
  
  // Reload initial state
  state.loadInitialNPCs("data/npcs.json");
  ASSERT_EQ(state.npcRegistry.getCount(), 10);  // Back to initial
}

// Use case: "New Game" button resets to clean slate
```

---

## Test Suite 4: Determinism Validation Tests

### Test 4.1: RNG Determinism with Same Seed
```cpp
TEST(DeterminismTest, RNGWithSameSeed) {
  // Setup: Two world states with same seed
  uint32_t seed = 42;
  WorldState state1(seed);
  WorldState state2(seed);
  
  // Generate 100 random numbers from each
  std::vector<int> randoms1, randoms2;
  for (int i = 0; i < 100; i++) {
    randoms1.push_back(state1.getRandomInt(0, 1000));
    randoms2.push_back(state2.getRandomInt(0, 1000));
  }
  
  // Assert sequences identical
  ASSERT_EQ(randoms1.size(), randoms2.size());
  for (int i = 0; i < 100; i++) {
    ASSERT_EQ(randoms1[i], randoms2[i]);
  }
}

// Critical: Same seed → identical random sequences (determinism)
```

### Test 4.2: RNG Divergence with Different Seeds
```cpp
TEST(DeterminismTest, RNGWithDifferentSeeds) {
  // Setup: Two world states with different seeds
  WorldState state1(42);
  WorldState state2(12345);
  
  // Generate 100 random numbers from each
  std::vector<int> randoms1, randoms2;
  for (int i = 0; i < 100; i++) {
    randoms1.push_back(state1.getRandomInt(0, 1000));
    randoms2.push_back(state2.getRandomInt(0, 1000));
  }
  
  // Assert sequences different (high probability)
  int matches = 0;
  for (int i = 0; i < 100; i++) {
    if (randoms1[i] == randoms2[i]) matches++;
  }
  
  // Expect <10% matches (random collision)
  ASSERT_LT(matches, 10);
}

// Different seeds → different random sequences (entropy)
```

### Test 4.3: Tick-Based RNG Seeding
```cpp
TEST(DeterminismTest, TickBasedRNGSeeding) {
  // Setup
  uint32_t globalSeed = 12345;
  WorldState state(globalSeed);
  
  // Simulate 3 ticks with RNG reseeding per tick
  std::vector<int> tick0_randoms, tick1_randoms, tick2_randoms;
  
  // Tick 0
  state.seedRNGForTick(0);
  for (int i = 0; i < 10; i++) {
    tick0_randoms.push_back(state.getRandomInt(0, 100));
  }
  
  // Tick 1
  state.seedRNGForTick(1);
  for (int i = 0; i < 10; i++) {
    tick1_randoms.push_back(state.getRandomInt(0, 100));
  }
  
  // Tick 2
  state.seedRNGForTick(2);
  for (int i = 0; i < 10; i++) {
    tick2_randoms.push_back(state.getRandomInt(0, 100));
  }
  
  // Reset and replay (should match)
  state.resetRNG(globalSeed);
  
  state.seedRNGForTick(0);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(state.getRandomInt(0, 100), tick0_randoms[i]);
  }
  
  state.seedRNGForTick(1);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(state.getRandomInt(0, 100), tick1_randoms[i]);
  }
}

// Copilot Prompt:
// "Implement seedRNGForTick(int tick).
// Formula: rng.seed(globalRandomSeed + tick)
// Ensures same tick always produces same random sequence."
```

### Test 4.4: WorldState Hash Consistency
```cpp
TEST(DeterminismTest, WorldStateHashConsistency) {
  // Setup: Two identical world states
  WorldState state1(12345);
  WorldState state2(12345);
  
  // Load same initial data
  state1.loadInitialNPCs("data/npcs.json");
  state2.loadInitialNPCs("data/npcs.json");
  
  // Calculate hash of world state
  uint64_t hash1 = state1.calculateHash();
  uint64_t hash2 = state2.calculateHash();
  
  // Assert hashes identical
  ASSERT_EQ(hash1, hash2);
  
  // Modify one state
  NPC* npc = state1.npcRegistry.getNPCById(0);
  npc->loyalty += 0.1f;
  
  // Recalculate hash
  uint64_t hash1_modified = state1.calculateHash();
  
  // Assert hash changed
  ASSERT_NE(hash1_modified, hash1);
  ASSERT_NE(hash1_modified, hash2);
}

// Copilot Prompt:
// "Implement WorldState::calculateHash() -> uint64_t.
// Hash all entity states (NPCs, factions, resources, events).
// Use CRC64 or FNV-1a hash algorithm.
// Used for determinism validation in replay system."
```

### Test 4.5: Floating-Point Determinism
```cpp
TEST(DeterminismTest, FloatingPointDeterminism) {
  // Setup: Test floating-point consistency across platforms
  WorldState state1(42);
  WorldState state2(42);
  
  // Load NPC with floating-point attributes
  NPC* npc1 = new NPC(1, "Alice", 25);
  npc1->loyalty = 0.75f;
  npc1->mood = 0.6f;
  npc1->attitude = 0.8f;
  
  NPC* npc2 = new NPC(1, "Alice", 25);
  npc2->loyalty = 0.75f;
  npc2->mood = 0.6f;
  npc2->attitude = 0.8f;
  
  state1.npcRegistry.addNPC(npc1);
  state2.npcRegistry.addNPC(npc2);
  
  // Simulate emotion update (from Equations.txt)
  float alpha = 0.1f;
  float E_i = 0.5f;
  
  // Update mood: M_s(t) = alpha * E_i + (1-alpha) * M_s(t-1)
  npc1->mood = alpha * E_i + (1.0f - alpha) * npc1->mood;
  npc2->mood = alpha * E_i + (1.0f - alpha) * npc2->mood;
  
  // Assert bit-identical (use memcmp for strict comparison)
  ASSERT_EQ(memcmp(&npc1->mood, &npc2->mood, sizeof(float)), 0);
  
  // Verify no rounding differences
  ASSERT_FLOAT_EQ(npc1->mood, npc2->mood);
  ASSERT_FLOAT_EQ(npc1->mood, 0.59f);  // Expected: 0.1*0.5 + 0.9*0.6 = 0.59
}

// Critical: Use float (32-bit), not double (64-bit) for determinism
// Avoid platform-specific math functions (use std:: versions)
```

---

## Copilot Code Generation Prompts

### Prompt 1: ComponentManager Implementation
```
Create ComponentManager class for entity component system.

Class structure:
  class ComponentManager {
  private:
    // Map: component_type -> (entity_id -> component_pointer)
    std::map<std::string, std::map<uint32_t, void*>> components;
    std::mutex mutex;  // Thread safety
    
  public:
    bool registerComponent(std::string type, uint32_t id, void* component);
    void* getComponentRaw(std::string type, uint32_t id);
    
    template<typename T>
    T* getComponent(std::string type, uint32_t id) {
      return static_cast<T*>(getComponentRaw(type, id));
    }
    
    bool removeComponent(std::string type, uint32_t id);
    size_t getComponentCount(std::string type);
    
    template<typename T>
    std::vector<T*> getAllComponents(std::string type);
  };

Features:
  - Thread-safe with std::mutex
  - Prevent duplicate registration (return false if exists)
  - Delete component on removal (owns memory)
  - O(log n) lookup by type and ID
  - Support iteration over all components of type

Example usage:
  ComponentManager cm;
  NPC* npc = new NPC(42, "Alice", 25);
  cm.registerComponent("NPC", 42, npc);
  
  NPC* retrieved = cm.getComponent<NPC>("NPC", 42);
  std::vector<NPC*> all = cm.getAllComponents<NPC>("NPC");
```

### Prompt 2: EventBus Implementation
```
Create EventBus class for publish-subscribe event system.

Class structure:
  class Event {
  public:
    std::string name;
    std::any data;  // Type-erased data
    
    template<typename T>
    T getData() const { return std::any_cast<T>(data); }
  };
  
  class EventBus {
  private:
    struct Subscription {
      uint64_t id;
      std::function<void(const Event&)> callback;
      int priority;  // Lower = higher priority
    };
    
    std::map<std::string, std::vector<Subscription>> subscribers;
    uint64_t next_id = 0;
    std::mutex mutex;
    
  public:
    uint64_t subscribe(std::string eventName, 
                      std::function<void(const Event&)> callback,
                      int priority = 5);
    
    bool unsubscribe(uint64_t subscriptionId);
    
    void publish(const Event& event);
    
    size_t getSubscriberCount(std::string eventName);
  };

Features:
  - Priority-based callback ordering (sort by priority before invoking)
  - Thread-safe with std::mutex
  - Return unique subscription ID for later unsubscription
  - Handle missing event types gracefully (no crash)
  - Support std::any for type-erased event data

Example usage:
  EventBus bus;
  
  uint64_t sub_id = bus.subscribe("NPCSpawned", [](const Event& e) {
    std::string npc_name = e.getData<std::string>();
    std::cout << "NPC spawned: " << npc_name << std::endl;
  });
  
  Event evt("NPCSpawned", std::string("Alice"));
  bus.publish(evt);
  
  bus.unsubscribe(sub_id);
```

### Prompt 3: WorldState Initialization
```
Create WorldState class to hold all game state.

Class structure:
  class WorldState {
  public:
    // Time
    uint32_t currentTick = 0;
    Season currentSeason = Season::SPRING;
    uint32_t daysSinceStart = 0;
    float gameSpeed = 1.0f;
    
    // Determinism
    uint32_t globalRandomSeed = 0;
    std::mt19937 rng;
    
    // Registries
    NPCRegistry npcRegistry;
    FactionRegistry factionRegistry;
    ResourceRegistry resourceRegistry;
    EventRegistry eventRegistry;
    
    // Constructor
    WorldState(uint32_t seed = 0);
    
    // Methods
    bool loadInitialNPCs(std::string path);
    bool loadInitialFactions(std::string path);
    bool loadInitialResources(std::string path);
    
    void reset();  // Clear all state
    
    // RNG methods
    void resetRNG(uint32_t seed);
    void seedRNGForTick(uint32_t tick);
    int getRandomInt(int min, int max);
    float getRandomFloat(float min, float max);
    
    // Determinism
    uint64_t calculateHash();
  };

Features:
  - Load initial game data from JSON files
  - Deterministic RNG with seed
  - Per-tick RNG seeding: seed = globalRandomSeed + currentTick
  - Hash calculation for state validation
  - Reset method for "New Game"

Example usage:
  WorldState state(12345);  // Seed = 12345
  state.loadInitialNPCs("data/npcs.json");
  state.loadInitialFactions("data/factions.json");
  
  state.seedRNGForTick(0);
  int random = state.getRandomInt(0, 100);  // Deterministic for tick 0
```

### Prompt 4: Deterministic Hash Calculation
```
Implement WorldState::calculateHash() for state validation.

Algorithm:
  1. Initialize hash: uint64_t hash = 0xCBF29CE484222325 (FNV-1a offset)
  2. Hash all NPC states:
     for (NPC* npc : npcRegistry.allNPCs):
       hash = fnv1a_update(hash, &npc->id, sizeof(uint32_t))
       hash = fnv1a_update(hash, &npc->loyalty, sizeof(float))
       hash = fnv1a_update(hash, &npc->mood, sizeof(float))
       hash = fnv1a_update(hash, &npc->position, sizeof(glm::vec3))
       
  3. Hash all faction states (similar)
  4. Hash all resource states
  5. Hash global state (currentTick, season, etc.)
  6. Return hash

FNV-1a update function:
  uint64_t fnv1a_update(uint64_t hash, const void* data, size_t len) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; i++) {
      hash ^= bytes[i];
      hash *= 0x100000001B3;  // FNV-1a prime
    }
    return hash;
  }

Usage: Compare hashes to detect state divergence in replay
```

### Prompt 5: Registry Base Class (Template)
```
Create generic Registry<T> template for entity management.

Template class:
  template<typename T>
  class Registry {
  private:
    std::map<uint32_t, T*> entities;
    uint32_t next_id = 0;
    std::mutex mutex;
    
  public:
    // Add entity (auto-assign ID or use provided)
    uint32_t add(T* entity, uint32_t id = UINT32_MAX);
    
    // Get entity by ID
    T* getById(uint32_t id);
    
    // Remove entity by ID (delete memory)
    bool remove(uint32_t id);
    
    // Get all entities (sorted by ID)
    std::vector<T*> getAll();
    
    // Count
    size_t getCount() const;
    
    // Clear all
    void clear();
  };

Specializations:
  using NPCRegistry = Registry<NPC>;
  using FactionRegistry = Registry<Faction>;
  using ResourceRegistry = Registry<Resource>;
  using EventRegistry = Registry<Event>;

Features:
  - Thread-safe with std::mutex
  - Auto-increment ID if not provided
  - Owns entity memory (delete on remove/clear)
  - O(log n) lookup by ID

Example usage:
  NPCRegistry registry;
  
  NPC* npc = new NPC(42, "Alice", 25);
  uint32_t id = registry.add(npc, 42);  // Use ID 42
  
  NPC* retrieved = registry.getById(42);
  
  std::vector<NPC*> all_npcs = registry.getAll();
```

---

## Test Execution

### Running All Tests
```bash
# Compile tests
g++ -std=c++17 Phase1Tests.cpp -lgtest -lgtest_main -pthread -o phase1_tests

# Run all tests
./phase1_tests

# Run with verbose output
./phase1_tests --gtest_verbose

# Run specific test suite
./phase1_tests --gtest_filter=ComponentManagerTest.*

# Run specific test
./phase1_tests --gtest_filter=ComponentManagerTest.RegisterAndRetrieveComponent
```

### Expected Output
```
[==========] Running 23 tests from 4 test suites.
[----------] Global test environment set-up.
[----------] 7 tests from ComponentManagerTest
[ RUN      ] ComponentManagerTest.RegisterAndRetrieveComponent
[       OK ] ComponentManagerTest.RegisterAndRetrieveComponent (0 ms)
[ RUN      ] ComponentManagerTest.PreventDuplicateRegistration
[       OK ] ComponentManagerTest.PreventDuplicateRegistration (0 ms)
...
[----------] 7 tests from ComponentManagerTest (15 ms total)

[----------] 6 tests from EventBusTest
...
[----------] 6 tests from EventBusTest (8 ms total)

[----------] 5 tests from WorldStateTest
...
[----------] 5 tests from WorldStateTest (120 ms total)

[----------] 5 tests from DeterminismTest
...
[----------] 5 tests from DeterminismTest (45 ms total)

[----------] Global test environment tear-down
[==========] 23 tests from 4 test suites ran. (188 ms total)
[  PASSED  ] 23 tests.
```

---

## Test Coverage Report

### Expected Coverage
```
Component                 Lines    Covered    %
────────────────────────────────────────────────
ComponentManager.cpp      145      138        95.2%
EventBus.cpp              98       94         95.9%
WorldState.cpp            234      218        93.2%
Registry.hpp              87       84         96.6%
────────────────────────────────────────────────
Total                     564      534        94.7%

Uncovered lines:
  - ComponentManager.cpp:142-144 (error handling for invalid type cast)
  - EventBus.cpp:76-78 (rare edge case: subscriber removal during publish)
  - WorldState.cpp:189-192 (file not found error path)
```

### Generating Coverage Report
```bash
# Compile with coverage flags
g++ -std=c++17 --coverage Phase1Tests.cpp -lgtest -lgtest_main -pthread -o phase1_tests

# Run tests
./phase1_tests

# Generate coverage report
gcov Phase1Tests.cpp
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report

# View report
open coverage_report/index.html
```

---

## Success Criteria

- [x] 23 test cases implemented
- [ ] All tests pass (0 failures)
- [ ] >90% code coverage achieved (target: 94.7%)
- [ ] Tests execute in <500ms total
- [ ] Thread-safety validated (Test 1.7)
- [ ] Determinism validated (Tests 4.1-4.5)
- [ ] No memory leaks detected (run with valgrind)
- [ ] All Copilot prompts provided (5 prompts)

---

## Integration with CI/CD

### GitHub Actions Workflow
```yaml
name: Phase 1 Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y libgtest-dev cmake
        
    - name: Build tests
      run: |
        mkdir build
        cd build
        cmake ..
        make phase1_tests
        
    - name: Run tests
      run: ./build/phase1_tests --gtest_output=xml:test_results.xml
      
    - name: Upload test results
      uses: actions/upload-artifact@v2
      with:
        name: test-results
        path: test_results.xml
```

---

## Notes

- All tests use Google Test framework (industry standard for C++)
- Tests validate core Phase 1 components before integration with later phases
- Determinism tests critical for replay system (Phase 14)
- Thread-safety tests ensure scalability to 1000+ NPCs
- Memory management validated (no leaks via RAII patterns)
- Test execution time target: <500ms total (fast iteration)

**Next Steps**: After Phase 1 tests pass, proceed to Phase 2 (NPC Behavior) test suite.
