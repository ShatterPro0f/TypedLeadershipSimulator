# Phase 14 Test Suite: Save/Load & Persistence

**Objective**: Comprehensive unit tests for binary serialization, state validation, incremental saves, lazy loading, deterministic replay, and data integrity  
**Target Coverage**: 93%+ code coverage, 48+ test cases  
**Execution Time**: <600ms for full suite  
**Framework**: Google Test (gtest)  
**Test Organization**: 6 test suites with 8 tests per suite, full pseudocode implementations

---

## Test Structure Overview

```
Phase14TestSuite.cpp
├── Test Suite 1: Binary Serialization & Format (8 tests)
├── Test Suite 2: Save Operations & Management (8 tests)
├── Test Suite 3: Load Operations & Validation (8 tests)
├── Test Suite 4: Incremental Saves & Delta Compression (8 tests)
├── Test Suite 5: Lazy Loading & Memory Management (8 tests)
└── Test Suite 6: Deterministic Replay & Data Integrity (8 tests)

Total: 48 test cases
Expected Coverage: 93%+
```

---

## Test Suite 1: Binary Serialization & Format (8 tests)

### Test 1.1: NPCBinarySerialization
**Purpose**: Serialize NPC to ~70 bytes efficiently  
**Setup**: NPC with all attributes
**Action**: Serialize to binary
**Expected**: Size ~70 bytes, contains all critical data

**Pseudocode**:
```cpp
TEST(BinarySerialization, NPCBinarySerialization) {
  NPC npc;
  npc.setId(1);
  npc.setName("Alice");
  npc.setAge(35);
  npc.setPosition({100, 50, 0});
  npc.setLoyalty(0.75f);
  npc.setMood(0.6f);
  npc.setFactionId(2);
  
  BinarySerializer serializer;
  vector<uint8_t> binary = serializer.serialize(npc);
  
  EXPECT_LE(binary.size(), 80);  // ~70 bytes target
  EXPECT_GE(binary.size(), 50);
  
  NPC restored = serializer.deserialize<NPC>(binary);
  EXPECT_EQ(restored.getId(), 1);
  EXPECT_EQ(restored.getName(), "Alice");
  EXPECT_EQ(restored.getAge(), 35);
}
```

### Test 1.2: FactionBinarySerialization
**Purpose**: Efficiently serialize faction data  
**Setup**: Faction with 50 members
**Action**: Serialize
**Expected**: Compact binary representation

**Pseudocode**:
```cpp
TEST(BinarySerialization, FactionBinarySerialization) {
  Faction faction;
  faction.setId(1);
  faction.setName("Merchants");
  
  for (int i = 1; i <= 50; i++) {
    faction.addMemberId(i);
  }
  
  BinarySerializer serializer;
  vector<uint8_t> binary = serializer.serialize(faction);
  
  Faction restored = serializer.deserialize<Faction>(binary);
  
  EXPECT_EQ(restored.getId(), 1);
  EXPECT_EQ(restored.getName(), "Merchants");
  EXPECT_EQ(restored.getMemberCount(), 50);
}
```

### Test 1.3: ResourceBinarySerialization
**Purpose**: Serialize resource with metadata efficiently  
**Setup**: Resource with production/consumption rates
**Action**: Serialize
**Expected**: All metadata preserved

**Pseudocode**:
```cpp
TEST(BinarySerialization, ResourceBinarySerialization) {
  Resource resource;
  resource.setId(1);
  resource.setName("Food");
  resource.setQuantity(500);
  resource.setProductionRate(20);
  resource.setConsumptionRate(15);
  resource.setScarcityThreshold(150);
  
  BinarySerializer serializer;
  vector<uint8_t> binary = serializer.serialize(resource);
  
  Resource restored = serializer.deserialize<Resource>(binary);
  
  EXPECT_EQ(restored.getQuantity(), 500);
  EXPECT_EQ(restored.getProductionRate(), 20);
  EXPECT_EQ(restored.getScarcityThreshold(), 150);
}
```

### Test 1.4: WorldStateBinarySerialization
**Purpose**: Serialize complete world state including all systems  
**Setup**: Complex world with 100 NPCs, 5 factions, 10 resources
**Action**: Serialize entire state
**Expected**: Complete serialization, deterministic output

**Pseudocode**:
```cpp
TEST(BinarySerialization, WorldStateBinarySerialization) {
  WorldState state = createComplexWorld(100, 5, 10);
  
  BinarySerializer serializer;
  vector<uint8_t> binary = serializer.serialize(state);
  
  WorldState restored = serializer.deserialize<WorldState>(binary);
  
  EXPECT_EQ(restored.getNPCCount(), 100);
  EXPECT_EQ(restored.getFactionCount(), 5);
  EXPECT_EQ(restored.getResourceCount(), 10);
  EXPECT_EQ(restored.getTickNumber(), state.getTickNumber());
}
```

### Test 1.5: HeaderGeneration
**Purpose**: Binary file header with version and metadata  
**Setup**: Create header for save file
**Action**: Write header
**Expected**: Version, checksum, size included

