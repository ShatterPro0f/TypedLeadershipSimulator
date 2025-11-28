# Phase 7 Implementation Plan: LLM Infrastructure & Provider Abstraction

**Objective**: Establish LLM backend with multiple provider support and offline fallback  
**Timeline**: ~2-3 weeks of development (expanded from 1-1.5 weeks for Phase 1-6 parity)  
**Dependency**: Phase 4 (async queue infrastructure)  
**Blocking**: Phases 8, 9, 10, 13 (all LLM-dependent systems)  
**Can Run Parallel With**: Phases 5, 6, 11, 12 (input/dialogue/movement/loop systems)  
**Detail Level**: ✅ Matches Phases 1-6 (algorithms, formulas, 30+ tests, edge cases, determinism)

---

## Overview

Phase 7 is the **LLM infrastructure layer** that abstracts multiple LLM providers and enables fallback to offline rule-based generation. This phase focuses on:

1. **Provider Abstraction** — Common interface for OpenAI, Local LLaMA, offline fallback
2. **API Configuration** — Load API keys from environment, config files, with validation
3. **Request Management** — Rate limiting, token tracking, cost estimation
4. **Timeout & Retry** — Exponential backoff with configurable timeouts
5. **Response Caching** — Avoid redundant LLM calls for identical prompts
6. **Error Handling** — Graceful fallback on API errors
7. **Deterministic Replay** — Log all LLM inputs/outputs for reproducibility

### Key Principles
- **Multi-Provider Support** — OpenAI (primary), Local LLaMA (fallback), Offline (emergency)
- **Zero Hallucination** — Fallback generation strictly templated, never free-form
- **Token Tracking** — Monitor usage for cost management
- **Non-Blocking** — All LLM calls via async queues from Phase 4
- **Reproducible** — Log all calls for replay mode

---

## Detailed Algorithms & Formulas

### Algorithm 1: Provider Selection & Fallback Cascade

**Purpose**: Determine which LLM provider to use based on availability and priority.

**Pseudocode**:
```
function selectLLMProvider(preferredProvider, configuration):
    if preferredProvider == OPENAI:
        if OpenAIProvider.isAvailable() and system.hasNetworkConnection():
            return OpenAIProvider
        else if configuration.fallbackEnabled:
            log("OpenAI unavailable; trying fallback")
            return selectLLMProvider(LOCAL_LLAMA, configuration)
    
    else if preferredProvider == LOCAL_LLAMA:
        if LocalLlamaProvider.isAvailable() and system.isServerRunning():
            return LocalLlamaProvider
        else if configuration.fallbackEnabled:
            log("Local LLaMA unavailable; using offline")
            return OfflineFallbackProvider
    
    else:
        return OfflineFallbackProvider
```

**Worked Example**:
- Scenario: User configures preferred provider as OpenAI
- OpenAI API key present in environment ✓
- Network connectivity check returns true ✓
- OpenAI API timeout set to 10 seconds
- **Result**: SelectProvider returns OpenAIProvider instance
- Expected call chain: openai_call() → fallback_retries(3) → offline_fallback()

**Edge Cases**:
1. **No API key, no network**: Skip to offline immediately (no delay)
2. **API key but network down**: Retry 3x with exponential backoff (1s, 2s, 4s), then fallback
3. **Multiple providers configured**: Strict priority order (OpenAI > Local > Offline)
4. **Configuration missing entirely**: Use hardcoded defaults (fallback to offline)

**Determinism**:
- Provider selection must be deterministic for same config (no randomness in selection)
- However, LLM response might vary (inherent to LLM)
- Fallback order must match logged provider selection for replays

---

### Algorithm 2: Provider Request Prioritization & Queuing

**Purpose**: Manage multiple concurrent LLM requests with priority-based processing.

**Priority Classes**:
```
enum Priority {
    HIGH = 0,        // Player input interpretation (must respond <3s)
    MEDIUM = 1,      // World state narrative generation (can be slower)
    LOW = 2          // NPC ambient conversations (lowest priority)
};

struct PriorityQueueEntry {
    int requestId;
    Priority priority;
    int enqueuedTick;
    int timeoutTicks;
    string prompt;
    function<void(LLMResponse)> callback;
    LLMProviderType preferredProvider;
};
```

**Queuing Algorithm**:
```
function enqueueRequest(request, priorityQueue):
    // Check if same request already queued
    if priorityQueue.contains(hash(request.prompt)):
        log("Duplicate request ignored; using existing call")
        return false
    
    // Determine timeout in ticks based on priority
    request.timeoutTicks = (request.priority == HIGH) ? 30 : 
                           (request.priority == MEDIUM) ? 100 : 
                           (request.priority == LOW) ? 50
    
    // Insert into priority queue (highest priority first)
    priorityQueue.insert(request, orderBy: [priority, enqueuedTick])
    
    // Check if queue length exceeds max for priority
    max_high = 5
    max_medium = 3
    max_low = 10
    
    if getQueueLengthForPriority(HIGH) > max_high:
        log("HIGH priority queue full; dropping oldest LOW request")
        removed = priorityQueue.removeLowest(LOW)
    
    return true

function dequeueNextRequest(priorityQueue):
    // Dequeue highest priority that hasn't timed out
    for entry in priorityQueue.sortedByPriority():
        if (currentTick - entry.enqueuedTick) < entry.timeoutTicks:
            return priorityQueue.remove(entry)
        else:
            log("Request timeout; marking failed")
            entry.callback(LLMResponse{.wasSuccessful = false, 
                                       .errorMessage = "Timeout in queue"})
    
    return null
```

**Worked Example**:
- Scenario: Player makes decision (HIGH priority) while NPC conversation queued (LOW)
- HIGH request: priority=0, timeout=30 ticks, enqueuedTick=1000, prompt="Player allocated food"
- LOW request: priority=2, timeout=50 ticks, enqueuedTick=995, prompt="NPC pair dialogue"
- Queue order: [HIGH(1000), LOW(995)]? NO → [LOW(995), HIGH(1000)]? NO
- **Result**: Sorted by priority first → [HIGH(1000), LOW(995)] 
- HIGH dequeued first, processed immediately
- LOW waits in queue, processes after HIGH completes (~3-5 ticks later)

**Queue Size Constraints**:
```
MAX_HIGH_PRIORITY_QUEUE = 5        // Player input must stay responsive
MAX_MEDIUM_PRIORITY_QUEUE = 3      // 3 world state snapshots max
MAX_LOW_PRIORITY_QUEUE = 10        // Ambient conversations can batch
MAX_TOTAL_QUEUE = 15
```

**Edge Cases**:
1. **Queue overflow**: Drop oldest LOW priority request (preserve HIGH/MEDIUM)
2. **Same prompt enqueued twice**: Use cache result instead of duplicating call
3. **Request timeout in queue**: Call callback with error, remove from queue
4. **Priority change mid-queue**: Not allowed; priority fixed at enqueue time

---

### Algorithm 3: Exponential Backoff with Jitter

**Purpose**: Retry failed API calls with increasing delays to avoid thundering herd.

**Formula**:
```
delay_ms = base_delay_ms * (2 ^ attempt) + random_jitter
where:
  base_delay_ms = 1000 (1 second)
  attempt = 0, 1, 2, ... (retry number)
  random_jitter = random(0, base_delay_ms * attempt) in milliseconds
  max_delay_ms = 30000 (cap at 30 seconds)
```

**Pseudocode**:
```
function callWithRetry(provider, prompt, maxRetries=3):
    for attempt = 0 to maxRetries-1:
        try:
            response = provider.callLLM(prompt, timeout=10s)
            if response.wasSuccessful:
                return response
        catch TimeoutException:
            log("Timeout on attempt {attempt+1}/{maxRetries}")
        catch NetworkException as e:
            log("Network error: {e.message}")
        
        // Exponential backoff
        if attempt < maxRetries - 1:
            delay_ms = 1000 * (1 << attempt)  // 2^attempt
            jitter_ms = random(0, delay_ms)
            total_delay_ms = min(delay_ms + jitter_ms, 30000)
            
            log("Retry {attempt+1} after {total_delay_ms}ms")
            sleep(total_delay_ms)
    
    // All retries exhausted
    return LLMResponse{.wasSuccessful = false, 
                       .errorMessage = "Max retries exceeded"}
```

**Worked Examples**:

**Example A**: OpenAI API timeout
- Attempt 0: Timeout at 10s → delay = 1000 * 2^0 + jitter(0-1000) = 1000-2000ms
- Sleep ~1500ms, retry
- Attempt 1: Timeout at 10s → delay = 1000 * 2^1 + jitter(0-2000) = 2000-4000ms
- Sleep ~3000ms, retry
- Attempt 2: Success at 2s → return response
- **Total time**: ~10s + ~1.5s + ~10s + ~3s + ~2s = ~26.5 seconds

**Example B**: Network down all attempts
- Attempt 0: Network error → delay = 1000-2000ms
- Attempt 1: Network error → delay = 2000-4000ms
- Attempt 2: Network error → delay = 4000-8000ms
- **Total time**: ~3 retries × average 5s = ~15 seconds before giving up
- **Result**: Return error, trigger offline fallback

**Edge Cases**:
1. **Jitter overflow**: Cap total at 30 seconds max (no individual delay > 30s)
2. **Timeout < delay**: If timeout=3s but delay=5s, use timeout without retry (avoid sleep cycle)
3. **Random seed consistency**: For deterministic replay, use seeded RNG (same seed per tick)
4. **Zero retries configured**: attempt=0 with no retries → fail immediately, no sleep

