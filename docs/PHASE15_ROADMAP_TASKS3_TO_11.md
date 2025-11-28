# Phase 15 Implementation Roadmap - Tasks #3-11

## Completed ✅
- **Task #1**: Save/Load System (Binary serialization, 60 lines)
- **Task #2**: NPC Problem System (Dialogue, escalation, 335 lines)

## Current State: 2/11 Tasks Complete (18%)

---

## Task #3: Event Cascading ⏱ (HIGH PRIORITY)
**Complexity**: MEDIUM | **Time**: 6-8 hours | **Impact**: HIGH  
**Why**: Enables crisis gameplay, multi-stage consequences

### What Needs Implementation
1. **Cascade Probability Formula**
   - Formula: `P_cascade = sigmoid(impact_level × trigger_factor)`
   - Range: [0, 1]
   - Higher impact events = higher cascade probability

2. **3-Stage Cascade Model**
   - Stage 1: Deterministic primary event (always happens)
   - Stage 2: Probabilistic secondary event (may cascade)
   - Stage 3: LLM narrative generates story cohesion

3. **Event Chain Storage**
   - Track primary → secondary → tertiary chain
   - Store cascade history for debugging

### Example Cascade Chain
```
Food Scarcity (primary, deterministic)
    ↓ (80% chance cascade)
Immigration Crisis (secondary, probabilistic)
    ↓ (40% chance cascade)
Faction Rebellion (tertiary, probabilistic)
    → LLM: "Desperate times breed dissent..."
```

### Implementation Files
- `include/Event.h`: Add cascade fields
- `src/core/Event.cpp`: Implement cascade methods
- `src/main.cpp`: Check cascades in `triggerRandomEvent()`

### Code Pattern
```cpp
struct EventCascade {
    Event primary;
    vector<Event> secondary;
    vector<Event> tertiary;
    vector<string> cascadeChain;
    float cascadeProbability;
};

float calculateCascadeProbability(Event event) {
    return sigmoid(event.impactLevel * 0.15f);  // Tune multiplier
}

void triggerCascade(Event primary) {
    // Roll for secondary
    if (randomFloat() < calculateCascadeProbability(primary)) {
        Event secondary = selectSecondaryEvent(primary);
        applyEvent(secondary);
        
        // Roll for tertiary
        if (randomFloat() < calculateCascadeProbability(secondary)) {
            Event tertiary = selectTertiaryEvent(secondary);
            applyEvent(tertiary);
        }
    }
}
```

---

## Task #4: LLM Error Recovery & Retry Logic 🔄 (MEDIUM PRIORITY)
**Complexity**: MEDIUM | **Time**: 5-7 hours | **Impact**: MEDIUM  
**Why**: Reliability, prevents game freezes on LLM timeout

### What Needs Implementation
1. **Exponential Backoff**
   - Retry 1: 1 second
   - Retry 2: 2 seconds
   - Retry 3: 4 seconds
   - Retry 4: 8 seconds

2. **Error Categorization**
   - Retryable: Network timeout, temporary error
   - Non-retryable: Invalid input, auth failure

3. **Timeout Enforcement**
   - Decision interpretation: 3s max
   - World state narrative: 10s max
   - Ambient conversation: 5s max

4. **Fallback Strategy**
   - After 3 retries: use rule-based fallback
   - Generate template response based on context
   - Log all failures for debugging

### Implementation Files
- `include/LLMErrorRecovery.h`: Retry logic
- `src/core/LLMErrorRecovery.cpp`: Implementation
- `src/core/OllamaProvider.cpp`: Integration

---

## Task #5: LLM Response Caching 💾 (LOW PRIORITY)
**Complexity**: MEDIUM | **Time**: 4-6 hours | **Impact**: MEDIUM  
**Why**: Reduce API costs, improve responsiveness

### What Needs Implementation
1. **Cache Key Generation**
   - Hash prompt + context
   - 128-bit hash for key

2. **TTL Management**
   - Decision interpretation: 2 minute TTL
   - World state narratives: 10 minute TTL
   - Ambient conversations: 5 minute TTL

3. **Cache Eviction**
   - LRU (Least Recently Used) policy
   - Max 1000 entries
   - Overflow: remove oldest

4. **Offline Fallback**
   - Template-based responses
   - No hallucination risk
   - Deterministic output

### Implementation Files
- `include/LLMResponseCache.h`: Cache interface
- `src/core/LLMResponseCache.cpp`: Implementation
- `src/core/DecisionInterpreter.cpp`: Integration

---

## Task #6: Proximity-Based NPC Dialogue ⏰ (HIGH PRIORITY)
**Complexity**: MEDIUM | **Time**: 6-8 hours | **Impact**: HIGH  
**Why**: Essential for UX, queue management critical

