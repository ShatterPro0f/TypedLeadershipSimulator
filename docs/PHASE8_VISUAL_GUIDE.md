# Phase 8 Quick Visual Guide

## System Overview Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         GAME MAIN LOOP                              │
│                  (Every Frame - ~60 FPS = ~16ms/frame)             │
└─────────────────────────────────────────────────────────────────────┘
                                   │
                    ┌──────────────┴──────────────┐
                    │                             │
                    ▼                             ▼
            ┌──────────────┐           ┌──────────────────┐
            │   Input      │           │  GameTickProc    │
            │ (Player)     │           │  (Event Loop)    │
            └──────────────┘           └────────┬─────────┘
                    │                           │
                    │                      Tick #1-29: Skip
                    │                      Tick #30: ◄─── TRIGGER
                    │                           │
                    │               ┌───────────┴───────────┐
                    │               │                       │
            ┌───────┴───────┐   ┌──▼──────┐      ┌────▼──────────┐
            │ Convert Input │   │  Update │      │   Monitor     │
            │ (Fuzzy Parse) │   │  NPCs   │      │  World State  │
            │       │       │   │ Emotions│      │  (NEW - #8)   │
            │       │       │   └─────────┘      └────┬──────────┘
            │       │       │                         │
            │  [LLM: Interpret] (1-3s)          Compare to previous:
            │       │       │                    • NPC moods (Δ > 0.2?)
            └───────┼───────┘                    • Faction loyalty (Δ > 0.15?)
                    │                            • Resource scarcity?
            Extracted Action:                    │
            • Target: farmers                    ▼
            • Action: allocate              Has Changes?
            • Tone: positive          ┌───────────┴──────────┐
            • Priority: high          │                      │
                    │              NO │                   YES│
            ┌───────▼──────┐         │               ┌──────▼──────┐
            │   Update     │         │               │   Build     │
            │ Simulation   │         │               │  Snapshot   │
            │ (NPC Loyalty │         │               │             │
            │  Mood, etc)  │         │               │  Prune to   │
            └───────┬──────┘         │               │  top 50 NPCs│
                    │                │               └──────┬──────┘
                    │                │                      │
                    │                │            ┌─────────▼────────┐
                    │                │            │ Build LLM Prompt │
                    │                │            │                  │
                    │                │            │ Format:          │
                    │                │            │ • NPC changes    │
                    │                │            │ • Faction changes│
                    │                │            │ • Resources      │
                    │                │            │ • Events         │
                    │                │            └────────┬─────────┘
                    │                │                     │
                    │                │          ┌──────────▼──────────┐
                    │                │          │  Queue LLM Call     │
                    │                │          │  (ASYNC, non-block) │
                    │                │          │  Timeout: 10s       │
                    │                │          └──────────┬──────────┘
                    │                │                     │
                    │                └─────────┬───────────┘
                    │                          │
                    │                  ┌───────▼────────┐
                    │                  │   Continue     │
                    │                  │    Simulation  │
                    │                  │   (while LLM   │
                    │                  │   processes)   │
                    │                  └────────────────┘
                    │
            ┌───────▼──────────────────────────────────────┐
            │   Queue Narrative Issues                     │
            │   (When LLM returns or times out)           │
            │                                              │
            │   ┌────────────────────────────────────┐   │
            │   │ NarrativeIssueQueue (max 20)       │   │
            │   │                                    │   │
            │   │ [CRISIS] Farmers report starvation│   │
            │   │ [WARNING] Warriors morale low     │   │
            │   │ [OPPORT] Good conditions for trade│   │
            │   └────────────────────────────────────┘   │
            └───────────┬──────────────────────────────────┘
                        │
            ┌───────────▼─────────────┐
            │   Display to Player     │
            │   (Next Frame)          │
            │                         │
            │  Active Issues:         │
            │  [1] Farmers starvation │
            │  [2] Warriors morale    │
            │  [3] Trade opportunity  │
            │                         │
            │  "What do you do?"      │
            └─────────────────────────┘
                        │
                   ┌────▼────┐
                   │          │
                   ▼ (repeat) ▼
```

## Data Flow: World State → Narrative Issues

```
TICK #30 (World State Detection)
│
├─ NPC States:
│  ├─ Alice (mood 0.6 → 0.3, Δ=0.3 > 0.2) ✓ Changed
│  ├─ Bob (mood 0.5 → 0.52, Δ=0.02 < 0.2) ✗ No change
│  └─ Carol (mood 0.7 → 0.9, Δ=0.2 ≈ threshold) ✓ Changed
│
├─ Faction States:
│  ├─ Farmers avg_loyalty (0.7 → 0.5, Δ=0.2 > 0.15) ✓ Changed
│  └─ Warriors avg_loyalty (0.6 → 0.58, Δ=0.02 < 0.15) ✗ No change
│
├─ Resource States:
│  ├─ Food (120 → 80, crossed threshold 150?) ✓ Below threshold!
│  └─ Wood (100 → 95, still above 80?) ✗ No change
│
└─ Summary: 5 changes detected
   ├─ NPCs: Alice, Carol (2)
   ├─ Factions: Farmers (1)
   └─ Resources: Food (1)
   ► BUILD SNAPSHOT
   │
   └─ WorldStateSnapshot:
      ├─ significant_npcs: [Alice, Carol]
      ├─ affected_factions: [Farmers]
      ├─ changed_resources: [Food]
      └─ tick_number: 30
      │
      ► BUILD PROMPT
      │
      └─ LLM Prompt:
         "Current Tick: 30
         
         NPC Changes:
         - Alice (Farmer): Mood=0.3, Loyalty=0.5
         - Carol (Priest): Mood=0.9, Loyalty=0.8
         
         Faction Changes:
         - Farmers: Strength=0.4, Members=25
         
         Resource Changes:
         - Food: Quantity=80 (Scarcity Threshold: 150)
         
         Based on these changes, what are the most significant narrative issues?"
         │
         ► CALL LLM
         │
         └─ LLM Response (JSON):
            [
              {
                "title": "Farmers report starvation",
                "description": "Food supplies have plummeted. Farmers cannot feed their families.",
                "suggestion": "Consider increasing food production or rationing.",
                "priority": "CRISIS",
                "type": "RESOURCE_SCARCITY"
              },
              {
                "title": "Alice is distressed",
                "description": "Alice's mood has declined significantly. She may have concerns.",
                "suggestion": "Speak with Alice to understand her problems.",
                "priority": "WARNING",
                "type": "NPC_CRISIS"
              },
              {
                "title": "Carol is inspired",
                "description": "Carol is in high spirits and receptive to new tasks.",
                "suggestion": "Assign Carol to an important project.",
                "priority": "OPPORTUNITY",
                "type": "OPPORTUNITY"
              }
            ]
            │
            ► ADD TO QUEUE
            │
            └─ NarrativeIssueQueue:
               Issue #0: [CRISIS] Farmers report starvation
               Issue #1: [WARNING] Alice is distressed
               Issue #2: [OPPORTUNITY] Carol is inspired
```

## Class Relationship Diagram

```
                    GameTickProcessor
                           │
                           │ calls
                           ▼
          processWorldStateChanges(tick)
                  (every 30 ticks)
                           │
                ┌──────────┴──────────┐
                │                     │
                ▼                     ▼
    monitorWorldState()    detectSignificantChange()
         │                          │
         ▼                          ▼
    WorldStateMonitor          bool hasSignificantChange_
    ├─ previous_npc_states         │
    ├─ previous_faction_states     │
    ├─ previous_resource_states    │
    └─ tracking deltas             │
                                   │
                    ┌──────────────┘
                    │
                ┌───▼──────┐
                │   YES?   │
                └───┬──────┘
                    │
          ┌─────────▼─────────┐
          │                   │
      NO  │                YES│
          │                   ▼
          │          triggerNarrativeGeneration()
          │                   │
          │          ┌────────┴────────┐
          │          │                 │
          │          ▼                 ▼
          │  buildSnapshot()    buildNarrativePrompt()
          │  │                  │
          │  ├─ WorldStateSnapshot
          │  │  └─ pruneNPCs(50)
          │  │
          │  └─ Influence Score
          │     = loyalty(0.4) + faction(0.3) + advisor(0.2) + problem(0.1)
          │
          │  ┌──────────────────────────────────┐
          │  │ LLMPromptBuilder                 │
          │  │ ┌──────────────────────────────┐ │
          │  │ │ System Prompt                │ │
          │  │ │ "You are a narrative        │ │
          │  │ │  designer..."               │ │
          │  │ └──────────────────────────────┘ │
          │  │ ┌──────────────────────────────┐ │
          │  │ │ User Prompt                  │ │
          │  │ │ "Current Tick: 30           │ │
          │  │ │  NPC Changes:               │ │
          │  │ │  - Alice: Mood=0.3..."     │ │
          │  │ └──────────────────────────────┘ │
          │  └──────────────────────────────────┘
          │
          └─────────────┬──────────────┐
                        │              │
            ┌───────────▼──────────┐   │
            │ NarrativeGenerator   │   │
            │                      │   │
            │ LLM available?       │   │
            └───────┬──────────────┘   │
                    │                  │
           ┌────────┴────────┐         │
           │                 │         │
         YES                NO         │
           │                 │         │
           ▼                 ▼         │
    callLLM(prompt)   generateFromRules()
           │                 │         │
           ├─ Timeout?       │         │
           │ (10s)          │ (Templates)
           │                 │         │
           └──────┬──────────┘         │
                  │                    │
                  ▼                    ▼
          LLMResponse or Rules     Rules Generated
                │                    │
                └────────┬───────────┘
                         │
                         ▼
            parseLLMResponse() or createXxxIssue()
                         │
        ┌────────────────┴────────────────┐
        │                                 │
        ▼                                 ▼
    NarrativeIssue                 NarrativeIssue
    ├─ id                           ├─ id
    ├─ title                        ├─ title
    ├─ description                  ├─ description
    ├─ suggestion                   ├─ suggestion
    ├─ priority                     ├─ priority
    ├─ type                         ├─ type
    ├─ affected_npc_ids             ├─ affected_npc_ids
    ├─ created_tick                 ├─ created_tick
    ├─ expires_tick                 ├─ expires_tick
    └─ is_active                    └─ is_active
        │                               │
        └───────────┬───────────────────┘
                    │
                    ▼
        NarrativeIssueQueue
        ├─ max 20 issues
        ├─ sorted by priority
        ├─ auto-expire
        │
        ├─ getTopIssues(3)
        │  └─ Return top 3 by priority
        │
        └─ formatIssuesForDisplay()
           │
           ▼
    NarrativeIssueDisplay
    ├─ formatIssueForDisplay()
    │  "[CRISIS] Farmers report starvation
    │   Food supplies have plummeted. Farmers cannot feed their families.
    │   → Consider increasing food production or rationing."
    │
    ├─ formatMultipleIssues()
    │  "Active Issues:
    │   [1] [CRISIS] Farmers report starvation
    │   [2] [WARNING] Alice is distressed
    │   [3] [OPPORTUNITY] Carol is inspired
    │   ... and 0 more issues."
    │
    └─ Display to Player
```

## Priority Sorting Example

```
Issue Queue (Before Sorting):
┌──────────────────────────────────────────┐
│ Issue #1: [WARNING] Alice is troubled    │ (priority=1)
│ Issue #2: [CRISIS] Food running low      │ (priority=0)
│ Issue #3: [OPPORTUNITY] Trade available │ (priority=2)
└──────────────────────────────────────────┘

Sort by priority enum value (lower = higher priority):
┌──────────────────────────────────────────┐
│ Issue #2: [CRISIS] Food running low      │ (priority=0) ◄─ First
│ Issue #1: [WARNING] Alice is troubled    │ (priority=1) ◄─ Second
│ Issue #3: [OPPORTUNITY] Trade available │ (priority=2) ◄─ Third
└──────────────────────────────────────────┘

Displayed to Player:
┌──────────────────────────────────────────┐
│ Active Issues:                           │
│ [1] [CRISIS] Food running low            │
│ [2] [WARNING] Alice is troubled          │
│ [3] [OPPORTUNITY] Trade available       │
└──────────────────────────────────────────┘
```

## Rule-Based Fallback Logic

```
if (LLM not available OR timeout)
{
    // Template 1: Unhappy NPC
    if (npc.mood < 0.3)
    {
        issue.title = npc.name + " is troubled"
        issue.priority = WARNING
        issue.type = NPC_CRISIS
    }
    
    // Template 2: Happy NPC
    if (npc.mood > 0.8)
    {
        issue.title = npc.name + " is inspired"
        issue.priority = OPPORTUNITY
        issue.type = OPPORTUNITY
    }
    
    // Template 3: Weak Faction
    if (faction.strength < 0.4)
    {
        issue.title = faction.name + " weakening"
        issue.priority = WARNING
        issue.type = FACTION_CONFLICT
    }
    
    // Template 4: Scarcity
    if (resource.quantity < resource.scarcity_threshold)
    {
        issue.title = resource.name + " scarcity"
        issue.priority = CRISIS
        issue.type = RESOURCE_SCARCITY
    }
}
```

## Timing Diagram

```
Time ──────────────────────────────────────────────────────────────────────▶

Frame 1-29:    Skip world state check (debounce)
               │
Frame 30:      ├─ Detect world state changes (2ms)
               ├─ Build snapshot (1ms)
               ├─ Queue LLM request (async, non-blocking) ✓
               ├─ Return immediately (0ms blocking)
               └─ Total: 3ms ✓ (budget: 16ms)
               │
Frame 31:      Continue simulation (while LLM processes)
Frame 32:      Continue simulation
Frame 33:      Continue simulation
...
Frame 40:      LLM response arrives (after ~10s real time)
               ├─ onNarrativeGenerationComplete()
               ├─ Add issues to queue (1ms)
               └─ Next frame: Display to player
               │
Frame 41:      Display issues to player
               ├─ Show top 3 narrative issues
               ├─ Wait for player response
               └─ Player responds

Frame 60:      Next world state check (30 frames later)
               └─ Repeat cycle
```

---

## Key Takeaways

1. **Event-Driven**: No schedules, continuous monitoring every tick
2. **Async/Non-Blocking**: LLM doesn't block simulation
3. **Fallback Safety**: Works without LLM via rule-based templates
4. **Efficient**: Max 20 issues, top 50 NPCs in snapshot
5. **Deterministic**: Seeded RNG, reproducible results
6. **Scalable**: Works with 1000+ NPCs (pruned to 50 per snapshot)

---

**For More Details**: See PHASE8_TECHNICAL_REFERENCE.md