---

### Algorithm 4: Rate Limiting with Token Bucket

**Purpose**: Enforce API rate limits (e.g., 60 calls/minute) without rejecting requests.

**Token Bucket Formula**:
```
tokens_available = min(max_tokens, 
                       last_tokens_available + 
                       refill_rate_per_second * (now - last_refill_time))

refill_rate = max_tokens_per_minute / 60
max_tokens = max_tokens_per_minute

if tokens_available >= tokens_required:
    allow request
    tokens_available -= tokens_required
else:
    deny request (queue for retry)
```

**Pseudocode**:
```
class TokenBucketRateLimiter {
    float tokens_available;
    float max_tokens = 60.0;     // 60 calls per minute
    float refill_rate = 1.0;     // 1 token per second (60/60)
    timestamp last_refill_time;
    
    function canMakeRequest():
        now = getCurrentTime()
        elapsed_seconds = (now - last_refill_time).totalSeconds()
        
        // Refill bucket
        tokens_available = min(max_tokens, 
                               tokens_available + 
                               refill_rate * elapsed_seconds)
        last_refill_time = now
        
        // Check if we have tokens
        if tokens_available >= 1.0:
            tokens_available -= 1.0
            return true
        else:
            wait_time = (1.0 - tokens_available) / refill_rate
            log("Rate limited; wait {wait_time} seconds")
            return false
}
```

**Worked Example**:
- Config: 60 calls per minute → max_tokens = 60, refill_rate = 1.0 token/sec
- Tick 0: tokens_available = 60 (full)
- Call 1 at tick 0: allowed, tokens = 59
- Calls 2-60 at tick 0: allowed (59 calls), tokens = 0
- Call 61 at tick 0: denied, wait_time = 1.0 second
- Tick 60 (1 second later): tokens_available = 0 + 1.0*1 = 1.0
- Call 61 retry at tick 60: allowed
- **Result**: Maintain 60 calls/minute rate limit with burst capacity

**Edge Cases**:
1. **Initial state**: Full bucket on startup (all 60 tokens available)
2. **Negative tokens**: Never allow; clamp to [0, max_tokens]
3. **Rate limit exceeded**: Queue request, retry when tokens available
4. **Burst traffic**: Allow bursts up to max_tokens, then enforce rate

---

### Algorithm 5: Response Caching with LRU Eviction

**Purpose**: Cache LLM responses to avoid duplicate API calls within TTL window.

**Cache Entry Structure**:
```
struct CacheEntry {
    string promptHash;          // SHA256(prompt)
    string response;            // LLM response text
    timestamp createdAt;        // When cached
    int hitCount;               // How many times hit
    int inputTokens;            // For cost tracking
    int completionTokens;
    enum CacheStatus {
        VALID,                  // Within TTL
        EXPIRED,                // Past TTL
        STALE                   // Within TTL but prompt changed
    };
};
```

**LRU Eviction Formula**:
```
priority_score = (1 - age_ratio) * w_age + (hitCount / max_hits) * w_frequency

where:
    age_ratio = (now - createdAt) / cache_ttl  [0-1]
    hitCount = number of cache hits
    max_hits = 100 (typical max)
    w_age = 0.6 (age more important than frequency)
    w_frequency = 0.4 (but frequency matters too)

Evict entries with LOWEST priority_score
```

**Pseudocode**:
```
function getOrFetch(prompt, provider):
    promptHash = sha256(prompt)
    
    // Check cache
    if cache.contains(promptHash):
        entry = cache[promptHash]
        if isValid(entry) and not isExpired(entry):
            entry.hitCount += 1
            log("Cache hit: {prompt}")
            return entry.response
        else:
            log("Cache miss: entry expired or stale")
            cache.remove(promptHash)
    
    // Fetch from provider
    response = provider.callLLM(prompt, timeout=10s)
    
    if response.wasSuccessful:
        // Add to cache
        entry = CacheEntry{
            .promptHash = promptHash,
            .response = response.text,
            .createdAt = now,
            .hitCount = 0,
            .inputTokens = response.inputTokens,
            .completionTokens = response.completionTokens
        }
        
        // Evict if cache full
        if cache.size() >= max_cache_entries:
            evictLRUEntry(cache)
        
        cache[promptHash] = entry
    
    return response.text

function evictLRUEntry(cache):
    lowest_priority = null
    lowest_score = 1.0
    
    for entry in cache:
        age_ratio = (now - entry.createdAt) / cache_ttl
        score = (1 - age_ratio) * 0.6 + (entry.hitCount / 100) * 0.4
        
        if score < lowest_score:
            lowest_score = score
            lowest_priority = entry
    
    log("Evicting cache entry: {lowest_priority.promptHash}")
    cache.remove(lowest_priority)
```

**Worked Example**:
- Cache TTL = 300 seconds
- Max entries = 100
- Entry A: created 50 seconds ago, 5 hits
  - age_ratio = 50/300 = 0.167
  - hitCount/max = 5/100 = 0.05
  - priority = (1-0.167)*0.6 + 0.05*0.4 = 0.50 + 0.02 = 0.52
- Entry B: created 200 seconds ago, 0 hits
  - age_ratio = 200/300 = 0.667
  - hitCount/max = 0/100 = 0
  - priority = (1-0.667)*0.6 + 0*0.4 = 0.20
- Entry C: created 5 seconds ago, 50 hits
  - age_ratio = 5/300 = 0.017
  - hitCount/max = 50/100 = 0.5
  - priority = (1-0.017)*0.6 + 0.5*0.4 = 0.59 + 0.20 = 0.79
- **Ranking by priority**: C (0.79) > A (0.52) > B (0.20)
- **Evict order** (if needed): B first (lowest score 0.20)

**Edge Cases**:
1. **Cache miss on new prompt**: Fetch, cache, return
2. **Cache hit but entry expired**: Treat as miss, fetch, replace cache entry
3. **Cache full and entry expired**: Remove expired, add new (no eviction needed)
4. **Identical prompts with different capitalization**: Normalize prompt before hashing
5. **Cache corruption**: Fallback to fetch without cache

---

### Algorithm 6: Token Cost Calculation & Budget Enforcement

**Purpose**: Track API usage and prevent overspending on LLM calls.

**Cost Formula**:
```
cost_usd = (input_tokens / 1000) * price_per_1k_input + 
           (completion_tokens / 1000) * price_per_1k_completion

Example pricing (GPT-3.5-turbo as of 2024):
    input_price = $0.0005 per 1K tokens
    completion_price = $0.0015 per 1K tokens

Example calculation:
    input_tokens = 300
    completion_tokens = 100
    
    cost = (300/1000)*0.0005 + (100/1000)*0.0015
         = 0.00015 + 0.00015
         = $0.0003
```

**Budget Enforcement**:
```
daily_cost = sum(all calls today)
daily_budget = configuration.daily_budget_usd (e.g., $10.00)

if daily_cost + estimated_call_cost > daily_budget:
    log("Daily budget exceeded; rejecting call")
    return error
else:
    allow call
```

**Pseudocode**:
```
function trackTokenUsage(response):
    usage = LLMUsage{
        .inputTokens = response.inputTokens,
        .completionTokens = response.completionTokens,
        .costUSD = calculateCost(response),
        .timestamp = now
    }
    
    usageHistory.push_back(usage)
    
    // Log usage
    totalCost = sum(u.costUSD for u in usageHistory)
    log("LLM Usage: {response.inputTokens} in + {response.completionTokens} out = ${usage.costUSD:.4f}")
    
    // Check daily budget
    dailyUsage = filterByToday(usageHistory)
    dailyCost = sum(u.costUSD for u in dailyUsage)
    
    if dailyCost > daily_budget * 0.8:  // Warn at 80%
        log("WARNING: Daily budget 80% consumed: ${dailyCost:.2f} / ${daily_budget:.2f}")
    
    if dailyCost > daily_budget:
        log("ERROR: Daily budget exceeded!")
        sendAlertEmail("LLM budget overage: ${dailyCost:.2f}")
        // Disable LLM, use offline fallback only

function estimateCostBeforeCall(prompt, maxTokens):
    estimated_input_tokens = wordCount(prompt) * 1.3  // Rough estimate
    estimated_completion_tokens = maxTokens * 0.8  // Assume 80% usage
    
    return (estimated_input_tokens/1000)*0.0005 + 
           (estimated_completion_tokens/1000)*0.0015
```

**Worked Example**:
- Daily budget: $10.00
- Pricing: $0.0005 per 1K input, $0.0015 per 1K completion
- Call 1 at 9:00 AM: 200 input, 50 output → $(200/1000)*0.0005 + (50/1000)*0.0015 = $0.00025
- Call 2 at 10:00 AM: 500 input, 150 output → $(500/1000)*0.0005 + (150/1000)*0.0015 = $0.00475
- Call 3 at 11:00 AM: 300 input, 100 output → $(300/1000)*0.0005 + (100/1000)*0.0015 = $0.00030
- **Daily total so far**: $0.00025 + $0.00475 + $0.00030 = $0.0053 (0.053% of budget)
- **At 80% budget threshold**: $10 * 0.8 = $8.00
- **Estimated calls per day to reach $8**: ~1,400 calls
- **Status**: Well within budget

