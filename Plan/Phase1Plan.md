# Phase 1 Implementation Plan: Foundation

**Objective**: Implement all core data structures and central registries  
**Timeline**: ~2-3 weeks of development  
**Dependency**: None (foundational phase)  
**Blocking**: Phases 2, 3, 4, and all subsequent phases

---

## Overview

Phase 1 establishes the entire data model and persistence layer for the Typed Leadership Simulator. This phase is critical—all other systems depend on having stable, efficient, serializable entity classes and registries. The focus is on **correctness** and **testability** over performance (optimization comes later).

### Key Principles
- **ID-Based References**: All cross-references use integer IDs, never pointers (enables serialization)
- **Registry Pattern**: Central registries manage all entities with O(1) lookup
- **Enum Over Strings**: Use enums for all state attributes (Mood, Skill, etc.) for memory efficiency
- **Binary Serialization**: Support efficient save/load for later phases
- **Zero External Dependencies**: No gameplay logic, no LLM calls, pure data structures

---

## Detailed Breakdown

### 1. Define Global Enums

**Files to Create**: `include/Enums.h`

**Enums to Define**:
```cpp
enum class Mood : uint8_t {
  NEUTRAL = 0,
  HAPPY = 1,
  UNHAPPY = 2,
  ANGRY = 3,
  FEARFUL = 4,
  CONTENT = 5,
  ANXIOUS = 6
};

enum class Skill : uint8_t {
  AGRICULTURE = 0,
  DIPLOMACY = 1,
  COMBAT = 2,
  EDUCATION = 3,
  RELIGION = 4,
  TRADE = 5,
  CRAFTING = 6,
  LEADERSHIP = 7
};

enum class Specialty : uint8_t {
  POLITICS = 0,
  MILITARY = 1,
  CULTURE = 2,
  RELIGION = 3
};

enum class Alignment : uint8_t {
  PLAYER_FRIENDLY = 0,
  NEUTRAL = 1,
  HOSTILE = 2
};

enum class EventType : uint8_t {
  ENVIRONMENTAL = 0,
  POLITICAL = 1,
  ECONOMIC = 2,
  SOCIAL = 3,
  RELIGIOUS = 4
};

enum class Activity : uint8_t {
  IDLE = 0,
  PATROLLING = 1,
  WORKING = 2,
  RESTING = 3,
  IN_CONVERSATION = 4
};

enum class Agenda : uint8_t {
  SHORT_TERM = 0,
  LONG_TERM = 1
};

enum class StrategyStyle : uint8_t {
  MANIPULATIVE = 0,
  HONEST = 1,
  PERSUASIVE = 2
};
```

**Implementation Tasks**:
- [ ] Create `include/Enums.h` with all 8 enum types
- [ ] Add helper functions to convert enums to strings: `string moodToString(Mood m)`
- [ ] Add reverse functions: `Mood stringToMood(string s)`
- [ ] Document each enum with range 0-255 (uint8_t)
- [ ] Add ostream operators for debugging: `cout << Mood::HAPPY`

**Tests**:
- [ ] Test enum-to-string conversion for all values
- [ ] Test string-to-enum conversion (case-insensitive)
- [ ] Test round-trip: enum→string→enum (ensure identity)

---

### 2. Define Core Data Structures

#### 2.1 Vector3 Struct
**File**: `include/Math.h`

```cpp
struct Vector3 {
  float x, y, z;
  
  Vector3(float x=0, float y=0, float z=0);
  
  float distance(const Vector3& other) const;
  Vector3 direction(const Vector3& other) const;
  Vector3 normalize() const;
  float dot(const Vector3& other) const;
  Vector3 cross(const Vector3& other) const;
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Vector3 fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement Vector3 with standard 3D operations
- [ ] Add serialization (write 3 floats to file, read 3 floats back)
- [ ] Add unit tests (distance, normalization, cross product)

#### 2.2 Resource Class
**File**: `include/Resource.h`

```cpp
class Resource {
public:
  int id;
  std::string name;
  int quantity;
  int productionRate;  // units per tick
  int consumptionRate; // units per tick
  int scarcityThreshold;
  Vector3 location;
  
  Resource(int id, const std::string& name, int initialQuantity,
           int prodRate, int consRate, int scarcityThresh);
  