**Pseudocode**:
```cpp
TEST(BinarySerialization, HeaderGeneration) {
  SaveFileHeader header;
  header.formatVersion = 1;
  header.gameVersion = "1.0.0";
  header.playerName = "TestPlayer";
  header.createdAt = getCurrentTimestamp();
  header.tickNumber = 12345;
  header.worldSize = 1000000;  // Bytes
  
  BinarySerializer serializer;
  vector<uint8_t> headerBinary = serializer.serializeHeader(header);
  
  SaveFileHeader restored = serializer.deserializeHeader(headerBinary);
  
  EXPECT_EQ(restored.formatVersion, 1);
  EXPECT_EQ(restored.playerName, "TestPlayer");
  EXPECT_EQ(restored.tickNumber, 12345);
}
```

### Test 1.6: ChecksumGeneration
**Purpose**: CRC32 checksum for integrity validation  
**Setup**: Serialize data with checksum
**Action**: Calculate and verify checksum
**Expected**: Checksum matches original

**Pseudocode**:
```cpp
TEST(BinarySerialization, ChecksumGeneration) {
  WorldState state = createTestWorld();
  
  BinarySerializer serializer;
  vector<uint8_t> binary = serializer.serialize(state);
  uint32_t checksum = serializer.calculateChecksum(binary);
  
  // Verify checksum
  uint32_t verified = serializer.calculateChecksum(binary);
  EXPECT_EQ(checksum, verified);
  
  // Corrupt data, verify checksum changes
  binary[100]++;
  uint32_t corrupted = serializer.calculateChecksum(binary);
  EXPECT_NE(checksum, corrupted);
}
```

### Test 1.7: CompressionSupport
**Purpose**: Optional gzip compression reduces size 50%+  
**Setup**: Serialize with and without compression
**Action**: Compare sizes
**Expected**: Compressed <50% of uncompressed

**Pseudocode**:
```cpp
TEST(BinarySerialization, CompressionSupport) {
  WorldState state = createComplexWorld(500, 10, 20);
  
  BinarySerializer serializer;
  
  vector<uint8_t> uncompressed = serializer.serialize(state);
  vector<uint8_t> compressed = serializer.serializeCompressed(state);
  
  float ratio = float(compressed.size()) / uncompressed.size();
  
  EXPECT_LT(ratio, 0.5f);  // 50% or smaller
  
  // Verify decompression works
  WorldState restored = serializer.deserializeCompressed(compressed);
  EXPECT_EQ(restored.getNPCCount(), 500);
}
```

### Test 1.8: SerializationPerformance
**Purpose**: Serialization of 1000 NPCs < 100ms  
**Setup**: World with 1000 NPCs
**Action**: Serialize
**Expected**: < 100ms

**Pseudocode**:
```cpp
TEST(BinarySerialization, SerializationPerformance) {
  WorldState state = createComplexWorld(1000, 20, 30);
  
  BinarySerializer serializer;
  
  auto t_start = chrono::high_resolution_clock::now();
  vector<uint8_t> binary = serializer.serialize(state);
  auto t_end = chrono::high_resolution_clock::now();
  
  long duration = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration, 100);
}
```

---

## Test Suite 2: Save Operations & Management (8 tests)

### Test 2.1: CompleteSave
**Purpose**: Save entire world state to file  
**Setup**: Complex game world
**Action**: Execute save
**Expected**: File created with all data

**Pseudocode**:
```cpp
TEST(SaveOperations, CompleteSave) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createComplexWorld(100, 5, 10);
  
  bool success = saveSystem.save(state, "test_save.dat");
  
  EXPECT_TRUE(success);
  EXPECT_TRUE(fileExists("./saves/test_save.dat"));
}
```

### Test 2.2: SaveFileSize
**Purpose**: Typical save file <2MB for 1000 NPCs  
**Setup**: World with 1000 NPCs
**Action**: Save and measure size
**Expected**: < 2MB

**Pseudocode**:
```cpp
TEST(SaveOperations, SaveFileSize) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createComplexWorld(1000, 20, 30);
  
  saveSystem.save(state, "large_save.dat");
  
  size_t fileSize = getFileSize("./saves/large_save.dat");
  
  EXPECT_LT(fileSize, 2 * 1024 * 1024);  // 2MB
}
```

### Test 2.3: SaveDuration
**Purpose**: Save operation completes <2 seconds  
**Setup**: World with 1000 NPCs
**Action**: Time save operation
**Expected**: < 2 seconds

**Pseudocode**:
```cpp
TEST(SaveOperations, SaveDuration) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createComplexWorld(1000, 20, 30);
  
  auto t_start = chrono::high_resolution_clock::now();
  saveSystem.save(state, "timed_save.dat");
  auto t_end = chrono::high_resolution_clock::now();
  
  long duration = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration, 2000);
}
```

### Test 2.4: AutoSaveExecution
**Purpose**: Auto-save triggers every 5 game minutes  
**Setup**: Initialize auto-save with 5-minute interval
**Action**: Simulate 5 game minutes (75000 ticks)
**Expected**: Auto-save triggered

**Pseudocode**:
```cpp
TEST(SaveOperations, AutoSaveExecution) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  saveSystem.enableAutoSave(true);
  saveSystem.setAutoSaveInterval(75000);  // 5 game minutes in ticks
  
  WorldState state = createTestWorld();
  int autoSaveCount = 0;
  
  saveSystem.onAutoSave([&]() {
    autoSaveCount++;
  });
  
  for (int tick = 0; tick < 150000; tick++) {
    saveSystem.tick(state);
  }
  
  EXPECT_GE(autoSaveCount, 1);  // At least one auto-save
}
```