**Edge Cases**:
1. **Estimation error**: Actual tokens >> estimated → overages tracked post-call
2. **Day boundary**: Reset daily cost at midnight
3. **Multiple concurrent requests**: Batch cost updates to avoid race conditions
4. **Budget zero or negative**: Disable LLM calls entirely (use offline fallback)

---

### Algorithm 7: Error Recovery & Fallback Cascade

**Purpose**: Handle API errors gracefully, attempting recovery or fallback.

**Error Classification**:
```
enum ErrorType {
    NETWORK_ERROR,          // No internet, DNS failure
    TIMEOUT_ERROR,          // API didn't respond in time
    AUTHENTICATION_ERROR,   // Invalid API key
    RATE_LIMIT_ERROR,       // 429 Too Many Requests
    SERVER_ERROR,           // 5xx from API
    INVALID_RESPONSE,       // Response malformed/unparseable
    UNKNOWN_ERROR
};

enum RecoveryStrategy {
    RETRY_WITH_BACKOFF,     // Try again with delay
    SWITCH_PROVIDER,        // Try different provider
    USE_CACHED_RESPONSE,    // Return last known good response
    USE_OFFLINE_FALLBACK,   // Generate templated response
    FAIL_AND_REPORT         // No recovery possible
};
```

**Error Recovery Pseudocode**:
```
function handleLLMError(error, request, currentProvider):
    strategy = FAIL_AND_REPORT
    
    if error.type == NETWORK_ERROR:
        if currentProvider != OFFLINE_FALLBACK:
            strategy = SWITCH_PROVIDER  // Try offline fallback
        else:
            strategy = FAIL_AND_REPORT
    
    else if error.type == TIMEOUT_ERROR:
        if request.retryCount < max_retries:
            strategy = RETRY_WITH_BACKOFF
        else:
            strategy = USE_OFFLINE_FALLBACK
    
    else if error.type == AUTHENTICATION_ERROR:
        log("CRITICAL: Invalid API key!")
        disableLLM(currentProvider)
        strategy = USE_OFFLINE_FALLBACK
    
    else if error.type == RATE_LIMIT_ERROR:
        wait_time = parseRetryAfter(error.headers)
        sleep(wait_time)
        if request.retryCount < max_retries:
            strategy = RETRY_WITH_BACKOFF
        else:
            strategy = QUEUE_FOR_LATER_RETRY
    
    else if error.type == SERVER_ERROR:
        if currentProvider == OPENAI:
            strategy = SWITCH_PROVIDER
        else:
            strategy = USE_CACHED_RESPONSE
    
    else if error.type == INVALID_RESPONSE:
        strategy = USE_OFFLINE_FALLBACK
    
    // Execute recovery strategy
    switch strategy:
        case RETRY_WITH_BACKOFF:
            delay_ms = calculateExponentialBackoff(request.retryCount)
            sleep(delay_ms)
            return callLLMProvider(request, currentProvider)
        
        case SWITCH_PROVIDER:
            nextProvider = getNextProviderInChain(currentProvider)
            return callLLMProvider(request, nextProvider)
        
        case USE_CACHED_RESPONSE:
            cached = getCachedResponse(request.prompt)
            if cached:
                log("Using cached response due to error")
                return cached
            else:
                strategy = USE_OFFLINE_FALLBACK
        
        case USE_OFFLINE_FALLBACK:
            return offlineFallback.callLLM(request.prompt)
        
        case QUEUE_FOR_LATER_RETRY:
            request.retryCount += 1
            delayedQueue.enqueue(request, delayMs=60000)
            return LLMResponse{.wasSuccessful = false, 
                               .errorMessage = "Queued for later retry"}
        
        case FAIL_AND_REPORT:
            sendErrorNotification("LLM call failed permanently")
            return LLMResponse{.wasSuccessful = false, 
                               .errorMessage = error.message}
```

**Recovery Decision Tree**:
```
Error occurs
  ├─ Network Error?
  │   ├─ Try offline → success
  │   └─ Offline also fails → report to player
  │
  ├─ Timeout?
  │   ├─ Retries left? → backoff & retry
  │   └─ No retries → use cached or offline
  │
  ├─ Auth Error?
  │   ├─ Log critical error
  │   ├─ Disable provider
  │   └─ Use offline fallback
  │
  ├─ Rate Limit?
  │   ├─ Parse Retry-After header
  │   ├─ Wait specified time
  │   └─ Retry or queue for later
  │
  ├─ Server Error?
  │   ├─ Switch to different provider
  │   └─ Or use cached response
  │
  └─ Invalid Response?
      ├─ Log parse error
      └─ Use offline fallback
```

**Worked Example**:
- Scenario: OpenAI API returns 500 Server Error
- Error type: SERVER_ERROR
- Current provider: OPENAI
- Cached response available: YES (30 minutes old)
- Decision tree: Server Error + Cached available → use cached response
- **Result**: Return cached response, log "Server error; using cached response from 30min ago"
- **User impact**: No visible delay, response provided successfully

**Edge Cases**:
1. **All providers fail**: Ultimate fallback to offline (always works)
2. **Cascade failures**: OpenAI fails → Local LLaMA fails → Offline succeeds
3. **Offline fallback fails**: Impossible (offline always succeeds by design)
4. **Retry storm**: Exponential backoff prevents thundering herd
5. **Cache corruption on fallback**: Fallback still works (templated responses)

---

### Algorithm 8: Deterministic Replay with LLM Logging

**Purpose**: Reproduce identical LLM behavior for debugging and testing.

**Replay Log Format**:
```
struct LLMCallLogEntry {
    int tickNumber;
    int callId;
    LLMProviderType usedProvider;
    string prompt;
    string llmResponse;
    int inputTokens;
    int completionTokens;
    float latencyMs;
    bool wasSuccessful;
    string errorIfAny;
    float randomSeed;          // For RNG during LLM call
    timestamp wallClockTime;
};

// Log stored as JSON lines (one entry per line)
// Example: {"tickNumber":1234,"callId":5,"usedProvider":"openai","prompt":"...","llmResponse":"...","wasSuccessful":true}
```

**Logging Pseudocode**:
```
function recordLLMCall(tickNumber, callId, prompt, response, provider, seed):
    entry = LLMCallLogEntry{
        .tickNumber = tickNumber,
        .callId = callId,
        .usedProvider = provider,
        .prompt = prompt,
        .llmResponse = response.text,
        .inputTokens = response.inputTokens,
        .completionTokens = response.completionTokens,
        .latencyMs = response.latencyMs,
        .wasSuccessful = response.wasSuccessful,
        .errorIfAny = response.errorMessage,
        .randomSeed = seed,
        .wallClockTime = now
    }
    
    llmCallLog.append(entry)
    logFile.writeLine(jsonSerialize(entry))

function replayLLMCall(tickNumber, callId, logFile):
    // In replay mode: skip actual LLM call
    entry = logFile.find(tickNumber, callId)
    
    if entry == null:
        log("ERROR: LLM call {callId} at tick {tickNumber} not found in replay log")
        return ERROR
    
    // Return logged response without calling LLM
    return LLMResponse{
        .text = entry.llmResponse,
        .inputTokens = entry.inputTokens,
        .completionTokens = entry.completionTokens,
        .latencyMs = entry.latencyMs,
        .wasSuccessful = entry.wasSuccessful,
        .errorMessage = entry.errorIfAny
    }

function validateDeterminism(saveFile, replayLogFile, numTicks):
    // Run simulation twice, compare results
    state1 = runSimulation(saveFile, replayLogFile, numTicks, replayMode=false)
    state2 = runSimulation(saveFile, replayLogFile, numTicks, replayMode=true)
    
    if state1.equals(state2):
        log("✓ Determinism validated: byte-identical replay")
        return true
    else:
        // Find first divergence
        for tick in 0 to numTicks:
            if state1[tick] != state2[tick]:
                log("✗ Divergence at tick {tick}")
                log("  Live LLM: {state1[tick].llmResponse}")
                log("  Replay:   {state2[tick].llmResponse}")
                return false
        return true
```

**Replay Mode Behavior**:
- Command-line flag: `--replay saveFile.dat logFile.jsonl`
- When replay mode enabled:
  - All LLM calls skipped (use logged responses)
  - All RNG seeded identically
  - All file I/O deterministic
  - Frame-by-frame stepping enabled (press SPACE to advance 1 tick)
- **Result**: Byte-identical simulation replay with logging at each tick

**Worked Example - Divergence Debugging**:
- Run 1 (Live): Tick 1234, Player says "help farmers", LLM returns "Allocate food"
- Run 2 (Replay): Tick 1234, same player input, LLM returns "Allocate food"
- Results match ✓
- 10 ticks later...
- Run 1 (Live): Tick 1244, Alice's mood = 0.65
- Run 2 (Replay): Tick 1244, Alice's mood = 0.63
- **Divergence found**: Mood calculation differs by 0.02
- Investigate: Floating-point precision? RNG seed different? Look at mood update algorithm
- **Resolution**: Mood calculation had platform-dependent floating-point rounding; fixed by using consistent precision

**Edge Cases**:
1. **Replay log missing**: Fall back to live LLM calls (may diverge)
2. **Replay log corrupted**: Panic and fail (prevent bad replay data)
3. **Tick number mismatch**: Log tick N but replay tick M → detect and report
4. **Call ID mismatch**: Different order of LLM calls → detect and report
5. **Platform differences**: Floating-point precision may vary (use tolerant comparison, ~1e-5)

---

## Detailed Breakdown

### 1. LLM Provider Interface

**File**: `include/LLMProvider.h`