  void updateResource(int change);
  bool checkScarcity() const;
  bool allocateToNPC(const std::string& npcName, int amount);
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Resource fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement Resource class with all attributes
- [ ] Add quantity bounds checking (clamp to [0, max_int])
- [ ] Implement scarcity detection: `return quantity < scarcityThreshold`
- [ ] Add allocation logic: deduct from quantity
- [ ] Implement binary serialization: write id, name, quantity, rates, threshold, location

#### 2.3 NPC Class
**File**: `include/NPC.h`

```cpp
class NPC {
public:
  // Identity
  int id;
  std::string name;
  uint16_t age;  // in years
  std::string gender;
  std::string background;
  std::string role;  // occupation
  int factionId;  // ID reference (not pointer)
  
  // Attributes
  std::vector<Skill> skills;
  float ambitionLevel;  // [0, 1]
  float loyalty;  // [0, 1] toward player
  std::vector<std::string> personality;  // traits: "cautious", "aggressive", etc.
  
  // Emotional Model
  float immediateEmotion;  // E_i, [0, 1]
  float shortTermMood;  // M_s, [0, 1]
  float longTermAttitude;  // A_l, [0, 1]
  Mood currentMood;  // enum representation
  
  // Spatial
  Vector3 position;
  Vector3 homeLocation;
  
  // Activity & Conversation
  Activity currentActivity;
  int conversationState;  // 0=not conversing, 1=pathfinding, 2=proximity, etc.
  int problemSeverity;  // [0, 1] indicating issue urgency
  
  // Constructor
  NPC(int id, const std::string& name, uint16_t age, const std::string& gender,
      const std::string& background, const std::string& role, int factionId);
  
  // Methods
  std::string reportIssue() const;
  void updateEmotion(float tone, float relevance);
  void updateMood();
  void updateAttitude();
  float getActionProbability() const;
  void interactWithFaction();
  void moveTo(Vector3 destination);
  void initiateConversation(class Player& player);
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static NPC fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement NPC class with all attributes listed above
- [ ] Add default values: loyalty=0.5, mood=0.5, attitude=0.5
- [ ] Initialize emotions as member variables (not calculated yet—that's Phase 2)
- [ ] Implement `reportIssue()` → returns string describing current problem (stub for now)
- [ ] Implement placeholder methods for emotion updates (logic in Phase 2)
- [ ] Add binary serialization: write all attributes in order

#### 2.4 Advisor Class
**File**: `include/Advisor.h`

```cpp
class Advisor : public NPC {
public:
  Specialty specialty;
  float influenceScore;  // calculated, not stored
  float trustLevel;  // [0, 1] with player
  float factionAlignment;  // [-1, 1]
  Agenda agenda;
  float riskTolerance;  // [0, 1]
  StrategyStyle strategyStyle;
  
  // Constructor
  Advisor(int id, const std::string& name, uint16_t age,
          const std::string& gender, const std::string& background,
          Specialty specialty);
  