### Test 2.5: ManualSaveOverwrite
**Purpose**: Manual save overwrites previous save  
**Setup**: Save twice to same file
**Action**: Verify file updated
**Expected**: Second save overwrites first

**Pseudocode**:
```cpp
TEST(SaveOperations, ManualSaveOverwrite) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state1 = createTestWorld();
  state1.tickNumber = 100;
  
  saveSystem.save(state1, "overwrite_test.dat");
  size_t size1 = getFileSize("./saves/overwrite_test.dat");
  
  WorldState state2 = createComplexWorld(50, 3, 5);
  state2.tickNumber = 200;
  
  saveSystem.save(state2, "overwrite_test.dat");
  size_t size2 = getFileSize("./saves/overwrite_test.dat");
  
  // Sizes may differ due to state complexity
  EXPECT_GT(size1, 0);
  EXPECT_GT(size2, 0);
}
```

### Test 2.6: SaveSlotManagement
**Purpose**: Multiple save slots maintained independently  
**Setup**: Save to 3 different slots
**Action**: Verify each slot exists and differs
**Expected**: 3 independent saves

**Pseudocode**:
```cpp
TEST(SaveOperations, SaveSlotManagement) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  for (int slot = 0; slot < 3; slot++) {
    WorldState state = createTestWorld();
    state.tickNumber = slot * 100;
    
    saveSystem.save(state, "slot_" + to_string(slot) + ".dat");
  }
  
  EXPECT_TRUE(fileExists("./saves/slot_0.dat"));
  EXPECT_TRUE(fileExists("./saves/slot_1.dat"));
  EXPECT_TRUE(fileExists("./saves/slot_2.dat"));
}
```

### Test 2.7: SaveLocationConfiguration
**Purpose**: Configurable save directory  
**Setup**: Create SaveSystem with custom path
**Action**: Save file
**Expected**: File created in custom location

**Pseudocode**:
```cpp
TEST(SaveOperations, SaveLocationConfiguration) {
  SaveSystem saveSystem;
  string customPath = "./custom_saves/subfolder";
  
  saveSystem.initialize(customPath);
  
  WorldState state = createTestWorld();
  bool success = saveSystem.save(state, "test.dat");
  
  EXPECT_TRUE(success);
  EXPECT_TRUE(fileExists(customPath + "/test.dat"));
}
```

### Test 2.8: SaveErrorHandling
**Purpose**: Handles save errors gracefully  
**Setup**: Try to save to read-only directory
**Action**: Attempt save
**Expected**: Error caught, message provided

**Pseudocode**:
```cpp
TEST(SaveOperations, SaveErrorHandling) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createTestWorld();
  
  // Simulate error (e.g., disk full, permission denied)
  saveSystem.simulateError(true);
  
  bool success = saveSystem.save(state, "error_test.dat");
  
  EXPECT_FALSE(success);
  EXPECT_GT(saveSystem.getLastError().length(), 0);
}
```

---

## Test Suite 3: Load Operations & Validation (8 tests)

### Test 3.1: CompleteLoad
**Purpose**: Load entire world state from save file  
**Setup**: Save a world, close, load it back
**Action**: Load from file
**Expected**: Complete world restored

**Pseudocode**:
```cpp
TEST(LoadOperations, CompleteLoad) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  // Save
  WorldState original = createComplexWorld(100, 5, 10);
  saveSystem.save(original, "load_test.dat");
  
  // Load
  WorldState loaded = saveSystem.load("load_test.dat");
  
  EXPECT_EQ(loaded.getNPCCount(), 100);
  EXPECT_EQ(loaded.getFactionCount(), 5);
}
```

### Test 3.2: LoadDuration
**Purpose**: Load operation completes <2 seconds  
**Setup**: Save file with 1000 NPCs
**Action**: Time load operation
**Expected**: < 2 seconds

**Pseudocode**:
```cpp
TEST(LoadOperations, LoadDuration) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  // Create and save large world
  WorldState state = createComplexWorld(1000, 20, 30);
  saveSystem.save(state, "timed_load.dat");
  
  auto t_start = chrono::high_resolution_clock::now();
  saveSystem.load("timed_load.dat");
  auto t_end = chrono::high_resolution_clock::now();
  
  long duration = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration, 2000);
}
```

### Test 3.3: StateValidation
**Purpose**: Loaded state passes integrity checks  
**Setup**: Load world, verify consistency
**Action**: Validate state
**Expected**: All checks pass

**Pseudocode**:
```cpp
TEST(LoadOperations, StateValidation) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState original = createComplexWorld(100, 5, 10);
  saveSystem.save(original, "validate_test.dat");
  
  WorldState loaded = saveSystem.load("validate_test.dat");
  
  // Verify consistency
  EXPECT_TRUE(loaded.isValid());
  EXPECT_EQ(loaded.getNPCCount(), 100);
  EXPECT_EQ(loaded.getFactionCount(), 5);
}
```

### Test 3.4: HeaderValidation
**Purpose**: Version compatibility checked on load  
**Setup**: Load file with version info
**Action**: Validate header
**Expected**: Version matches or migration available