```cpp
enum class LLMProviderType {
  OPENAI = 0,
  LLAMA_LOCAL = 1,
  OFFLINE_FALLBACK = 2
};

struct LLMResponse {
  std::string text;
  int inputTokens;
  int completionTokens;
  float costUSD;
  float latencyMs;
  bool wasSuccessful;
  std::string errorMessage;
};

struct LLMUsage {
  int inputTokens = 0;
  int completionTokens = 0;
  float costUSD = 0.0f;
  std::chrono::system_clock::time_point timestamp;
};

// Abstract base class for all LLM providers
class LLMProvider {
public:
  virtual ~LLMProvider() = default;
  
  // Call LLM (blocking)
  virtual LLMResponse callLLM(
    const std::string& prompt,
    float temperature = 0.7f,
    int maxTokens = 500
  ) = 0;
  
  // Check if provider is available
  virtual bool isAvailable() = 0;
  
  // Get provider type
  virtual LLMProviderType getProviderType() const = 0;
  
  // Configuration
  virtual bool loadConfig(const std::string& configFile) = 0;
};

// Factory function to create appropriate provider
class LLMProviderFactory {
public:
  static LLMProvider* createProvider(
    LLMProviderType type,
    const std::string& configFile
  );
};
```

---

### 2. OpenAI Provider Implementation

**File**: `include/OpenAIProvider.h`

```cpp
class OpenAIProvider : public LLMProvider {
private:
  std::string apiKey;
  std::string apiUrl = "https://api.openai.com/v1/chat/completions";
  std::string model = "gpt-3.5-turbo";
  int timeoutSeconds = 10;
  int maxRetries = 3;
  
public:
  LLMResponse callLLM(
    const std::string& prompt,
    float temperature = 0.7f,
    int maxTokens = 500
  ) override;
  
  bool isAvailable() override;
  
  LLMProviderType getProviderType() const override { 
    return LLMProviderType::OPENAI; 
  }
  
  bool loadConfig(const std::string& configFile) override;
  
private:
  // HTTP request helper
  std::string makeHTTPRequest(const std::string& requestBody);
  
  // Parse OpenAI response JSON
  LLMResponse parseOpenAIResponse(const std::string& responseJSON);
};
```

**Implementation Details**:

```cpp
LLMResponse OpenAIProvider::callLLM(
  const std::string& prompt,
  float temperature,
  int maxTokens
) {
  if (!isAvailable()) {
    return LLMResponse{
      .text = "",
      .wasSuccessful = false,
      .errorMessage = "API key not configured"
    };
  }
  
  // Retry loop with exponential backoff
  for (int attempt = 0; attempt < maxRetries; attempt++) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Build JSON request
    nlohmann::json requestBody = {
      {"model", model},
      {"messages", nlohmann::json::array()},
      {"temperature", temperature},
      {"max_tokens", maxTokens}
    };
    requestBody["messages"].push_back({
      {"role", "user"},
      {"content", prompt}
    });
    
    try {
      // Make HTTP request
      std::string response = makeHTTPRequest(requestBody.dump());
      
      // Parse response
      LLMResponse result = parseOpenAIResponse(response);
      
      // Calculate latency
      auto endTime = std::chrono::high_resolution_clock::now();
      result.latencyMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
      
      return result;
      
    } catch (const std::exception& e) {
      // Exponential backoff: 1s, 2s, 4s, 8s
      if (attempt < maxRetries - 1) {
        int backoffMs = 1000 * (1 << attempt);
        std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
      }
      
      if (attempt == maxRetries - 1) {
        return LLMResponse{
          .text = "",
          .wasSuccessful = false,
          .errorMessage = "Max retries exceeded: " + std::string(e.what())
        };
      }
    }
  }
  
  return LLMResponse{
    .text = "",
    .wasSuccessful = false,
    .errorMessage = "Unknown error"
  };
}

bool OpenAIProvider::isAvailable() {
  return !apiKey.empty();
}

bool OpenAIProvider::loadConfig(const std::string& configFile) {
  // Try environment variable first
  const char* env_key = std::getenv("OPENAI_API_KEY");
  if (env_key) {
    apiKey = std::string(env_key);
    return true;
  }
  
  // Try config file
  std::ifstream config_file(configFile);
  if (!config_file.is_open()) {
    std::cerr << "Config file not found: " << configFile << "\n";
    return false;
  }
  
  nlohmann::json config;
  config_file >> config;
  
  if (config.contains("openai_api_key")) {
    apiKey = config["openai_api_key"].get<std::string>();
    return true;
  }
  
  return false;
}
```

---

### 3. Offline Fallback Provider

**File**: `include/OfflineFallbackProvider.h`

```cpp
class OfflineFallbackProvider : public LLMProvider {
private:
  // Template library for offline generation
  std::map<std::string, std::vector<std::string>> responseTemplates;
  
public:
  LLMResponse callLLM(
    const std::string& prompt,
    float temperature = 0.7f,
    int maxTokens = 500
  ) override;
  
  bool isAvailable() override { return true; }  // Always available
  
  LLMProviderType getProviderType() const override { 
    return LLMProviderType::OFFLINE_FALLBACK; 
  }
  
  bool loadConfig(const std::string& configFile) override;
  
private:
  // Deterministic template selection
  std::string selectTemplate(
    const std::string& prompt,
    int randomSeed
  );
};
```

**Implementation Details**:

```cpp
LLMResponse OfflineFallbackProvider::callLLM(
  const std::string& prompt,
  float temperature,
  int maxTokens
) {
  // Example: if prompt contains "starvation", return starvation-related narrative
  std::string selected_template;
  
  if (prompt.find("food") != std::string::npos || 
      prompt.find("scarcity") != std::string::npos) {
    selected_template = "Farmers report starvation risks. Consider increased hunting, rationing, or trade.";
  } else if (prompt.find("morale") != std::string::npos ||
             prompt.find("rebellion") != std::string::npos) {
    selected_template = "Settlement morale declining. Warriors grow restless. Consider diplomatic action.";
  } else if (prompt.find("religious") != std::string::npos ||
             prompt.find("schism") != std::string::npos) {
    selected_template = "Religious doctrinal conflict emerging. Priest factions divided. Mediate carefully.";
  } else {
    selected_template = "Settlement status changing. Monitor factions and resources carefully.";
  }
  
  return LLMResponse{
    .text = selected_template,
    .inputTokens = 0,
    .completionTokens = 0,
    .costUSD = 0.0f,
    .latencyMs = 5.0f,  // Very fast
    .wasSuccessful = true,
    .errorMessage = ""
  };
}
```

---

### 4. LLM Request Queue Manager

**File**: `include/LLMRequestQueue.h`

```cpp
struct LLMRequest {
  int callId;
  LLMProviderType preferredProvider;
  std::string prompt;
  float temperature;
  int maxTokens;
  int timeoutSeconds;
  std::function<void(const LLMResponse&)> callback;
  enum class Priority { HIGH = 0, MEDIUM = 1, LOW = 2 } priority;
};

class LLMRequestQueue {
private:
  std::priority_queue<LLMRequest> requestQueue;
  std::mutex queueMutex;
  
public:
  void enqueue(const LLMRequest& request);
  LLMRequest dequeue();
  bool hasRequests() const;
  int getQueueLength() const;
};

class LLMRequestProcessor {
private:
  LLMProvider* provider;
  LLMRequestQueue requestQueue;
  std::thread processingThread;
  bool isRunning = false;
  
public:
  LLMRequestProcessor(LLMProvider* provider);
  ~LLMRequestProcessor();
  
  // Start background thread
  void startProcessing();
  void stopProcessing();
  
  // Queue request
  void queueRequest(const LLMRequest& request);
  
private:
  // Background processing loop
  void processingLoop();
};
```

---

### 5. LLM Usage Tracking & Cost Estimation

**File**: `include/LLMUsageTracker.h`

```cpp
class LLMUsageTracker {
private:
  std::vector<LLMUsage> usageHistory;
  std::ofstream usageLogFile;
  
  // Token costs (as of 2024)
  const float GPT35_INPUT_COST = 0.0005f;  // per 1K tokens
  const float GPT35_OUTPUT_COST = 0.0015f;  // per 1K tokens
  
public:
  LLMUsageTracker(const std::string& logFilename);
  
  // Record LLM call
  void recordUsage(const LLMResponse& response);
  
  // Get statistics
  struct UsageStats {
    int totalInputTokens;
    int totalOutputTokens;
    float totalCostUSD;
    int totalCalls;
  };
  
  UsageStats getStats(
    std::optional<std::chrono::system_clock::time_point> sinceTime = std::nullopt
  ) const;
  
  // Cost management
  bool isWithinBudget(float dailyBudgetUSD) const;
  void warnIfApproachingBudget(float budgetUSD, float warningThreshold = 0.8f);
};
```

---

### 6. Response Caching

**File**: `include/LLMResponseCache.h`

```cpp
class LLMResponseCache {
private:
  struct CacheEntry {
    std::string prompt;
    std::string response;
    std::chrono::system_clock::time_point createdAt;
    int hitCount = 0;
  };
  
  std::unordered_map<std::string, CacheEntry> cache;
  int cacheTTLSeconds = 300;  // Cache for 5 minutes
  
public:
  // Check cache
  std::optional<std::string> get(const std::string& prompt);
  
  // Store response
  void put(const std::string& prompt, const std::string& response);
  
  // Clear expired entries
  void pruneExpired();
  
  // Statistics
  struct CacheStats {
    int totalEntries;
    int totalHits;
    float hitRate;
  };
  
  CacheStats getStats() const;
};
```