  // Methods
  std::string giveAdvice() const;
  float calculateInfluenceScore(const class Crisis& currentIssue);
  void updateInfluence();
  void respondToPlayerDecision(const class Decision& decision);
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Advisor fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Derive Advisor from NPC (inherits all NPC attributes and methods)
- [ ] Add Advisor-specific attributes listed above
- [ ] Initialize advisors with reasonable defaults (specialty, strategy, risk tolerance)
- [ ] Implement placeholder `giveAdvice()` → returns generic string
- [ ] Implement `calculateInfluenceScore()` stub (actual formula in Phase 2)
- [ ] Add binary serialization: call parent `toBinary()` first, then write advisor-specific data

#### 2.5 Faction Class
**File**: `include/Faction.h`

```cpp
class Faction {
public:
  int id;
  std::string name;
  std::vector<int> memberIds;  // NPC IDs, not pointers
  float strength;  // S_f, calculated from member loyalties
  float emergenceProbability;  // likelihood of faction action
  Alignment alignment;  // toward player
  std::vector<int> leaders;  // NPC IDs of leaders
  std::vector<int> secretWings;  // hidden subgroup IDs
  Vector3 homeLocation;
  
  // Constructor
  Faction(int id, const std::string& name, Alignment alignment);
  
  // Methods
  void updateStrength(class NPCRegistry& registry);
  void updateEmergenceProbability();
  void performAction(const class WorldState& state);
  void interactWithOtherFaction(Faction& other);
  void addMember(int npcId);
  void removeMember(int npcId);
  bool isMember(int npcId) const;
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Faction fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement Faction class with all attributes
- [ ] Implement `addMember()` and `removeMember()` (manipulate memberIds vector)
- [ ] Implement `isMember()` → linear search in memberIds (small vector, acceptable)
- [ ] Implement placeholder `updateStrength()` → stub, logic in Phase 2
- [ ] Implement placeholder `updateEmergenceProbability()` → stub
- [ ] Implement placeholder `performAction()` → stub
- [ ] Add binary serialization

#### 2.6 Event Class
**File**: `include/Event.h`

```cpp
class Event {
public:
  int id;
  std::string name;
  EventType type;
  uint8_t impactLevel;  // [0, 10]
  std::vector<int> affectedNPCIds;  // NPC IDs affected
  std::vector<int> affectedResourceIds;  // Resource IDs affected
  Vector3 location;  // optional epicenter for 3D events
  int createdAtTick;  // when event was triggered
  
  // Constructor
  Event(int id, const std::string& name, EventType type, uint8_t impact);
  
  // Methods
  void triggerEvent();
  void applyEffects(class WorldState& state);
  void cascade(class EventRegistry& registry, int randomSeed);
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Event fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement Event class with all attributes
- [ ] Implement placeholder methods: `triggerEvent()`, `applyEffects()`, `cascade()` (logic in Phase 2)
- [ ] Add binary serialization

#### 2.7 Culture Struct
**File**: `include/Culture.h`

```cpp
struct Culture {
  std::string name;
  std::vector<std::string> norms;
  std::vector<std::string> traditions;
  float evolutionRate;  // [0, 1], how quickly culture changes
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Culture fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement Culture struct
- [ ] Add binary serialization

#### 2.8 Religion Struct
**File**: `include/Religion.h`

```cpp
struct Religion {
  std::string name;
  std::vector<std::string> doctrines;
  std::vector<int> followerIds;  // NPC IDs who follow this religion
  float schismProbability;  // [0, 1], likelihood of religious split
  
  // Serialization
  bool toBinary(std::ofstream& file) const;
  static Religion fromBinary(std::ifstream& file);
};
```

**Implementation Tasks**:
- [ ] Implement Religion struct
- [ ] Add binary serialization

---

### 3. Implement Central Registries

#### 3.1 NPCRegistry Singleton
**File**: `include/NPCRegistry.h`

```cpp
class NPCRegistry {
private:
  static NPCRegistry* instance;
  std::map<int, NPC*> npcById;
  std::vector<NPC*> allNPCs;
  int nextId;
  
  NPCRegistry();  // private constructor for singleton
  
public:
  static NPCRegistry* getInstance();
  
  NPC* getNPCById(int id);
  void addNPC(NPC* npc);
  void removeNPC(int id);
  std::vector<NPC*>& getAllNPCs();
  int getNextId();
  
  // Serialization
  bool saveToBinary(const std::string& filename);
  bool loadFromBinary(const std::string& filename);
  void clear();
};
```

**Implementation Tasks**:
- [ ] Implement singleton pattern (private constructor, static instance, getInstance())
- [ ] `getNPCById(int id)` → O(1) map lookup, return pointer or nullptr
- [ ] `addNPC(NPC* npc)` → insert into map and vector, auto-increment nextId
- [ ] `removeNPC(int id)` → erase from map, remove from vector
- [ ] `saveToBinary()` → serialize all NPCs sequentially
- [ ] `loadFromBinary()` → deserialize all NPCs, rebuild map
- [ ] Add unit tests for all operations

#### 3.2 ResourceRegistry Singleton
**File**: `include/ResourceRegistry.h`

```cpp
class ResourceRegistry {
private:
  static ResourceRegistry* instance;
  std::map<int, Resource*> resourceById;
  std::vector<Resource*> allResources;
  int nextId;
  
  ResourceRegistry();
  
public:
  static ResourceRegistry* getInstance();
  