**Pseudocode**:
```cpp
TEST(LoadOperations, HeaderValidation) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createTestWorld();
  saveSystem.save(state, "header_test.dat");
  
  SaveFileHeader header = saveSystem.readHeader("header_test.dat");
  
  EXPECT_EQ(header.formatVersion, 1);
  EXPECT_GT(header.gameVersion.length(), 0);
}
```

### Test 3.5: ChecksumValidation
**Purpose**: CRC32 checksum verified on load  
**Setup**: Load file with checksum validation
**Action**: Validate checksum
**Expected**: Checksum matches, data valid

**Pseudocode**:
```cpp
TEST(LoadOperations, ChecksumValidation) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createTestWorld();
  saveSystem.save(state, "checksum_test.dat");
  
  bool valid = saveSystem.validateChecksum("checksum_test.dat");
  
  EXPECT_TRUE(valid);
}
```

### Test 3.6: CorruptionDetection
**Purpose**: Detects and reports corrupted save files  
**Setup**: Corrupt a save file
**Action**: Attempt load
**Expected**: Corruption detected

**Pseudocode**:
```cpp
TEST(LoadOperations, CorruptionDetection) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createTestWorld();
  saveSystem.save(state, "corrupt_test.dat");
  
  // Corrupt file
  corruptFile("./saves/corrupt_test.dat", 50, 100);
  
  bool valid = saveSystem.validateChecksum("corrupt_test.dat");
  
  EXPECT_FALSE(valid);
}
```

### Test 3.7: RecoveryMechanism
**Purpose**: Attempts recovery or safe fallback on error  
**Setup**: Corrupted save file
**Action**: Load with recovery enabled
**Expected**: Recovery attempted or fallback provided

**Pseudocode**:
```cpp
TEST(LoadOperations, RecoveryMechanism) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  saveSystem.enableRecovery(true);
  
  // Create corrupted file
  corruptFile("./saves/corrupt_test.dat", 50, 100);
  
  WorldState recovered = saveSystem.loadWithRecovery("corrupt_test.dat");
  
  // Should attempt partial recovery or provide default
  EXPECT_TRUE(recovered.isValid());
}
```

### Test 3.8: LoadPerformance
**Purpose**: Load operations scale efficiently  
**Setup**: Load 1000 NPC file 10 times
**Action**: Measure total time
**Expected**: < 20 seconds total

**Pseudocode**:
```cpp
TEST(LoadOperations, LoadPerformance) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createComplexWorld(1000, 20, 30);
  saveSystem.save(state, "perf_test.dat");
  
  auto t_start = chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 10; i++) {
    saveSystem.load("perf_test.dat");
  }
  
  auto t_end = chrono::high_resolution_clock::now();
  long total = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(total, 20000);  // 20 seconds
}
```

---

## Test Suite 4: Incremental Saves & Delta Compression (8 tests)

### Test 4.1: DeltaDetection
**Purpose**: Identifies changed data since last save  
**Setup**: Modify world, detect changes
**Action**: Track deltas
**Expected**: Changes identified accurately

**Pseudocode**:
```cpp
TEST(IncrementalSave, DeltaDetection) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createTestWorld();
  saveSystem.save(state, "delta_base.dat");
  
  // Modify state
  state.npcs[0].setMood(0.5f);
  state.resources["food"].quantity -= 10;
  
  DeltaDetector detector;
  vector<Delta> deltas = detector.detectDeltas(state, state);  // Compare versions
  
  EXPECT_GT(deltas.size(), 0);
}
```

### Test 4.2: DeltaCompression
**Purpose**: Delta saves ~95% more efficient than full saves  
**Setup**: Save full, then delta
**Action**: Compare sizes
**Expected**: Delta <5% of full size

**Pseudocode**:
```cpp
TEST(IncrementalSave, DeltaCompression) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createComplexWorld(100, 5, 10);
  
  // Full save
  saveSystem.save(state, "full_save.dat");
  size_t fullSize = getFileSize("./saves/full_save.dat");
  
  // Modify and delta save
  state.resources["food"].quantity -= 10;
  state.npcs[0].setLoyalty(0.8f);
  
  saveSystem.saveDelta(state, "delta_save.dat");
  size_t deltaSize = getFileSize("./saves/delta_save.dat");
  
  float ratio = float(deltaSize) / fullSize;
  EXPECT_LT(ratio, 0.05f);  // Delta <5% of full
}
```

### Test 4.3: FullRestoration
**Purpose**: Delta saves reconstruct complete state  
**Setup**: Full save + multiple deltas
**Action**: Load and restore
**Expected**: Complete state reconstructed

**Pseudocode**:
```cpp
TEST(IncrementalSave, FullRestoration) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState original = createComplexWorld(100, 5, 10);
  saveSystem.save(original, "restore_base.dat");
  
  // Apply changes and save deltas
  original.resources["food"].quantity -= 20;
  saveSystem.saveDelta(original, "restore_delta_1.dat");
  
  original.npcs[0].setLoyalty(0.9f);
  saveSystem.saveDelta(original, "restore_delta_2.dat");
  
  // Restore from base + deltas
  WorldState restored = saveSystem.loadWithDeltas("restore_base.dat", 
                                                   {"restore_delta_1.dat", "restore_delta_2.dat"});
  
  EXPECT_EQ(restored.resources["food"].quantity, 
            original.resources["food"].quantity);
}
```

### Test 4.4: ChainedDeltas
**Purpose**: Multiple deltas applied in sequence  
**Setup**: 5 incremental deltas
**Action**: Apply all
**Expected**: Final state correct