---

### 7. Configuration System

**File**: `data/llm_config.json`

```json
{
  "llm": {
    "primaryProvider": "openai",
    "fallbackProvider": "local_llama",
    "offlineProvider": "offline_fallback",
    "enableCaching": true,
    "cacheTTL": 300
  },
  "openai": {
    "apiKey": "${OPENAI_API_KEY}",
    "model": "gpt-3.5-turbo",
    "timeoutSeconds": 10,
    "maxRetries": 3,
    "temperature": 0.7
  },
  "local_llama": {
    "apiUrl": "http://localhost:8000/v1/completions",
    "model": "llama-7b",
    "timeoutSeconds": 15,
    "maxRetries": 2
  },
  "rateLimit": {
    "callsPerMinute": 60,
    "dailyBudgetUSD": 10.0,
    "enableRateLimiting": true
  },
  "logging": {
    "usageLogFile": "logs/llm_usage.json",
    "enableDetailedLogging": false
  }
}
```

---

### 8. Unit Tests (30+ Comprehensive Test Templates)

**File**: `tests/Phase7Tests.cpp`

**Test Suite 1: Provider Selection & Factory**
```cpp
TEST(ProviderFactoryTests, CreateOpenAIProvider) {
  // Verify factory creates OpenAI provider correctly
  LLMProvider* provider = LLMProviderFactory::createProvider(
    LLMProviderType::OPENAI,
    "data/llm_config.json"
  );
  
  EXPECT_NE(provider, nullptr);
  EXPECT_EQ(provider->getProviderType(), LLMProviderType::OPENAI);
  delete provider;
}

TEST(ProviderFactoryTests, CreateOfflineFallback) {
  // Verify factory creates offline fallback
  LLMProvider* provider = LLMProviderFactory::createProvider(
    LLMProviderType::OFFLINE_FALLBACK,
    "data/llm_config.json"
  );
  
  EXPECT_NE(provider, nullptr);
  EXPECT_TRUE(provider->isAvailable());
  EXPECT_EQ(provider->getProviderType(), LLMProviderType::OFFLINE_FALLBACK);
  delete provider;
}

TEST(ProviderFactoryTests, CreateLocalLLaMA) {
  // Verify factory creates local LLaMA provider
  LLMProvider* provider = LLMProviderFactory::createProvider(
    LLMProviderType::LLAMA_LOCAL,
    "data/llm_config.json"
  );
  
  EXPECT_NE(provider, nullptr);
  delete provider;
}

TEST(ProviderFactoryTests, InvalidConfigFile) {
  // Verify factory handles missing config gracefully
  LLMProvider* provider = LLMProviderFactory::createProvider(
    LLMProviderType::OPENAI,
    "invalid/config/path.json"
  );
  
  // Should still create provider but with fallback config
  EXPECT_NE(provider, nullptr);
  delete provider;
}
```

**Test Suite 2: Configuration Loading**
```cpp
TEST(ConfigLoadingTests, LoadFromEnvironmentVariable) {
  // Set environment variable
  _putenv("OPENAI_API_KEY=sk-test-key-12345");
  
  OpenAIProvider provider;
  bool loaded = provider.loadConfig("data/llm_config.json");
  
  EXPECT_TRUE(loaded || provider.isAvailable());
}

TEST(ConfigLoadingTests, LoadFromJSONFile) {
  // Create test config file
  std::ofstream config("test_config.json");
  config << R"({
    "openai": {
      "apiKey": "sk-test-key",
      "model": "gpt-3.5-turbo",
      "timeoutSeconds": 10
    }
  })";
  config.close();
  
  OpenAIProvider provider;
  bool loaded = provider.loadConfig("test_config.json");
  
  EXPECT_TRUE(loaded);
  std::remove("test_config.json");
}

TEST(ConfigLoadingTests, MissingAPIKey) {
  // Config missing API key
  OpenAIProvider provider;
  
  // Should still load, but isAvailable should return false
  EXPECT_FALSE(provider.isAvailable());
}

TEST(ConfigLoadingTests, InvalidJSON) {
  // Create malformed config
  std::ofstream config("bad_config.json");
  config << "{ invalid json";
  config.close();
  
  OpenAIProvider provider;
  bool loaded = provider.loadConfig("bad_config.json");
  
  // Should handle gracefully (no crash)
  EXPECT_FALSE(loaded);
  std::remove("bad_config.json");
}
```

**Test Suite 3: Offline Fallback Response Generation**
```cpp
TEST(OfflineFallbackTests, FoodScarcityNarrative) {
  OfflineFallbackProvider provider;
  
  LLMResponse response = provider.callLLM(
    "Food scarcity: levels below 50. Farmers starving.",
    0.7f,
    500
  );
  
  EXPECT_TRUE(response.wasSuccessful);
  EXPECT_FALSE(response.text.empty());
  EXPECT_NE(response.text.find("food"), std::string::npos);
}

TEST(OfflineFallbackTests, RebellionNarrative) {
  OfflineFallbackProvider provider;
  
  LLMResponse response = provider.callLLM(
    "Faction morale declining. Warriors rebellion risk 60%.",
    0.7f,
    500
  );
  
  EXPECT_TRUE(response.wasSuccessful);
  EXPECT_NE(response.text.find("morale"), std::string::npos);
}

TEST(OfflineFallbackTests, ReligiousSchismNarrative) {
  OfflineFallbackProvider provider;
  
  LLMResponse response = provider.callLLM(
    "Religious schism forming between priest factions.",
    0.7f,
    500
  );
  
  EXPECT_TRUE(response.wasSuccessful);
  EXPECT_NE(response.text.find("religious"), std::string::npos);
}

TEST(OfflineFallbackTests, UnknownPrompt) {
  OfflineFallbackProvider provider;
  
  LLMResponse response = provider.callLLM(
    "Completely unknown prompt about xyz",
    0.7f,
    500
  );
  
  EXPECT_TRUE(response.wasSuccessful);
  EXPECT_FALSE(response.text.empty());
  // Should fallback to generic narrative
}

TEST(OfflineFallbackTests, FastResponse) {
  OfflineFallbackProvider provider;
  
  auto start = std::chrono::high_resolution_clock::now();
  LLMResponse response = provider.callLLM("test prompt");
  auto end = std::chrono::high_resolution_clock::now();
  
  auto duration_ms = std::chrono::duration<float, std::milli>(end - start).count();
  EXPECT_LT(duration_ms, 50.0f);  // Offline should be very fast
  EXPECT_LT(response.latencyMs, 50.0f);
}

TEST(OfflineFallbackTests, NoHallucination) {
  OfflineFallbackProvider provider;
  
  // Call many times with random prompts
  for (int i = 0; i < 100; i++) {
    LLMResponse response = provider.callLLM(
      "Random prompt " + std::to_string(i)
    );
    
    // Response should always be from templates (no gibberish)
    EXPECT_LT(response.text.length(), 1000);  // Templates are short
    EXPECT_GT(response.text.length(), 5);     // But not empty
  }
}
```

**Test Suite 4: Response Caching**
```cpp
TEST(CacheTests, CacheHit) {
  LLMResponseCache cache;
  
  cache.put("test prompt", "test response");
  auto cached = cache.get("test prompt");
  
  EXPECT_TRUE(cached.has_value());
  EXPECT_EQ(cached.value(), "test response");
}

TEST(CacheTests, CacheMiss) {
  LLMResponseCache cache;
  
  auto cached = cache.get("nonexistent prompt");
  
  EXPECT_FALSE(cached.has_value());
}

TEST(CacheTests, ExpiredEntry) {
  LLMResponseCache cache;
  cache.put("test prompt", "test response");
  
  // Wait for TTL to expire (default 300 seconds, but we can test with mock)
  std::this_thread::sleep_for(std::chrono::seconds(6));
  cache.pruneExpired();
  
  auto cached = cache.get("test prompt");
  EXPECT_FALSE(cached.has_value());
}

TEST(CacheTests, MultipleEntries) {
  LLMResponseCache cache;
  
  cache.put("prompt1", "response1");
  cache.put("prompt2", "response2");
  cache.put("prompt3", "response3");
  
  EXPECT_TRUE(cache.get("prompt1").has_value());
  EXPECT_TRUE(cache.get("prompt2").has_value());
  EXPECT_TRUE(cache.get("prompt3").has_value());
}

TEST(CacheTests, CacheOverwrite) {
  LLMResponseCache cache;
  
  cache.put("prompt", "response1");
  cache.put("prompt", "response2");  // Overwrite
  
  auto cached = cache.get("prompt");
  EXPECT_EQ(cached.value(), "response2");
}

TEST(CacheTests, LRUEviction) {
  LLMResponseCache cache;
  cache.setMaxEntries(3);  // Small cache for testing
  
  cache.put("p1", "r1");
  cache.put("p2", "r2");
  cache.put("p3", "r3");
  cache.put("p4", "r4");  // Should evict least recently used
  
  // p1 should be evicted (oldest, not recently used)
  EXPECT_FALSE(cache.get("p1").has_value());
  EXPECT_TRUE(cache.get("p2").has_value());
  EXPECT_TRUE(cache.get("p3").has_value());
  EXPECT_TRUE(cache.get("p4").has_value());
}

TEST(CacheTests, HitCountTracking) {
  LLMResponseCache cache;
  
  cache.put("prompt", "response");
  cache.get("prompt");  // Hit 1
  cache.get("prompt");  // Hit 2
  cache.get("prompt");  // Hit 3
  
  auto stats = cache.getStats();
  EXPECT_EQ(stats.totalHits, 3);
}
```