  Resource* getResourceById(int id);
  Resource* getResourceByName(const std::string& name);
  void addResource(Resource* resource);
  void removeResource(int id);
  std::vector<Resource*>& getAllResources();
  
  // Serialization
  bool saveToBinary(const std::string& filename);
  bool loadFromBinary(const std::string& filename);
  void clear();
};
```

**Implementation Tasks**:
- [ ] Implement singleton pattern (similar to NPCRegistry)
- [ ] `getResourceById(int id)` → O(1) map lookup
- [ ] `getResourceByName(string name)` → linear search (small number of resources)
- [ ] Add binary serialization

#### 3.3 FactionRegistry Singleton
**File**: `include/FactionRegistry.h`

```cpp
class FactionRegistry {
private:
  static FactionRegistry* instance;
  std::map<int, Faction*> factionById;
  std::vector<Faction*> allFactions;
  int nextId;
  
  FactionRegistry();
  
public:
  static FactionRegistry* getInstance();
  
  Faction* getFactionById(int id);
  Faction* getFactionByName(const std::string& name);
  void addFaction(Faction* faction);
  void removeFaction(int id);
  std::vector<Faction*>& getAllFactions();
  
  // Serialization
  bool saveToBinary(const std::string& filename);
  bool loadFromBinary(const std::string& filename);
  void clear();
};
```

**Implementation Tasks**:
- [ ] Implement singleton pattern
- [ ] Add binary serialization

#### 3.4 EventRegistry Singleton
**File**: `include/EventRegistry.h`

```cpp
class EventRegistry {
private:
  static EventRegistry* instance;
  std::map<int, Event*> eventById;
  std::vector<Event*> allEvents;
  int nextId;
  
  EventRegistry();
  
public:
  static EventRegistry* getInstance();
  
  Event* getEventById(int id);
  void addEvent(Event* event);
  void removeEvent(int id);
  std::vector<Event*>& getAllEvents();
  
  // Serialization
  bool saveToBinary(const std::string& filename);
  bool loadFromBinary(const std::string& filename);
  void clear();
};
```

**Implementation Tasks**:
- [ ] Implement singleton pattern
- [ ] Add binary serialization

---

### 4. Implement Binary Serialization Framework

**File**: `include/Serializer.h`

```cpp
namespace Serializer {
  // Helper functions for binary I/O
  bool writeInt(std::ofstream& file, int value);
  bool writeFloat(std::ofstream& file, float value);
  bool writeString(std::ofstream& file, const std::string& value);
  bool writeVector(std::ofstream& file, const std::vector<int>& values);
  bool writeEnum(std::ofstream& file, uint8_t value);
  
  int readInt(std::ifstream& file);
  float readFloat(std::ifstream& file);
  std::string readString(std::ifstream& file);
  std::vector<int> readVector(std::ifstream& file);
  uint8_t readEnum(std::ifstream& file);
  