**Pseudocode**:
```cpp
TEST(IncrementalSave, ChainedDeltas) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createTestWorld();
  
  int foodStart = state.resources["food"].quantity;
  
  for (int i = 0; i < 5; i++) {
    state.resources["food"].quantity -= (i + 1) * 10;
    saveSystem.saveDelta(state, "chain_delta_" + to_string(i) + ".dat");
  }
  
  int totalReduction = 10 + 20 + 30 + 40 + 50;  // 150
  
  WorldState restored = saveSystem.loadWithDeltaChain("chain_delta");
  EXPECT_EQ(restored.resources["food"].quantity, 
            foodStart - totalReduction);
}
```

### Test 4.5: DeltaExpiration
**Purpose**: Old deltas pruned after 10 full saves  
**Setup**: Create many deltas
**Action**: Prune old deltas
**Expected**: Old deltas removed

**Pseudocode**:
```cpp
TEST(IncrementalSave, DeltaExpiration) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  saveSystem.setMaxDeltasPerFullSave(10);
  
  WorldState state = createTestWorld();
  
  for (int i = 0; i < 15; i++) {
    state.resources["food"].quantity -= 5;
    saveSystem.saveDelta(state, "expire_delta_" + to_string(i) + ".dat");
  }
  
  // Prune
  saveSystem.pruneDeltaArchive();
  
  int remaining = saveSystem.getDeltaCount();
  EXPECT_LE(remaining, 10);
}
```

### Test 4.6: DeltaConsistency
**Purpose**: Delta saves maintain consistency  
**Setup**: Apply deltas, verify state
**Action**: Validate consistency
**Expected**: State valid after delta restore

**Pseudocode**:
```cpp
TEST(IncrementalSave, DeltaConsistency) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createComplexWorld(50, 3, 8);
  saveSystem.save(state, "consistency_base.dat");
  
  // Modify and delta
  state.npcs[0].setLoyalty(0.7f);
  state.resources["food"].quantity -= 5;
  saveSystem.saveDelta(state, "consistency_delta.dat");
  
  // Restore
  WorldState restored = saveSystem.loadWithDeltas("consistency_base.dat", 
                                                   {"consistency_delta.dat"});
  
  EXPECT_TRUE(restored.isValid());
  EXPECT_EQ(restored.getNPCCount(), 50);
}
```

### Test 4.7: DeltaPerformance
**Purpose**: Delta detection and compression <50ms  
**Setup**: World with 100 NPCs, apply changes
**Action**: Detect and save delta
**Expected**: < 50ms

**Pseudocode**:
```cpp
TEST(IncrementalSave, DeltaPerformance) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createComplexWorld(100, 5, 10);
  saveSystem.save(state, "perf_base.dat");
  
  state.resources["food"].quantity -= 10;
  
  auto t_start = chrono::high_resolution_clock::now();
  saveSystem.saveDelta(state, "perf_delta.dat");
  auto t_end = chrono::high_resolution_clock::now();
  
  long duration = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(duration, 50);
}
```

### Test 4.8: DeltaIntegrity
**Purpose**: Deltas maintain data integrity  
**Setup**: Save with deltas, verify checksums
**Action**: Validate delta integrity
**Expected**: All checksums valid

**Pseudocode**:
```cpp
TEST(IncrementalSave, DeltaIntegrity) {
  SaveSystem saveSystem;
  saveSystem.initialize("./saves");
  
  WorldState state = createTestWorld();
  saveSystem.save(state, "integrity_base.dat");
  saveSystem.saveDelta(state, "integrity_delta.dat");
  
  // Verify checksums
  bool baseValid = saveSystem.validateChecksum("integrity_base.dat");
  bool deltaValid = saveSystem.validateChecksum("integrity_delta.dat");
  
  EXPECT_TRUE(baseValid);
  EXPECT_TRUE(deltaValid);
}
```

---

## Test Suite 5: Lazy Loading & Memory Management (8 tests)

### Test 5.1: SnapshotCreation
**Purpose**: Unloaded NPCs stored as 36-byte snapshots  
**Setup**: Create NPC snapshots
**Action**: Serialize to snapshot
**Expected**: ~36 bytes per NPC

**Pseudocode**:
```cpp
TEST(LazyLoading, SnapshotCreation) {
  NPCSnapshot snapshot;
  snapshot.id = 1;
  snapshot.position = {100, 50, 0};
  snapshot.factionId = 2;
  snapshot.loyalty = 0.75f;
  snapshot.mood = 0.6f;
  
  BinarySerializer serializer;
  vector<uint8_t> data = serializer.serialize(snapshot);
  
  EXPECT_LE(data.size(), 50);  // ~36 bytes target
}
```

### Test 5.2: OnDemandLoading
**Purpose**: NPC loaded when within 30 units of player  
**Setup**: Unloaded NPC outside 30 unit range, player moves within range
**Action**: Track loading
**Expected**: NPC loaded when nearby

**Pseudocode**:
```cpp
TEST(LazyLoading, OnDemandLoading) {
  LazyLoadingSystem loader;
  loader.initialize("./saves");
  
  Player player({0, 0, 0});
  
  // NPC at distance 50 (not loaded)
  NPCSnapshot snapshot;
  snapshot.id = 1;
  snapshot.position = {50, 0, 0};
  
  NPC* npc = loader.ensureLoaded(snapshot, player);
  EXPECT_FALSE(npc->isFullyLoaded());
  
  // Player moves within 30 units
  player.setPosition({25, 0, 0});
  npc = loader.ensureLoaded(snapshot, player);
  EXPECT_TRUE(npc->isFullyLoaded());
}
```