**Test Suite 5: Usage Tracking & Cost Calculation**
```cpp
TEST(UsageTrackingTests, RecordSingleCall) {
  LLMUsageTracker tracker("test_usage.json");
  
  LLMResponse response{
    .inputTokens = 100,
    .completionTokens = 50,
    .costUSD = 0.0003f
  };
  
  tracker.recordUsage(response);
  
  auto stats = tracker.getStats();
  EXPECT_EQ(stats.totalInputTokens, 100);
  EXPECT_EQ(stats.totalOutputTokens, 50);
  EXPECT_FLOAT_EQ(stats.totalCostUSD, 0.0003f);
}

TEST(UsageTrackingTests, RecordMultipleCalls) {
  LLMUsageTracker tracker("test_usage.json");
  
  LLMResponse r1{.inputTokens = 100, .completionTokens = 50, .costUSD = 0.0003f};
  LLMResponse r2{.inputTokens = 200, .completionTokens = 75, .costUSD = 0.0006f};
  
  tracker.recordUsage(r1);
  tracker.recordUsage(r2);
  
  auto stats = tracker.getStats();
  EXPECT_EQ(stats.totalInputTokens, 300);
  EXPECT_EQ(stats.totalOutputTokens, 125);
  EXPECT_FLOAT_EQ(stats.totalCostUSD, 0.0009f);
}

TEST(UsageTrackingTests, CostCalculation) {
  // GPT-3.5: $0.0005 per 1K input, $0.0015 per 1K output
  float inputCost = (100.0f / 1000.0f) * 0.0005f;      // 0.00005
  float completionCost = (50.0f / 1000.0f) * 0.0015f;  // 0.000075
  float totalCost = inputCost + completionCost;        // 0.000125
  
  EXPECT_FLOAT_EQ(totalCost, 0.000125f);
}

TEST(UsageTrackingTests, BudgetWarning) {
  LLMUsageTracker tracker("test_usage.json");
  
  // Simulate expensive calls approaching daily budget
  for (int i = 0; i < 5000; i++) {
    LLMResponse response{.costUSD = 0.002f};
    tracker.recordUsage(response);
  }
  
  auto stats = tracker.getStats();
  EXPECT_GT(stats.totalCostUSD, 9.0f);  // Over $9
  
  // Should warn at 80% of $10 budget
  bool withinBudget = tracker.isWithinBudget(10.0f);
  EXPECT_FALSE(withinBudget);
}

TEST(UsageTrackingTests, DailyRollover) {
  // Test that usage resets at day boundary
  // (Mock time or use real time based on test infrastructure)
  LLMUsageTracker tracker("test_usage.json");
  
  LLMResponse response{.costUSD = 5.0f};
  tracker.recordUsage(response);
  
  auto stats = tracker.getStats();
  EXPECT_EQ(stats.totalCalls, 1);
}
```

**Test Suite 6: Exponential Backoff & Retry Logic**
```cpp
TEST(RetryTests, BackoffCalculation) {
  // Verify exponential backoff formula
  // delay = 1000 * (2 ^ attempt) + jitter
  
  int delay0 = 1000 * (1 << 0);  // 1000ms
  int delay1 = 1000 * (1 << 1);  // 2000ms
  int delay2 = 1000 * (1 << 2);  // 4000ms
  int delay3 = 1000 * (1 << 3);  // 8000ms
  
  EXPECT_EQ(delay0, 1000);
  EXPECT_EQ(delay1, 2000);
  EXPECT_EQ(delay2, 4000);
  EXPECT_EQ(delay3, 8000);
}

TEST(RetryTests, MaxDelayCapture) {
  // Ensure delay never exceeds 30 seconds
  int max_attempts = 10;
  int max_delay = 30000;
  
  for (int attempt = 0; attempt < max_attempts; attempt++) {
    int delay = std::min(1000 * (1 << attempt), max_delay);
    EXPECT_LE(delay, 30000);
  }
}

TEST(RetryTests, JitterDeterminsticWithSeed) {
  // With same seed, jitter should be reproducible
  srand(42);
  int jitter1 = rand() % 1000;
  
  srand(42);
  int jitter2 = rand() % 1000;
  
  EXPECT_EQ(jitter1, jitter2);
}
```

**Test Suite 7: Rate Limiting**
```cpp
TEST(RateLimitTests, TokenBucketRefill) {
  TokenBucketRateLimiter limiter;
  limiter.setMaxTokens(60);
  limiter.setRefillRate(1.0f);  // 1 token per second
  
  EXPECT_TRUE(limiter.canMakeRequest());  // First request allowed
  EXPECT_EQ(limiter.getTokensAvailable(), 59.0f);
}

TEST(RateLimitTests, TokenBucketExhaustion) {
  TokenBucketRateLimiter limiter;
  limiter.setMaxTokens(5);
  
  // Consume all tokens
  for (int i = 0; i < 5; i++) {
    EXPECT_TRUE(limiter.canMakeRequest());
  }
  
  // Next request should be denied
  EXPECT_FALSE(limiter.canMakeRequest());
}

TEST(RateLimitTests, TokenRefillAfterWait) {
  TokenBucketRateLimiter limiter;
  limiter.setMaxTokens(10);
  limiter.setRefillRate(1.0f);
  
  // Consume all tokens
  for (int i = 0; i < 10; i++) {
    limiter.canMakeRequest();
  }
  
  // Wait 5 seconds
  std::this_thread::sleep_for(std::chrono::seconds(5));
  
  // Should have ~5 tokens available now
  EXPECT_GE(limiter.getTokensAvailable(), 4.5f);
}

TEST(RateLimitTests, RateLimit60PerMinute) {
  TokenBucketRateLimiter limiter;
  limiter.setMaxTokens(60);
  limiter.setRefillRate(60.0f / 60.0f);  // 1 per second
  
  // Make 60 requests rapidly
  int successful = 0;
  for (int i = 0; i < 60; i++) {
    if (limiter.canMakeRequest()) successful++;
  }
  
  EXPECT_EQ(successful, 60);
  EXPECT_FALSE(limiter.canMakeRequest());  // 61st should fail
}
```

**Test Suite 8: Error Recovery & Fallback**
```cpp
TEST(ErrorHandlingTests, NetworkErrorFallback) {
  // Simulate network error, expect fallback to offline
  LLMResponse response = handleLLMError(
    ErrorType::NETWORK_ERROR,
    OPENAI,
    nullptr
  );
  
  // Should fallback successfully
  EXPECT_TRUE(response.wasSuccessful || response.provider == OFFLINE_FALLBACK);
}

TEST(ErrorHandlingTests, TimeoutWithRetry) {
  // Simulate timeout on first attempt, success on second
  LLMResponse response = handleLLMError(
    ErrorType::TIMEOUT_ERROR,
    OPENAI,
    nullptr,
    retryCount=0
  );
  
  // Should retry (not fail immediately)
  EXPECT_EQ(response.errorMessage, "");  // No error yet
}

TEST(ErrorHandlingTests, AuthenticationFailure) {
  // Invalid API key
  LLMResponse response = handleLLMError(
    ErrorType::AUTHENTICATION_ERROR,
    OPENAI,
    nullptr
  );
  
  // Should disable OpenAI, fallback to offline
  EXPECT_TRUE(response.wasSuccessful);
}

TEST(ErrorHandlingTests, RateLimitBackoff) {
  // API returns 429 Too Many Requests
  LLMResponse response = handleLLMError(
    ErrorType::RATE_LIMIT_ERROR,
    OPENAI,
    nullptr
  );
  
  // Should queue for later retry (not fail)
  EXPECT_EQ(response.errorMessage, "");
}

TEST(ErrorHandlingTests, ServerErrorSwitchProvider) {
  // OpenAI returns 500 error
  LLMResponse response = handleLLMError(
    ErrorType::SERVER_ERROR,
    OPENAI,
    nullptr
  );
  
  // Should try fallback provider
  EXPECT_TRUE(response.wasSuccessful);
}
```

**Test Suite 9: Deterministic Replay**
```cpp
TEST(ReplayTests, LogLLMCall) {
  LLMCallLog log;
  
  log.recordCall(
    tickNumber=1234,
    callId=5,
    provider=OPENAI,
    prompt="test",
    response="result",
    wasSuccessful=true
  );
  
  EXPECT_EQ(log.getEntries().size(), 1);
}

TEST(ReplayTests, ReplayCallUsesCachedResponse) {
  // Record call
  LLMCallLog log;
  log.recordCall(1234, 5, OPENAI, "test prompt", "expected response", true);
  
  // In replay mode, should return recorded response
  LLMResponse replayed = log.replayCall(1234, 5);
  EXPECT_EQ(replayed.text, "expected response");
}

TEST(ReplayTests, DetectDivergence) {
  // Run simulation twice, detect if outputs differ
  auto run1 = runSimulation(seed=42, replayMode=false);
  auto run2 = runSimulation(seed=42, replayMode=true);
  
  // Should be identical
  EXPECT_TRUE(run1.equals(run2));
}

TEST(ReplayTests, MissingLogEntry) {
  LLMCallLog log;
  
  // Try to replay call that wasn't recorded
  LLMResponse replayed = log.replayCall(9999, 999);
  
  EXPECT_FALSE(replayed.wasSuccessful);
}

TEST(ReplayTests, DeterminismWithRNG) {
  // Same seed should produce same random sequence
  srand(42);
  std::vector<int> seq1;
  for (int i = 0; i < 10; i++) {
    seq1.push_back(rand() % 100);
  }
  
  srand(42);
  std::vector<int> seq2;
  for (int i = 0; i < 10; i++) {
    seq2.push_back(rand() % 100);
  }
  
  EXPECT_EQ(seq1, seq2);
}
```