### What Needs Implementation
1. **Multi-NPC Queue**
   - Max 5 NPCs queued
   - Display: "Next in queue: Alice, then Bob"
   - Overflow handling (drop lowest priority)

2. **Priority Sorting**
   - `severity (w=0.4) + influence (w=0.3) + distance (w=0.15) + time (w=0.15)`
   - Formula: `priority = 0.4*sev + 0.3*inf + 0.15*dist + 0.15*time`

3. **Dialogue State Transitions**
   - PATHFINDING → IN_DIALOGUE → ACKNOWLEDGED → RESOLVED
   - Track NPC position for proximity
   - Trigger at < 5 units distance

4. **Cooldown Enforcement**
   - Per-NPC: 1 day minimum before re-initiating
   - Global: 2-5 second delay between NPCs

### Implementation Files
- `include/ProximityDialogueQueue.h`: Queue system
- `src/core/ProximityDialogueQueue.cpp`: Implementation
- `src/main.cpp`: Position tracking, queue processing

---

## Task #7: NPC Dialogue State Machine 🔄 (MEDIUM PRIORITY)
**Complexity**: LOW | **Time**: 3-4 hours | **Impact**: MEDIUM  
**Why**: Refines dialogue system, enables advanced responses

### What Needs Implementation
1. **5-State Machine**
   ```
   UNRESOLVED → IN_DIALOGUE → ACKNOWLEDGED → RESOLVED
            ↗ PERSISTENT ↖
   ```

2. **State-Specific Behavior**
   - UNRESOLVED: Monitor severity
   - IN_DIALOGUE: Display NPC stats, wait for input
   - ACKNOWLEDGED: Process response, evaluate outcome
   - RESOLVED: Mark complete, resume normal activity
   - PERSISTENT: Retry after cooldown

3. **Escalation in Machine**
   - Track unresolved days
   - After 5 days: escalate severity +0.1
   - Multiple escalations possible

### Implementation Files
- `include/DialogueState.h`: State machine
- `src/core/DialogueState.cpp`: Implementation
- `src/main.cpp`: State transitions

---

## Task #8: Culture & Religion Systems 🙏 (LOW PRIORITY)
**Complexity**: HIGH | **Time**: 10-12 hours | **Impact**: MEDIUM  
**Why**: World-building, adds depth and complexity

### What Needs Implementation
1. **Culture Class**
   - Norms (vector of strings)
   - Traditions (vector of strings)
   - Evolution rate (0-1)

2. **Religion Class**
   - Doctrines (vector of strings)
   - Followers (vector of NPC IDs)
   - Schism probability (0-1)

3. **Culture Evolution**
   - Per event: culture norms shift 0.05
   - Based on player decisions
   - Affects NPC affinity

4. **Religion Mechanics**
   - Follower conversion based on:
     - Event success/failure
     - NPC loyalty to faith leader
     - Cultural alignment
   - Schism probability: `P = sigmoid(doctrine_conflict × disagreement)`

### Implementation Files
- `include/CultureReligionSystem.h`: Systems
- `src/core/CultureReligionSystem.cpp`: Implementation
- `src/main.cpp`: Event integration

---

## Task #9: Ambient NPC-to-NPC Dialogue 🗨️ (MEDIUM PRIORITY)
**Complexity**: HIGH | **Time**: 8-10 hours | **Impact**: MEDIUM  
**Why**: Makes world feel alive, adds organic storytelling

### What Needs Implementation
1. **NPC Pair Discovery**
   - Proximity < 15 units
   - Both IDLE or WORKING state
   - Compatibility scoring

2. **Topic Selection**
   - Based on NPC roles, emotions, shared problems
   - Topics: work, family, gossip, concerns, opportunities

3. **LLM Dialogue Generation**
   - Lightweight context (npc1, npc2, topic)
   - Quality validation (grammar>0.7, relevance>0.7, sentiment>0.7)
   - Reject and retry if quality fails

4. **Cascade Propagation**
   - Gossip spreads to nearby NPCs
   - Bond strength calculations for future cooperation
   - Faction tension tracking

### Implementation Files
- `include/AmbientDialogueSystem.h`: System
- `src/core/AmbientDialogueSystem.cpp`: Implementation
- `src/main.cpp`: Integration, NPC pair discovery

### Target Performance
- 5-10 conversations per game minute
- Store max 100 recent conversations
- 2-3 concurrent LLM calls

---

## Task #10: LLM Request Queue & Async Processing 📊 (HIGH PRIORITY)
**Complexity**: HIGH | **Time**: 10-12 hours | **Impact**: HIGH  
**Why**: Non-blocking gameplay, essential for multiplayer-like feel