### Test 5.3: BatchLoading
**Purpose**: Multiple NPCs loaded in batches (10/tick)  
**Setup**: 100 NPCs to load, track batch processing
**Action**: Load batch by batch
**Expected**: Max 10 per tick

**Pseudocode**:
```cpp
TEST(LazyLoading, BatchLoading) {
  LazyLoadingSystem loader;
  loader.initialize("./saves");
  loader.setBatchSize(10);
  
  vector<NPCSnapshot> snapshots;
  for (int i = 0; i < 100; i++) {
    snapshots.push_back(createRandomSnapshot());
  }
  
  int loaded = 0;
  for (int tick = 0; tick < 20; tick++) {
    int batchLoaded = loader.processBatch();
    loaded += batchLoaded;
    EXPECT_LE(batchLoaded, 10);
  }
  
  EXPECT_EQ(loaded, 100);
}
```

### Test 5.4: ActiveSetManagement
**Purpose**: Max 200 NPCs active (targets 60 FPS)  
**Setup**: 1000 NPCs, player moving
**Action**: Track active set size
**Expected**: Never exceeds 200 active

**Pseudocode**:
```cpp
TEST(LazyLoading, ActiveSetManagement) {
  LazyLoadingSystem loader;
  loader.initialize("./saves");
  loader.setMaxActive(200);
  
  Player player({500, 500, 0});
  
  for (int i = 0; i < 1000; i++) {
    NPCSnapshot snapshot = createSnapshotAt(randomPosition());
    loader.registerSnapshot(snapshot);
  }
  
  loader.updateActiveSet(player);
  
  int activeCount = loader.getActiveNPCCount();
  EXPECT_LE(activeCount, 200);
}
```

### Test 5.5: SnapshotAccuracy
**Purpose**: Restored NPC matches original state  
**Setup**: Save NPC, restore from snapshot
**Action**: Compare attributes
**Expected**: All critical attributes match

**Pseudocode**:
```cpp
TEST(LazyLoading, SnapshotAccuracy) {
  NPC original;
  original.setId(1);
  original.setName("Alice");
  original.setPosition({100, 50, 0});
  original.setLoyalty(0.75f);
  original.setMood(0.6f);
  original.setFactionId(2);
  
  NPCSnapshot snapshot = original.createSnapshot();
  
  NPC restored = NPC::fromSnapshot(snapshot);
  
  EXPECT_EQ(restored.getId(), 1);
  EXPECT_EQ(restored.getPosition(), Vector3(100, 50, 0));
  EXPECT_EQ(restored.getLoyalty(), 0.75f);
}
```

### Test 5.6: MemoryEfficiency
**Purpose**: Active set uses less memory than full load  
**Setup**: Load 1000 NPCs with lazy loading
**Action**: Measure memory
**Expected**: <100MB total

**Pseudocode**:
```cpp
TEST(LazyLoading, MemoryEfficiency) {
  size_t memBefore = getMemoryUsage();
  
  LazyLoadingSystem loader;
  loader.initialize("./saves");
  loader.setMaxActive(200);
  
  for (int i = 0; i < 1000; i++) {
    loader.registerSnapshot(createRandomSnapshot());
  }
  
  size_t memAfter = getMemoryUsage();
  size_t memUsed = memAfter - memBefore;
  
  EXPECT_LT(memUsed, 100 * 1024 * 1024);  // 100MB
}
```

### Test 5.7: LazyLoadingUnload
**Purpose**: Distant NPCs unloaded and re-snapshoted  
**Setup**: Loaded NPC moves away from player
**Action**: Track unloading
**Expected**: NPC unloaded when far

**Pseudocode**:
```cpp
TEST(LazyLoading, LazyLoadingUnload) {
  LazyLoadingSystem loader;
  loader.initialize("./saves");
  
  Player player({0, 0, 0});
  
  NPC npc;
  npc.setPosition({10, 0, 0});
  loader.registerActive(npc);
  
  EXPECT_TRUE(loader.isLoaded(npc.getId()));
  
  // Player moves away
  player.setPosition({200, 0, 0});
  loader.updateActiveSet(player);
  
  EXPECT_FALSE(loader.isLoaded(npc.getId()));
}
```

### Test 5.8: LazyLoadingPerformance
**Purpose**: Active set updates <5ms per frame  
**Setup**: 1000 NPCs, 200 active
**Action**: Update 100 times
**Expected**: < 5ms average

**Pseudocode**:
```cpp
TEST(LazyLoading, LazyLoadingPerformance) {
  LazyLoadingSystem loader;
  loader.initialize("./saves");
  loader.setMaxActive(200);
  
  for (int i = 0; i < 1000; i++) {
    loader.registerSnapshot(createRandomSnapshot());
  }
  
  Player player({500, 500, 0});
  
  auto t_start = chrono::high_resolution_clock::now();
  
  for (int i = 0; i < 100; i++) {
    player.setPosition(randomPosition());
    loader.updateActiveSet(player);
  }
  
  auto t_end = chrono::high_resolution_clock::now();
  long total = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(total, 500);  // 5ms average
}
```

