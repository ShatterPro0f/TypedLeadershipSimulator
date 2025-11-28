# Phase 13: NPC-to-NPC Ambient Dialogue & Event-Driven Narratives

**Status**: Planning & Test Suite Development
**Phase 12 Foundation**: Event-driven main loop, input queue, proximity dialogue ✅

---

## Phase 13 Overview

Phase 13 focuses on creating a living, breathing settlement through ambient NPC conversations and event-driven narrative generation. While Phase 12 established the framework for player-facing dialogue, Phase 13 brings the world alive with organic NPC-to-NPC interactions.

### Core Objectives

1. **Ambient NPC Conversations** - NPCs naturally converse when nearby
2. **Event-Driven Narratives** - LLM generates emergent story issues  
3. **Narrative Issue Queue** - Player encounters issues from conversations
4. **Dialogue Quality Control** - Ensure contextual, coherent conversations
5. **Gossip Propagation** - Information spreads through population
6. **World Flavor** - Ambient dialogue creates immersive settlement

---

## Architecture: Ambient Dialogue System

### Design Principles

**Continuous, Not Scheduled**:
- NPCs initiate conversations when conditions met (not on timer)
- Conditions: proximity (<15 units) + compatible activity + cooldown expired

**Non-Blocking**:
- LLM calls async (doesn't block main loop)
- Dialogue generation in background
- UI updates when ready (no waiting)

**Deterministic**:
- All generation seeded by tick + NPC pair ID
- Same seed produces identical conversations
- Replay system logs all dialogues

**Lightweight Context**:
- Only pass essential NPC attributes (name, role, mood, recent event)
- Avoid passing full NPC/faction/resource state
- Reduces LLM token usage (~200 tokens vs 500+)

### Main Loop Integration (Phase 13)

```cpp
void SimulationManager::tick(float deltaTime)
{
    // ... existing Phase 12 systems ...
    
    // NEW: Phase 13 - Ambient NPC conversations
    if (shouldGenerateAmbientConversation())  // Every 10-30 ticks
    {
        selectNPCPair();
        generateAmbientDialogue();  // [Async] Non-blocking LLM call
    }
    
    // NEW: Phase 13 - Conversation cascading
    checkForConversationCascades();  // Does dialogue trigger faction conflict?
    propagateGossip();               // Spread rumors through population
    
    // ... rest of systems ...
}
```

### NPC Pair Selection Algorithm

```
Ambient Conversation Trigger:
  1. Find all NPC pairs within proximity (<15 units)
  2. Filter by: not already in conversation, both idle/working
  3. Check cooldown: last conversation > 5 game minutes ago
  4. Calculate conversation weight:
     weight = proximity_bonus + mood_similarity + faction_tension + shared_goal
  5. Select highest-weight pair (weighted random)
  6. Queue for LLM generation
```

---

## Implementation Details

### 1. Ambient Conversation Queue System

```cpp
struct AmbientConversationRequest {
    int npcId1;
    int npcId2;
    float weight;  // Priority for queue ordering
    int tickQueued;
    std::string location;
    std::vector<std::string> topics;  // Suggested topics based on context
};

class AmbientConversationQueue {
    void enqueueConversation(AmbientConversationRequest request);
    AmbientConversationRequest dequeue();
    void prioritizeQueue();
    int getQueueSize() const;
};
```

### 2. LLM Prompt Template (Lightweight)

```
You are generating a brief, natural conversation between two NPCs in a medieval settlement.

NPC 1: {name}, {role}, mood={mood}
NPC 2: {name}, {role}, mood={mood}
Location: {location} ({activity_description})
Recent event: {shared_context}

Generate a 2-3 line natural dialogue. Format:
NPC1: "[dialogue]"
NPC2: "[response]"

Keep responses concise (under 15 words each), contextual, and realistic for their roles.
```

### 3. Dialogue Quality Validation

```cpp
struct DialogueQualityScore {
    float grammarScore;       // 0-1: proper English
    float relevanceScore;     // 0-1: contextual relevance
    float characterScore;     // 0-1: character consistency
    float engagementScore;    // 0-1: interesting/narrative value
    float overallScore;       // weighted average
};

DialogueQualityScore validateDialogue(const std::string& npc1Line, 
                                      const std::string& npc2Line,
                                      const NPC& npc1,
                                      const NPC& npc2);

// Acceptance threshold: 0.65+
// Rejection action: retry or use template fallback
```

### 4. Event Cascading from Conversations

```
Conversation detected as important:
  - Mood delta between NPCs > 0.3 (argument)
  - Loyalty mention toward faction
  - Gossip about player
  - Discovered problem/opportunity

Cascade triggers:
  - Record in dialogue history
  - Update NPC relationship scores
  - Propagate gossip to nearby NPCs
  - Flag for potential player narrative issue
  - Recalculate faction tension
```

### 5. Gossip Propagation System

```cpp
struct Rumor {
    int originNpcId;
    std::string rumor;
    float credibility;        // 0-1 based on source
    int propagationDistance;  // How far it spreads
    int tickCreated;
    std::vector<int> heardBy; // Track who knows
};

void propagateGossip(const Rumor& rumor);
// For each NPC within earshot:
//   probability = credibility * (1 - heardBy.contains(npc))
//   if random() < probability:
//     add to npc's known rumors
//     potentially trigger dialogue about rumor
```

---

## Test Suite: Phase 13Tests (48 tests, 6 suites)

### Test Suite Structure

```
Phase13Tests (48 tests)
├─ AmbientConversationGeneration (8 tests)
│  ├─ QueueInitialization
│  ├─ NPCPairSelection
│  ├─ ConversationWeighting
│  ├─ MultipleConversationsInProgress
│  ├─ ConversationQueueOverflow
│  ├─ ConversationQueuePriority
│  ├─ NonBlockingQueueing
│  └─ LLMAsync NonBlocking
│
├─ DialogueQualityControl (8 tests)
│  ├─ GrammarValidation
│  ├─ RelevanceChecking
│  ├─ CharacterConsistency
│  ├─ LengthValidation
│  ├─ ContextAwareness
│  ├─ RejectionHandling
│  ├─ TemplateFallback
│  └─ QualityScoreAccuracy
│
├─ EventCascading (8 tests)
│  ├─ ArgumentDetection
│  ├─ FactionalTensionIncrease
│  ├─ RelationshipUpdates
│  ├─ MultiLevelCascade
│  ├─ CascadeProbability
│  ├─ CascadeTermination
│  ├─ NPCStateChanges
│  └─ FactionDynamics
│
├─ GossipPropagation (8 tests)
│  ├─ RumorCreation
│  ├─ ProximitySpread
│  ├─ CredibilityFading
│  ├─ PopulationReachCap
│  ├─ RumorChaining
│  ├─ IncorrectPropagation
│  ├─ AmbientDialogueTrigger
│  └─ GossipHistoryTracking
│
├─ SystemIntegration (8 tests)
│  ├─ ConversationContinuity
│  ├─ MultipleSystemsInteraction
│  ├─ EventToNarrativeFlow
│  ├─ DialogueHistoryTracking
│  ├─ RegistryAccess
│  ├─ LLMCallbackHandling
│  ├─ FullAmbientLoop
│  └─ SetupTeardown
│
└─ PerformanceEdgeCases (8 tests)
   ├─ ZeroAmbientConversations
   ├─ LargeNPCPopulation
   ├─ RapidConversationGeneration
   ├─ LLMTimeouts
   ├─ QueueSaturation
   ├─ ConcurrentGeneration
   ├─ MemoryStability
   └─ DeterminismUnderLoad
```

---

## Acceptance Criteria

### Ambient Conversation System
- ✅ NPCs initiate conversations when within 15 units + idle/working
- ✅ Conversations generated non-blocking (async LLM calls)
- ✅ Conversation queue prioritized by weight (proximity, mood, tension)
- ✅ Quality control rejects <65% score conversations
- ✅ Template fallback used on repeated failures
- ✅ All 8 AmbientConversationGeneration tests pass

### Dialogue Quality Control
- ✅ Grammar validated (basic English correctness)
- ✅ Relevance checked (mentions location, NPC names, context)
- ✅ Character consistency verified (dialogue matches NPC role)
- ✅ Length constraints enforced (<20 words per line)
- ✅ All 8 DialogueQualityControl tests pass

### Event Cascading
- ✅ Conversations flagged as important trigger cascades
- ✅ Faction tension increases on conflicts
- ✅ NPC relationships updated from conversation
- ✅ Multi-level cascades prevent runaway reactions
- ✅ All 8 EventCascading tests pass

### Gossip System
- ✅ Rumors spread to nearby NPCs
- ✅ Credibility fades over time/distance
- ✅ Population reach capped (~50% settlement)
- ✅ Rumors can be chained (rumor about rumor)
- ✅ All 8 GossipPropagation tests pass

### System Integration
- ✅ Ambient conversations integrate with main loop
- ✅ LLM callbacks update NPC state
- ✅ Dialogue history tracked for context
- ✅ Event cascading triggered properly
- ✅ All 8 SystemIntegration tests pass

### Performance
- ✅ 48 tests execute <2 seconds total
- ✅ No memory leaks (10K+ conversations)
- ✅ LLM timeouts handled gracefully
- ✅ All 8 PerformanceEdgeCases tests pass

---

## Key Files to Create/Modify

### New Files
- `src/phase13/AmbientConversationSystem.h` - Core system
- `src/phase13/AmbientConversationSystem.cpp` - Implementation
- `src/phase13/DialogueQualityValidator.h` - Quality control
- `src/phase13/DialogueQualityValidator.cpp` - Validation logic
- `src/phase13/GossipSystem.h` - Rumor propagation
- `src/phase13/GossipSystem.cpp` - Gossip implementation
- `tests/Phase13Tests.cpp` - 48 comprehensive tests

### Modified Files
- `include/SimulationManager.h` - Add Phase 13 methods
- `src/phase4/SimulationManager.cpp` - Integrate ambient conversations into tick()

---

## Implementation Roadmap

### Phase 13a: Ambient Conversation System (Week 1)
1. Create AmbientConversationQueue class
2. Implement NPC pair selection algorithm
3. Wire LLM calls for dialogue generation
4. Integrate into main tick loop (non-blocking)
5. **Tests**: AmbientConversationGeneration (8 tests)

### Phase 13b: Dialogue Quality Control (Week 1-2)
1. Create DialogueQualityValidator class
2. Implement grammar/relevance/character checks
3. Add template fallback system
4. Configure rejection thresholds
5. **Tests**: DialogueQualityControl (8 tests)

### Phase 13c: Event Cascading (Week 2)
1. Detect important conversation types
2. Trigger faction tension updates
3. Update NPC relationships
4. Implement multi-level cascade prevention
5. **Tests**: EventCascading (8 tests)

### Phase 13d: Gossip System (Week 2-3)
1. Create Rumor data structure
2. Implement proximity-based propagation
3. Add credibility decay
4. Wire to ambient conversations
5. **Tests**: GossipPropagation (8 tests)

### Phase 13e: Integration Testing (Week 3)
1. System integration tests
2. Performance benchmarks
3. Edge case validation
4. Regression testing (Phase 11 + Phase 12)
5. **Tests**: SystemIntegration (8) + PerformanceEdgeCases (8)

---

## Success Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Tests Passing | 48/48 | Phase13Tests execution |
| LLM Call Success | >90% | Callbacks executed properly |
| Dialogue Quality | >75% avg | Quality validator scores |
| Gossip Reach | 40-60% | Population % who hear rumor |
| Frame Stability | 60 FPS | No drops during conversations |
| Async Latency | <50ms | LLM call queuing latency |
| Memory Usage | <20MB delta | Ambient system overhead |
| Determinism | 100% | Same seed = same dialogues |

---

## Integration with Existing Systems

### Phase 12 (Event-Driven Loop)
- Phase 13 uses input queue pattern
- Follows non-blocking async pattern
- Integrates into main tick() method
- Uses same world state snapshot system

### Phase 11 (3D Pathfinding)
- NPCs pathfind to conversation location
- Formation movement when conversing
- Collision avoidance during dialogue

### Phase 10 (Dialogue System)
- Reuses dialogue formatting
- Extends with ambient mode
- Uses same NPC state systems

### Future Phase 14+ (Other Systems)
- Ambient conversations can trigger events
- Events can affect resource allocation
- Faction conversations affect diplomacy
- Gossip impacts player reputation

---

## Risk Mitigation

| Risk | Impact | Mitigation |
|------|--------|-----------|
| LLM quality | Story coherence | Quality validator + templates |
| Performance | Frame drops | Async queueing, token limits |
| Determinism | Replay issues | Seed all RNG by tick + pair |
| Memory | Long sessions | Circular dialogue buffer (100 max) |
| Feedback loop | Runaway cascades | Debouncing + cascade caps |

---

## Next Steps

1. ✅ Create Phase13plan.md (this document)
2. ➡️ Create Phase13Tests.cpp (48 tests)
3. ➡️ Implement AmbientConversationSystem
4. ➡️ Implement DialogueQualityValidator
5. ➡️ Implement GossipSystem
6. ➡️ Run full test suite (Phase 11 + Phase 12 + Phase 13)
7. ➡️ Performance benchmarking
8. ➡️ Phase 13 completion summary

---

*Phase 13 Plan - Ready for Test Suite Creation*