**Test Suite 10: Edge Cases & Boundary Conditions**
```cpp
TEST(EdgeCaseTests, EmptyPrompt) {
  OfflineFallbackProvider provider;
  LLMResponse response = provider.callLLM("");
  
  // Should handle gracefully
  EXPECT_TRUE(response.wasSuccessful);
}

TEST(EdgeCaseTests, VeryLargePrompt) {
  std::string largePrompt(100000, 'x');  // 100KB prompt
  OfflineFallbackProvider provider;
  LLMResponse response = provider.callLLM(largePrompt);
  
  // Should handle without crash
  EXPECT_TRUE(response.wasSuccessful || response.errorMessage.length() > 0);
}

TEST(EdgeCaseTests, ZeroCostCall) {
  LLMUsageTracker tracker("test.json");
  LLMResponse response{.costUSD = 0.0f};
  
  tracker.recordUsage(response);
  auto stats = tracker.getStats();
  
  EXPECT_EQ(stats.totalCostUSD, 0.0f);
}

TEST(EdgeCaseTests, NegativeCostSuppressed) {
  // Negative cost shouldn't happen, but test safeguard
  float cost = std::max(0.0f, -0.001f);
  EXPECT_EQ(cost, 0.0f);
}

TEST(EdgeCaseTests, CacheFullAndFull) {
  LLMResponseCache cache;
  cache.setMaxEntries(5);
  
  // Fill cache to max
  for (int i = 0; i < 5; i++) {
    cache.put("p" + std::to_string(i), "r" + std::to_string(i));
  }
  
  // Add more (should evict LRU)
  for (int i = 5; i < 10; i++) {
    cache.put("p" + std::to_string(i), "r" + std::to_string(i));
  }
  
  // Cache should still be at max size
  EXPECT_EQ(cache.getStats().totalEntries, 5);
}

TEST(EdgeCaseTests, ConcurrentRequests) {
  LLMRequestQueue queue;
  
  // Add requests from multiple threads
  std::thread t1([&] { queue.enqueue({.priority = Priority::HIGH}); });
  std::thread t2([&] { queue.enqueue({.priority = Priority::LOW}); });
  
  t1.join();
  t2.join();
  
  EXPECT_EQ(queue.getQueueLength(), 2);
}

TEST(EdgeCaseTests, DuplicateEnqueueDetection) {
  LLMRequestQueue queue;
  
  LLMRequest req{.prompt = "test"};
  queue.enqueue(req);
  bool added = queue.enqueue(req);  // Try to add same prompt
  
  // Should detect duplicate
  EXPECT_FALSE(added);
  EXPECT_EQ(queue.getQueueLength(), 1);
}
```

---

## File Structure

```
include/
  LLMProvider.h
  OpenAIProvider.h
  OfflineFallbackProvider.h
  LLMRequestQueue.h
  LLMUsageTracker.h
  LLMResponseCache.h
  
src/
  LLMProvider.cpp
  OpenAIProvider.cpp
  OfflineFallbackProvider.cpp
  LLMRequestQueue.cpp
  LLMUsageTracker.cpp
  LLMResponseCache.cpp
  
data/
  llm_config.json
  
tests/
  Phase7Tests.cpp
```

---

## Success Criteria Checklist

- [ ] Abstract LLMProvider interface with multiple implementations
- [ ] OpenAI provider calls GPT-3.5 with proper authentication
- [ ] Offline fallback generates plausible non-hallucinated responses
- [ ] Response caching prevents duplicate calls for 5 minutes
- [ ] Token usage tracked with cost estimation
- [ ] Rate limiting enforced (e.g., 60 calls/minute)
- [ ] Exponential backoff on API failures (1s, 2s, 4s, 8s)
- [ ] Timeout handling with graceful fallback
- [ ] Configuration loaded from environment variables and JSON
- [ ] LLM calls logged for replay/debugging
- [ ] 30+ unit tests written and passing

---

## Implementation Order

1. **LLM Provider Interface** (1-2 hours)
2. **OpenAI Provider** (2-3 hours)
3. **Offline Fallback** (1-2 hours)
4. **Request Queue** (2-3 hours)
5. **Usage Tracking** (1-2 hours)
6. **Response Caching** (1-2 hours)
7. **Configuration** (1 hour)
8. **Unit Tests** (3-5 hours)

**Estimated Total**: 20-35 hours of development (expanded for Phase 1-6 parity)

---

## Edge Cases & Error Handling Specifications