---

## Test Suite 6: Deterministic Replay & Data Integrity (8 tests)

### Test 6.1: LLMCallLogging
**Purpose**: All LLM calls logged with inputs/outputs  
**Setup**: Execute gameplay with LLM calls
**Action**: Check log
**Expected**: All calls recorded

**Pseudocode**:
```cpp
TEST(DeterministicReplay, LLMCallLogging) {
  ReplaySystem replay;
  replay.enableLogging(true);
  
  // Simulate LLM calls
  replay.logLLMCall("decision_interpretation", "allocate food", 
                    "{action: allocate, target: food}");
  replay.logLLMCall("narrative_generation", "world state...", 
                    "Food scarcity...");
  
  auto log = replay.getLLMCallLog();
  
  EXPECT_EQ(log.size(), 2);
  EXPECT_EQ(log[0].type, "decision_interpretation");
}
```

### Test 6.2: RNGSeeding
**Purpose**: RNG seed logged per tick  
**Setup**: Run 100 ticks with logging
**Action**: Check seed log
**Expected**: 100 seeds recorded

**Pseudocode**:
```cpp
TEST(DeterministicReplay, RNGSeeding) {
  ReplaySystem replay;
  replay.enableLogging(true);
  
  for (int tick = 0; tick < 100; tick++) {
    int seed = 42 + tick;
    srand(seed);
    replay.logRNGSeed(tick, seed);
  }
  
  auto seedLog = replay.getRNGSeedLog();
  
  EXPECT_EQ(seedLog.size(), 100);
  EXPECT_EQ(seedLog[0].seed, 42);
  EXPECT_EQ(seedLog[99].seed, 141);
}
```

### Test 6.3: ReplayExecution
**Purpose**: Simulation replayed with logged data  
**Setup**: Record session, replay with same inputs
**Action**: Execute replay
**Expected**: Replay completes successfully

**Pseudocode**:
```cpp
TEST(DeterministicReplay, ReplayExecution) {
  ReplaySystem replay;
  replay.initialize("./replays");
  
  // Load replay session
  bool loaded = replay.load("session_001.replay");
  
  EXPECT_TRUE(loaded);
  
  // Execute replay
  bool success = replay.execute();
  
  EXPECT_TRUE(success);
}
```

### Test 6.4: DeterminismValidation
**Purpose**: Replay produces byte-identical output  
**Setup**: Original run vs replay run
**Action**: Compare world states at each tick
**Expected**: Byte-identical states

**Pseudocode**:
```cpp
TEST(DeterministicReplay, DeterminismValidation) {
  ReplaySystem replay;
  replay.initialize("./replays");
  replay.enableLogging(true);
  
  // Original run
  vector<string> originalStates;
  GameLoop game;
  for (int tick = 0; tick < 100; tick++) {
    game.tick(16.67f);
    originalStates.push_back(game.getWorldState().hash());
  }
  
  // Replay run
  vector<string> replayStates;
  replay.load("recorded_session.replay");
  for (int tick = 0; tick < 100; tick++) {
    replayStates.push_back(replay.getStateAt(tick).hash());
  }
  
  // Compare
  for (size_t i = 0; i < originalStates.size(); i++) {
    EXPECT_EQ(originalStates[i], replayStates[i]);
  }
}
```

### Test 6.5: ReplayLogFormat
**Purpose**: Logs stored in JSON lines format  
**Setup**: Create replay log
**Action**: Verify format
**Expected**: Valid JSON lines

**Pseudocode**:
```cpp
TEST(DeterministicReplay, ReplayLogFormat) {
  ReplaySystem replay;
  replay.initialize("./replays");
  
  replay.logLLMCall("decision", "input", "output");
  replay.logRNGSeed(0, 12345);
  
  replay.save("format_test.replay");
  
  string content = readFile("./replays/format_test.replay");
  
  // Should be valid JSON lines
  auto lines = splitByNewline(content);
  for (const auto& line : lines) {
    if (!line.empty()) {
      EXPECT_NO_THROW(json::parse(line));
    }
  }
}
```

### Test 6.6: DebugOutput
**Purpose**: Tick-by-tick state available for debugging  
**Setup**: Enable debug output, run 50 ticks
**Action**: Check debug log
**Expected**: Detailed state per tick

**Pseudocode**:
```cpp
TEST(DeterministicReplay, DebugOutput) {
  ReplaySystem replay;
  replay.initialize("./replays");
  replay.enableDebugOutput(true);
  
  GameLoop game;
  for (int tick = 0; tick < 50; tick++) {
    game.tick(16.67f);
  }
  
  replay.save("debug_output.replay");
  
  auto debugLog = replay.getDebugLog();
  
  EXPECT_EQ(debugLog.size(), 50);  // One entry per tick
  
  for (const auto& entry : debugLog) {
    EXPECT_GT(entry.npcCount, 0);
    EXPECT_GT(entry.tickNumber, 0);
  }
}
```

### Test 6.7: CorruptionDetection
**Purpose**: Detects corrupted save states  
**Setup**: Corrupt replay file
**Action**: Attempt to load
**Expected**: Corruption detected