  // Error handling
  bool checkFileState(const std::ofstream& file, const std::string& context);
  bool checkFileState(const std::ifstream& file, const std::string& context);
}
```

**Implementation Tasks**:
- [ ] Implement all serializer helper functions
- [ ] Add endian handling (for cross-platform compatibility)
- [ ] Add error checking (bad stream state)
- [ ] Add logging for debug mode

---

### 5. Save File Format Specification

**File Format** (`game.dat`):

```
[Header]
format_version: int32 (e.g., 1 for v1)
tickNumber: int32
playerName: string
timestamp: int64 (unix time)

[NPC Count]
npcCount: int32

[NPCs]
for each NPC:
  id: int32
  name: string
  age: uint16
  gender: string
  background: string
  role: string
  factionId: int32
  skillCount: int32
  [skills...]
  ambitionLevel: float
  loyalty: float
  personalityCount: int32
  [personalities...]
  immediateEmotion: float
  shortTermMood: float
  longTermAttitude: float
  currentMood: uint8
  position: Vector3 (3 floats)
  homeLocation: Vector3 (3 floats)
  currentActivity: uint8
  conversationState: int32
  problemSeverity: int32

[Resources]
resourceCount: int32
for each resource:
  id: int32
  name: string
  quantity: int32
  productionRate: int32
  consumptionRate: int32
  scarcityThreshold: int32
  location: Vector3

[Factions]
factionCount: int32
for each faction:
  id: int32
  name: string
  memberCount: int32
  [memberIds...]
  strength: float
  emergenceProbability: float
  alignment: uint8
  leaderCount: int32
  [leaderIds...]
  secretWingCount: int32
  [secretWingIds...]
  homeLocation: Vector3

[Events]
eventCount: int32
for each event:
  id: int32
  name: string
  type: uint8
  impactLevel: uint8
  affectedNPCCount: int32
  [affectedNPCIds...]
  affectedResourceCount: int32
  [affectedResourceIds...]
  location: Vector3
  createdAtTick: int32
```

**Implementation Tasks**:
- [ ] Document save file format (above)
- [ ] Implement save function: `bool saveGame(const std::string& filename)`
- [ ] Implement load function: `bool loadGame(const std::string& filename)`
- [ ] Add version checking: if version != 1, handle migration or error

---

### 6. Unit Tests

**File**: `tests/Phase1Tests.cpp`

**Test Suite 1: Enum Conversions**
```cpp
TEST(EnumTests, MoodConversion) {
  EXPECT_EQ(moodToString(Mood::HAPPY), "HAPPY");
  EXPECT_EQ(stringToMood("HAPPY"), Mood::HAPPY);
  EXPECT_EQ(stringToMood("happy"), Mood::HAPPY);  // case-insensitive
}

TEST(EnumTests, AllEnumsConvert) {
  // Test all enum values round-trip correctly
}
```

**Test Suite 2: Vector3**
```cpp
TEST(Vector3Tests, Distance) {
  Vector3 a(0, 0, 0);
  Vector3 b(3, 4, 0);
  EXPECT_FLOAT_EQ(a.distance(b), 5.0f);
}

TEST(Vector3Tests, Normalization) {
  Vector3 v(3, 4, 0);
  Vector3 n = v.normalize();
  EXPECT_FLOAT_EQ(n.x, 0.6f);
  EXPECT_FLOAT_EQ(n.y, 0.8f);
}

TEST(Vector3Tests, Serialization) {
  Vector3 original(1.5f, 2.5f, 3.5f);
  // Save to file
  // Load from file
  // Assert equal
}
```

**Test Suite 3: NPC**
```cpp
TEST(NPCTests, Creation) {
  NPC npc(1, "Alice", 25, "F", "peasant", "farmer", 0);
  EXPECT_EQ(npc.id, 1);
  EXPECT_EQ(npc.name, "Alice");
  EXPECT_EQ(npc.loyalty, 0.5f);
}

TEST(NPCTests, Serialization) {
  NPC original(1, "Alice", 25, "F", "peasant", "farmer", 0);
  // Save to file
  // Load from file
  // Assert equal (id, name, loyalty, etc.)
}
```

**Test Suite 4: Registry**
```cpp
TEST(RegistryTests, NPCRegistryCRUD) {
  NPCRegistry* registry = NPCRegistry::getInstance();
  registry->clear();
  
  NPC* npc = new NPC(1, "Alice", 25, "F", "peasant", "farmer", 0);
  registry->addNPC(npc);
  
  EXPECT_EQ(registry->getNPCById(1)->name, "Alice");
  registry->removeNPC(1);
  EXPECT_EQ(registry->getNPCById(1), nullptr);
}

TEST(RegistryTests, O1Lookup) {
  NPCRegistry* registry = NPCRegistry::getInstance();
  registry->clear();
  
  // Add 1000 NPCs
  for (int i = 0; i < 1000; i++) {
    NPC* npc = new NPC(i, "NPC_" + std::to_string(i), 25, "M", "back", "role", 0);
    registry->addNPC(npc);
  }
  
  // Lookup should be fast (map-based)
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 1000; i++) {
    registry->getNPCById(i);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  EXPECT_LT(duration.count(), 1000);  // 1000 lookups in <1ms total
}
```

**Test Suite 5: Binary Save/Load**
```cpp
TEST(SerializationTests, FullGameState) {
  // Create full game state (NPCs, resources, factions, events)
  // Save to file
  // Load from file
  // Assert all entities match exactly (id, name, attributes)
}

TEST(SerializationTests, DeterministicSerialization) {
  // Create same game state twice
  // Serialize both to files
  // Assert files are byte-identical
}
```

**Implementation Tasks**:
- [ ] Create test framework (Google Test or similar)
- [ ] Write 20-30 unit tests covering all classes and registries
- [ ] Add integration test: create full game state, save, load, verify
- [ ] Add performance test: 1000-NPC save/load timing

---

## File Structure

```
include/
  Enums.h
  Math.h
  Resource.h
  NPC.h
  Advisor.h
  Faction.h
  Event.h
  Culture.h
  Religion.h
  NPCRegistry.h
  ResourceRegistry.h
  FactionRegistry.h
  EventRegistry.h
  Serializer.h

src/
  Enums.cpp
  Math.cpp
  Resource.cpp
  NPC.cpp
  Advisor.cpp
  Faction.cpp
  Event.cpp
  Culture.cpp
  Religion.cpp
  NPCRegistry.cpp
  ResourceRegistry.cpp
  FactionRegistry.cpp
  EventRegistry.cpp
  Serializer.cpp

tests/
  Phase1Tests.cpp
  CMakeLists.txt

data/
  (empty for now, config files added later)
```

---

## Success Criteria Checklist

- [ ] All 8 enum types defined and tested
- [ ] All 8 data classes/structs implemented (NPC, Advisor, Resource, Faction, Event, Culture, Religion, Vector3)
- [ ] All 4 registries implemented as singletons with O(1) lookups
- [ ] Binary serialization implemented for all classes
- [ ] Save file format documented and working
- [ ] 30+ unit tests written and passing
- [ ] Integration test: full game state save/load cycle works correctly
- [ ] Performance test: 1000-NPC save/load completes in <2 seconds
- [ ] All code compiles without warnings
- [ ] Deterministic serialization verified (same input = identical bytes)

---

## Implementation Order

1. **Enums** (1-2 hours) — Least dependencies, easy to test
2. **Math.h (Vector3)** (2-3 hours) — Pure math, no dependencies
3. **Data Classes** (4-6 hours) — Implement in order: Resource, NPC, Advisor, Faction, Event, Culture, Religion
4. **Serializer Framework** (2-3 hours) — Helper functions for binary I/O
5. **Registries** (3-4 hours) — Singleton pattern, map management
6. **Binary Serialization** (3-4 hours) — Implement `toBinary()` and `fromBinary()` for all classes
7. **Unit Tests** (4-6 hours) — Test each class individually, then integration tests
8. **Documentation** (1-2 hours) — Add code comments, API documentation

**Estimated Total**: 20-30 hours of development

---

## Copilot Code Generation Tips

When requesting Copilot to generate Phase 1 code:

1. **For Each Enum**: "Create enum class {EnumName} : uint8_t with values {list}. Add helper functions: {enumName}ToString() and stringTo{EnumName}()."

2. **For Each Data Class**: "Implement class/struct {ClassName} with attributes: {list}. Add constructor, getter/setter methods, and placeholder methods {list}. Add toBinary() and fromBinary() methods."

3. **For Registries**: "Implement singleton pattern for {RegistryName} with: private constructor, static getInstance(), map<int, {EntityType}*> for O(1) lookup, add/remove/get methods, saveToBinary() and loadFromBinary()."

4. **For Tests**: "Create unit test suite for {ClassName} using Google Test. Test: construction, serialization round-trip, error conditions."

---

## Notes & Warnings

- **No Game Logic Yet**: Phase 1 is purely data structures. All gameplay logic stubs are placeholders for Phase 2.
- **Memory Management**: Use `new` for heap allocation; implement cleanup in registry destructors.
- **Endianness**: Binary format may differ across platforms. Test on target platform or use explicit endian handling.
- **String Serialization**: Serialize strings as [length: int32][data: char*]. Ensure null-termination.
- **Floating-Point Precision**: Use `float` (32-bit), not `double`, for consistency. Test serialization round-trip for drift.

---

## Next Phase Dependency

Phase 2 (Simulation Engine) requires:
- All Phase 1 classes implemented and tested
- Registries fully functional
- Binary serialization working

Start Phase 2 only after Phase 1 complete and all tests passing.