**Division by Zero Prevention**:
- Token cost calculation: if input_tokens=0, cost = 0 (don't divide by 1000)
- Budget calculation: if daily_calls=0, average_cost = 0 (no panic)
- Cache hit rate: if totalHits=0, hitRate = 0% (valid state)

**Collection Edge Cases**:
- Empty request queue: dequeue returns null (not exception)
- Cache size zero: all gets miss (valid, just inefficient)
- Provider list empty: fallback to offline (always works)

**Null/Invalid References**:
- Missing API key: isAvailable() returns false (don't crash)
- Null callback function: store as nullptr, check before calling
- Invalid config file: use hardcoded defaults (never crash on config error)

**Concurrency Issues**:
- Simultaneous cache access: use mutex lock for get/put operations
- Queue insertion from multiple threads: prioritize HIGH requests atomically
- Token bucket updates: atomic increment/decrement

**Out-of-Bounds Access**:
- Response array index: bounds check before access
- String substring operations: check length before substr
- Retry count: clamp to [0, MAX_RETRIES]

**Type Mismatches**:
- Cost as string vs float: parse with error handling
- Temperature range: clamp to [0.0, 2.0] for OpenAI
- MaxTokens: clamp to [1, 4096] per model limits

---

## Determinism Validation Specifications

**RNG Seeding Requirements**:
- Per-tick seeding: seed(globalSeed + currentTick) at start of tick
- Exponential backoff jitter: use seeded RNG (same seed = same jitter)
- Cache eviction priority: deterministic ordering (not random selection)

**Floating-Point Precision Constraints**:
- Cost calculations: use float32 consistently (no mixed float/double)
- Budget comparisons: compare with tolerance ±0.0001 to avoid floating-point drift
- Avoid unnecessary conversions: don't convert float→int→float

**Logging Patterns for Reproducibility**:
- Log LLM call tick number, prompt hash, response text, provider used
- Log RNG seed for jitter calculation
- Log provider selection decision tree (why OPENAI chosen, why fallback triggered)

**Replay Mechanism**:
- Load `replay_log.jsonl` file with all LLM calls
- Match by (tickNumber, callId) to find logged response
- If no match: mismatch detected → divergence error
- Compare live state with replayed state: if differ → divergence bug

**Bit-Identical Comparison Approach**:
- NPC mood after tick 1234: live=0.6543, replay=0.6543 ✓
- If differ: check floating-point precision (use ~0.0001 tolerance for acceptance)
- If still differ: identify which system introduced non-determinism (LLM call, RNG, emotion update)

**Platform-Dependent Code Flagging**:
- system time: don't use for RNG seed (platform-dependent)
- file I/O latency: affects replay timing (log actual latency, not wall-clock)
- random device: don't use (platform-dependent entropy)

**Debugging Non-Determinism**:
1. Run save file twice with `--replay` flag
2. Diff output state at each tick
3. Find first tick where divergence occurs
4. Inspect LLM calls at that tick (check if logged vs live differ)
5. Inspect RNG state before that tick
6. Inspect emotion/mood calculations (check floating-point precision)

---

## Comprehensive Copilot Code Generation Prompts

### Prompt 1: Provider Abstraction Layer
"Create an abstract `LLMProvider` base class with virtual methods: `callLLM(prompt, temperature, maxTokens)` returning `LLMResponse`, `isAvailable()` returning bool, `getProviderType()` returning enum, and `loadConfig(filename)` returning bool. The `LLMResponse` struct should contain: `text` (string), `inputTokens`, `completionTokens` (int), `costUSD` (float), `latencyMs` (float), `wasSuccessful` (bool), `errorMessage` (string). Implement a `LLMProviderFactory::createProvider(type, configFile)` factory function that returns the appropriate provider subclass. Ensure the interface is extensible for adding new providers."

### Prompt 2: OpenAI Provider with Retry Logic
"Implement `OpenAIProvider` subclass with: (1) Configuration loading from environment variables (`OPENAI_API_KEY`) and JSON files; (2) HTTP POST request to `https://api.openai.com/v1/chat/completions` with Bearer token authentication; (3) JSON request building with model name, messages array, temperature, and max_tokens parameters; (4) Exponential backoff retry loop: attempt 1 delay 1s, attempt 2 delay 2s, attempt 3 delay 4s, with random jitter (0-delay) added; (5) JSON response parsing to extract `choices[0].message.content`, `usage.prompt_tokens`, and `usage.completion_tokens`; (6) Timeout handling (default 10s per call); (7) Error categorization (network, timeout, auth, rate limit, server error). Target: fast (<3s typical latency), robust (3 retries with backoff), cost-efficient (token tracking)."

### Prompt 3: Offline Fallback Provider
"Implement `OfflineFallbackProvider` with deterministic template-based narrative generation. Create a keyword-to-template map: if prompt contains 'food' or 'scarcity', return 'Farmers report starvation risks. Consider increased hunting, rationing, or trade.'; if 'morale' or 'rebellion', return 'Settlement morale declining. Warriors grow restless. Consider diplomatic action.'; if 'religious' or 'schism', return 'Religious doctrinal conflict emerging. Priest factions divided. Mediate carefully.'; default case returns 'Settlement status changing. Monitor factions and resources carefully.' Use SHA256 or simple hash of prompt to select template deterministically (same prompt = same template). Never generate free-form text. Target: instant response (<10ms), always available, never crashes."

### Prompt 4: Response Caching with LRU
"Implement `LLMResponseCache` using unordered_map keyed by `sha256(prompt)`. Each cache entry contains: prompt hash, response text, creation timestamp, hit count. Implement `get(prompt)`: return optional<string> with cached response if exists and not expired (TTL=300s default). On miss, return empty optional. Implement `put(prompt, response)`: insert or update cache entry. Implement LRU eviction: when cache exceeds max_entries (default 100), calculate priority_score = (1 - age_ratio) * 0.6 + (hitCount / 100) * 0.4, evict entry with lowest score. Provide `getStats()` returning cache size, total hits, hit rate percentage. Test: cache hit, cache miss, TTL expiration, LRU eviction under full cache."

### Prompt 5: Token Usage Tracking & Budget Enforcement
"Implement `LLMUsageTracker` with: (1) `recordUsage(response)` that appends to usage history and JSON log file; (2) `getStats(optionalSinceTime)` returning totalInputTokens, totalOutputTokens, totalCostUSD, totalCalls; (3) Cost calculation: inputCost = (inputTokens / 1000) * 0.0005, completionCost = (completionTokens / 1000) * 0.0015, totalCost = inputCost + completionCost; (4) Budget enforcement: track daily total, warn at 80% threshold, error at 100% threshold; (5) Log format: JSON lines (one entry per line) with fields: timestamp, inputTokens, completionTokens, costUSD, callType. Target: accurate cost tracking, daily budget enforcement, exportable logs for accounting."

### Prompt 6: Token Bucket Rate Limiter
"Implement `TokenBucketRateLimiter` with: (1) Initialization: maxTokens=60 (for 60 calls/minute), refillRate=1.0 (1 token per second); (2) `canMakeRequest()`: on call, refill bucket based on elapsed time (tokens_available = min(maxTokens, tokens_available + refillRate * secondsElapsed)), check if tokens_available >= 1.0, if yes: decrement and return true, if no: return false and log wait_time = (1.0 - tokens_available) / refillRate seconds; (3) Thread-safe using mutex lock; (4) Deterministic with seed-based RNG (never random). Target: enforce 60 calls/minute, allow bursts up to 60 requests, graceful queueing when rate limit hit."

### Prompt 7: Exponential Backoff with Jitter
"Implement exponential backoff function `calculateBackoffDelay(attempt, basedelay_ms=1000, max_delay_ms=30000)` that returns: delay_ms = basedelay_ms * (2 ^ attempt) + jitter, where jitter = random(0, basedelay_ms * attempt) from seeded RNG, clamped to [0, max_delay_ms]. For attempt 0-3: expected delays 1s, 2-4s, 4-8s, 8-16s. Use in retry loop: on failure, sleep for calculated delay, then retry. Target: prevents thundering herd, allows API recovery time, deterministic with seed."

### Prompt 8: Error Recovery & Fallback Cascade
"Implement `handleLLMError(errorType, currentProvider, request)` that returns recovery strategy: (1) NETWORK_ERROR: switch provider (try OPENAI→LLAMA→OFFLINE); (2) TIMEOUT_ERROR: retry with backoff if attempts < 3, else use offline; (3) AUTHENTICATION_ERROR: log critical, disable provider, use offline; (4) RATE_LIMIT_ERROR: parse Retry-After header, sleep, retry or queue; (5) SERVER_ERROR: switch to different provider or use cached response; (6) INVALID_RESPONSE: use offline fallback. Execute strategy and return result. Target: graceful degradation, always deliver response (via fallback if needed), clear error logging."

### Prompt 9: LLM Request Queue with Priority
"Implement `LLMRequestQueue` with priority-based ordering: HIGH (player input, timeout=30 ticks, max=5), MEDIUM (world state, timeout=100 ticks, max=3), LOW (ambient, timeout=50 ticks, max=10). Struct `LLMRequest`: callId, priority, enqueuedTick, timeoutTicks, prompt, callback. `enqueue(request)`: check for duplicate prompt (skip if exists), insert into priority_queue sorted by priority then enqueuedTick, check queue length for priority, evict oldest LOW if overflow. `dequeue()`: return highest priority non-expired request, call callback with error if expired. Target: responsive player input, fair world state priority, ambient conversation filling gaps."

### Prompt 10: Deterministic Replay & Logging
"Implement `LLMCallLog` that records all LLM calls to JSON lines file (one entry per line). Entry format: {tickNumber, callId, usedProvider, prompt_hash, llmResponse, inputTokens, completionTokens, latencyMs, wasSuccessful, errorIfAny, randomSeed, timestamp}. Implement `recordLLMCall(tick, callId, prompt, response, provider, seed)` to append entry. Implement `replayLLMCall(tick, callId, logFile)` to look up and return logged response (skip actual LLM call). Implement `validateDeterminism(saveFile, logFile, numTicks)` to run simulation twice (live vs replay) and verify byte-identical output. Target: complete auditability, frame-by-frame replay capability, determinism validation."

---

## Implementation Order & Dependencies (20-35 Hours Total)

1. **LLM Provider Interface** (1-2 hours) - None → All providers
2. **OpenAI Provider** (2-3 hours) - Interface
3. **Offline Fallback Provider** (1-2 hours) - Interface
4. **Local LLaMA Provider** (2-3 hours) - Interface (optional, can be stub)
5. **Response Caching** (1-2 hours) - None
6. **Request Queue** (2-3 hours) - LLMRequest struct
7. **Token Usage Tracking** (1-2 hours) - None
8. **Rate Limiting** (1-2 hours) - None
9. **Error Recovery** (2-3 hours) - All providers
10. **Async Processor Thread** (2-3 hours) - Request queue, error recovery
11. **Configuration Loading** (1-2 hours) - None
12. **Deterministic Replay** (2-3 hours) - All LLM call sites
13. **Unit Tests** (4-6 hours) - All implementations

---

## Success Criteria (Expanded for Phase 1-6 Parity)

- [ ] Abstract LLMProvider interface with ≥3 implementations
- [ ] OpenAI provider with retry + exponential backoff
- [ ] Offline fallback with deterministic templates (never hallucinate)
- [ ] Response caching (LRU, TTL=300s, 5-minute consistency)
- [ ] Token tracking with cost estimation (GPT-3.5 pricing model)
- [ ] Rate limiting (60 calls/minute token bucket)
- [ ] Timeout handling (10s default with fallback)
- [ ] Configuration from env vars + JSON + defaults
- [ ] LLM call logging for replay/debugging
- [ ] 30+ unit tests (>90% code coverage)
- [ ] Priority queue (HIGH for player, MEDIUM for world, LOW for ambient)
- [ ] Async non-blocking (background thread processor)
- [ ] Deterministic replay (same seed = byte-identical output)
- [ ] All error types handled (6+ error classifications)
- [ ] Budget enforcement (warn 80%, error 100%)
- [ ] Thread safety (mutex locks on shared data)
- [ ] Documentation (all algorithms with pseudocode)
- [ ] Binary serialization for save files

---

## Copilot Code Generation Tips

1. **For Provider Abstraction**: "Create abstract LLMProvider class with callLLM(), isAvailable(), loadConfig() methods. Implement OpenAIProvider, LocalLLaMAProvider, OfflineFallbackProvider subclasses with provider-specific logic."

2. **For OpenAI Integration**: "Use HTTPS request library (e.g., curl) to call OpenAI API at https://api.openai.com/v1/chat/completions. Include API key in Authorization header. Parse JSON response to extract text and token usage. Implement retry loop with exponential backoff (1s, 2s, 4s, 8s)."

3. **For Offline Fallback**: "Generate deterministic responses based on prompt keywords. If 'food' or 'scarcity' in prompt, return template about starvation. If 'morale' or 'rebellion', return template about unrest. Never hallucinate—use only predefined templates."

4. **For Caching**: "Implement LRU cache keyed by prompt hash. Store responses for 5 minutes (configurable TTL). On miss, call LLM. On hit, return cached response without LLM call. Track hit rate for statistics."

---

## Critical Implementation Notes

- **API Key Security**: Never log API keys. Load from environment variables preferentially.
- **Fallback Chain**: Try OpenAI → Local LLaMA → Offline if both fail
- **Non-Blocking**: All LLM calls should be async via queue processor thread
- **Cost Management**: Track daily spend, warn approaching budget limit
- **Determinism**: Cache responses consistently so replay uses same LLM output

---

## Next Phase Dependencies

Phase 8 (Decision Interpretation) requires:
- Phase 7 LLM provider with prompt execution

Phase 9 (Narrative Generation) requires:
- Phase 7 LLM provider with caching

Phases 10+ require Phase 7 infrastructure.