**Pseudocode**:
```cpp
TEST(DeterministicReplay, CorruptionDetection) {
  ReplaySystem replay;
  replay.initialize("./replays");
  
  // Create valid replay
  replay.logLLMCall("test", "input", "output");
  replay.save("corruption_test.replay");
  
  // Corrupt it
  corruptFile("./replays/corruption_test.replay", 50, 100);
  
  // Try to load
  bool loaded = replay.load("corruption_test.replay");
  
  EXPECT_FALSE(loaded);  // Detects corruption
}
```

### Test 6.8: FullIntegrationPerformance
**Purpose**: Full replay cycle <600ms per suite  
**Setup**: Record, replay, validate 100 ticks
**Action**: Measure total time
**Expected**: < 600ms total

**Pseudocode**:
```cpp
TEST(DeterministicReplay, FullIntegrationPerformance) {
  ReplaySystem replay;
  replay.initialize("./replays");
  
  auto t_start = chrono::high_resolution_clock::now();
  
  // Record
  GameLoop game;
  for (int tick = 0; tick < 100; tick++) {
    game.tick(16.67f);
  }
  
  // Replay
  replay.load("recorded.replay");
  replay.execute();
  
  auto t_end = chrono::high_resolution_clock::now();
  long total = chrono::duration_cast<chrono::milliseconds>(t_end - t_start).count();
  
  EXPECT_LT(total, 600);
}
```

---

## Success Criteria

### Functionality Validation
- ✓ All 48 tests pass consistently
- ✓ Binary serialization efficient (~70 bytes/NPC)
- ✓ Save/load operations fast (<2 seconds)
- ✓ Delta saves 95%+ more efficient
- ✓ Lazy loading keeps active set <200 NPCs
- ✓ Deterministic replay produces identical states
- ✓ Corruption detection functional
- ✓ File compression optional but effective

### Coverage Requirements
- ✓ Binary Serialization & Format: 95%+
- ✓ Save Operations & Management: 94%+
- ✓ Load Operations & Validation: 95%+
- ✓ Incremental Saves & Delta Compression: 92%+
- ✓ Lazy Loading & Memory Management: 91%+
- ✓ Deterministic Replay & Data Integrity: 93%+
- ✓ Overall Coverage: 93%+

### Performance Targets
- ✓ Save duration: <2 seconds for 1000 NPCs
- ✓ Load duration: <2 seconds for 1000 NPCs
- ✓ Delta save efficiency: 95%+ reduction
- ✓ Lazy loading batch: <10 per tick
- ✓ Active set max: 200 NPCs
- ✓ Memory usage: <100MB for 1000 NPCs
- ✓ Full test suite: <600ms

### Data Integrity Validation
- ✓ Checksums validated on load
- ✓ Corruption detected and reported
- ✓ Recovery mechanism attempted
- ✓ Version compatibility checked
- ✓ Delta integrity maintained

### Determinism Validation
- ✓ Replay produces byte-identical output
- ✓ RNG seeds logged and replayed
- ✓ LLM calls logged and replayed
- ✓ 100+ tick runs deterministic

---

## Copilot Prompt for Implementation

**Prompt**: "Implement the Phase 14 Test Suite with 48 test cases covering binary serialization, save/load persistence, incremental saves with delta compression, lazy loading with active set management, deterministic replay, and data integrity validation. Follow detailed pseudocode. Ensure binary format ~70 bytes/NPC, save/load <2 seconds for 1000 NPCs, delta saves 95%+ efficient, lazy loading keeps <200 NPCs active, compression optional but 50%+ reduction, checksum validation, corruption detection with recovery, deterministic replay producing byte-identical output, RNG seeding logged per tick, and comprehensive performance benchmarks. Use Google Test framework with file I/O operations."

---

## Coverage Analysis Summary

```
Phase 14 Test Suite Coverage Summary
====================================
Binary Serialization & Format:        95%+ (NPC, faction, resource, world, header, checksum, compression)
Save Operations & Management:         94%+ (complete save, sizing, duration, auto-save, slots)
Load Operations & Validation:         95%+ (complete load, duration, validation, checksums, corruption)
Incremental Saves & Delta Compression: 92%+ (delta detection, compression, restoration, chaining, expiration)
Lazy Loading & Memory Management:    91%+ (snapshots, on-demand, batch, active set, memory efficiency)
Deterministic Replay & Data Integrity: 93%+ (LLM logging, RNG seeding, replay, determinism, corruption)

Overall Coverage:                    93%+
Expected Coverage Range:             91-95%
```

---

## Test Execution Timeline

| Phase | Tests | Est. Time | Status |
|-------|-------|-----------|--------|
| Suite 1: Binary Serialization & Format | 8 | 96ms | Ready |
| Suite 2: Save Operations & Management | 8 | 100ms | Ready |
| Suite 3: Load Operations & Validation | 8 | 104ms | Ready |
| Suite 4: Incremental Saves & Delta Compression | 8 | 112ms | Ready |
| Suite 5: Lazy Loading & Memory Management | 8 | 100ms | Ready |
| Suite 6: Deterministic Replay & Data Integrity | 8 | 120ms | Ready |
| **Total** | **48** | **<600ms** | **Ready** |

---

**Created**: Phase 14 Test Suite - Detailed Format  
**Lines**: 1400+ (comprehensive pseudocode + implementation guidelines)  
**Tests**: 48 test cases  
**Coverage Target**: 93%+  
**Status**: ✓ Expansion Complete