### What Needs Implementation
1. **3-Priority Queue System**
   - HIGH: Player input (3s timeout)
   - MEDIUM: World state snapshots (10s timeout)
   - LOW: Ambient conversations (5s timeout, max 3 concurrent)

2. **Request Processing**
   - Async/non-blocking
   - Callback on completion
   - Drop stale requests if superseded

3. **Retry Scheduling**
   - Exponential backoff within queue
   - Max 3 retries per request

4. **Concurrent Management**
   - Max 1 HIGH (player input)
   - Max 1 MEDIUM (world state)
   - Max 3 LOW (ambient)
   - Total: max 5 concurrent

### Implementation Files
- `include/LLMRequestQueue.h`: Queue system
- `src/core/LLMRequestQueue.cpp`: Implementation
- `src/main.cpp`: Main loop integration

### Example Queue Flow
```
Tick 1000: Player types command
  → Enqueue HIGH priority
  → Player input LLM processing

Tick 1001: World state changes detected
  → Enqueue MEDIUM priority
  → Queued (waiting for HIGH to complete)

Tick 1002: NPC pair wants dialogue
  → Enqueue LOW priority (allow 3 concurrent)
  → Start processing immediately

Tick 1003: Previous MEDIUM completes
  → Start world state LLM
  → Player sees response
```

---

## Task #11: NPC Lazy Loading & Memory Optimization 💾 (LOW PRIORITY)
**Complexity**: HIGH | **Time**: 12-15 hours | **Impact**: HIGH  
**Why**: Enables 1000+ NPC support

### What Needs Implementation
1. **Active Set Management**
   - Max 200 NPCs loaded in memory
   - Rest stored as snapshots (~50 bytes each)
   - VIP protection: leaders/advisors never unload

2. **Relevance Scoring**
   - Formula: `w_dist*dist + w_event*event + w_influence*influence`
   - Weights: 0.5, 0.3, 0.2
   - Unload lowest relevance first

3. **NPC Snapshot Format**
   - ID, name, position, faction, loyalty, mood, last_tick, events
   - ~50 bytes per NPC (vs 200+ loaded)
   - Binary serialization to disk

4. **Re-loading Strategy**
   - Proximity trigger: < 30 units
   - Event trigger: < 1 day until scheduled
   - Faction call trigger: immediately
   - Batch load 10 per tick (smooth loading)

### Implementation Files
- `include/Core.h`: NPCSnapshot struct
- `src/core/Core.cpp`: Snapshot methods
- `include/Registries.h`: Active set management
- `src/core/Registries.cpp`: Load/unload logic
- `src/main.cpp`: Main loop management

### Performance Target
- 1000 NPCs: <200 MB peak memory
- <1ms per tick management overhead
- Fast load: 10 NPCs/tick = 100ms to fill

---

## Priority Tiers

### TIER 1 (Critical Path - Do These First)
1. ✅ Task #1: Save/Load (DONE)
2. ✅ Task #2: NPC Problem System (DONE)
3. 🔄 Task #3: Event Cascading (NEXT)
4. 🔄 Task #6: Proximity Dialogue Queue

### TIER 2 (High Value - Do After Tier 1)
5. Task #4: LLM Error Recovery
6. Task #7: Dialogue State Machine
7. Task #10: LLM Request Queue

### TIER 3 (Polish & Scale - Optional but Nice)
8. Task #5: LLM Response Caching
9. Task #9: Ambient NPC-to-NPC Dialogue
10. Task #8: Culture & Religion
11. Task #11: NPC Lazy Loading

---

## Estimated Total Effort
- Tier 1: ✅ 10 hours (COMPLETE)
- Tier 2: ~40 hours (5 tasks, 6-12 hours each)
- Tier 3: ~60 hours (4 tasks, 10-15 hours each)
- **Total**: ~100 hours for complete Phase 15

---

## Success Metrics

### Per Task
- ✅ Code compiles (0 errors)
- ✅ No test regressions
- ✅ Feature works end-to-end
- ✅ Performance acceptable (<2ms overhead)

### System-Wide
- ✅ 10 NPCs with stable gameplay
- ✅ 100 NPCs with <10ms frame time
- ✅ 1000 NPCs with lazy loading (Task #11)
- ✅ Multi-hour play sessions stable
- ✅ Save/load cycle preserves state

---

## Next Immediate Action
**Task #3: Event Cascading** (6-8 hours)
- Highest impact on gameplay
- Enables crisis-driven narratives
- Moderate complexity, high value

**OR**

**Task #6: Multi-NPC Dialogue Queue** (6-8 hours)
- Better UX (multiple simultaneous problems)
- High player satisfaction
- Moderate-high complexity

Choose based on gameplay priority!

---

**Total Progress**: 2/11 Tasks (18%)  
**Next Session**: Task #3 or #6  
**Build Status**: ✅ All Green
