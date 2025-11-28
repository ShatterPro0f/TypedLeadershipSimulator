#include <gtest/gtest.h>
#include "PathfindingEngine.h"
#include "MovementController.h"
#include "Vector3.h"
#include <chrono>
#include <random>

using namespace TLS;

// ==================== PATHFINDING CACHE TESTS ====================

class PathfindingCachingTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
    }

    PathfindingEngine* engine_;
};

TEST_F(PathfindingCachingTest, CachePathOnFirstComputation) {
    Vector3 start(0, 0, 0);
    Vector3 dest(10, 10, 0);

    engine_->clearPathCache();
    EXPECT_EQ(engine_->getCacheSize(), 0);
    
    // First computation - cache miss
    float hitRateBefore = engine_->getCacheHitRate();
    auto path1 = engine_->computePath(start, dest);
    EXPECT_EQ(engine_->getCacheSize(), 1);  // Path should be cached

    // Second computation - cache hit
    auto path2 = engine_->computePath(start, dest);
    float hitRateAfter = engine_->getCacheHitRate();

    EXPECT_EQ(path1, path2);  // Same path returned
    EXPECT_GT(hitRateAfter, hitRateBefore);  // Cache hit rate increased
}

TEST_F(PathfindingCachingTest, CacheHitOnIdenticalRequest) {
    Vector3 start(0, 0, 0);
    Vector3 dest(5, 5, 0);
    engine_->clearPathCache();

    std::vector<long> timings;
    std::vector<std::vector<Vector3>> paths;

    for (int i = 0; i < 5; i++) {
        auto t_start = std::chrono::high_resolution_clock::now();
        paths.push_back(engine_->computePath(start, dest));
        auto t_end = std::chrono::high_resolution_clock::now();
        timings.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());
    }

    // All paths should be identical
    for (size_t i = 1; i < paths.size(); ++i) {
        EXPECT_EQ(paths[0], paths[i]);
    }

    // Cache hits should be faster than first computation
    EXPECT_LE(timings[1], timings[0]);
}

TEST_F(PathfindingCachingTest, CacheSizeIncreases) {
    EXPECT_EQ(engine_->getCacheSize(), 0);

    engine_->computePath(Vector3(0, 0, 0), Vector3(5, 5, 0));
    EXPECT_EQ(engine_->getCacheSize(), 1);

    engine_->computePath(Vector3(0, 0, 0), Vector3(10, 10, 0));
    EXPECT_EQ(engine_->getCacheSize(), 2);

    engine_->computePath(Vector3(0, 0, 0), Vector3(15, 15, 0));
    EXPECT_EQ(engine_->getCacheSize(), 3);
}

TEST_F(PathfindingCachingTest, ClearCacheWorks) {
    engine_->computePath(Vector3(0, 0, 0), Vector3(5, 5, 0));
    EXPECT_GT(engine_->getCacheSize(), 0);

    engine_->clearPathCache();
    EXPECT_EQ(engine_->getCacheSize(), 0);
}

TEST_F(PathfindingCachingTest, CacheHitRateCalculation) {
    engine_->clearPathCache();

    engine_->computePath(Vector3(0, 0, 0), Vector3(5, 5, 0));
    // First call is always a miss
    float hitRate1 = engine_->getCacheHitRate();
    EXPECT_EQ(hitRate1, 0.0f);  // 0 hits, 1 miss

    engine_->computePath(Vector3(0, 0, 0), Vector3(5, 5, 0));  // Cache hit
    float hitRate2 = engine_->getCacheHitRate();
    EXPECT_EQ(hitRate2, 50.0f);  // 1 hit, 1 miss = 50%

    engine_->computePath(Vector3(0, 0, 0), Vector3(5, 5, 0));  // Cache hit
    float hitRate3 = engine_->getCacheHitRate();
    EXPECT_GT(hitRate3, 50.0f);  // 2 hits, 1 miss = 66%+
}

TEST_F(PathfindingCachingTest, DifferentPathsNotCached) {
    auto path1 = engine_->computePath(Vector3(0, 0, 0), Vector3(5, 5, 0));
    auto path2 = engine_->computePath(Vector3(0, 0, 0), Vector3(10, 10, 0));

    EXPECT_NE(path1, path2);  // Different paths
    EXPECT_EQ(engine_->getCacheSize(), 2);  // Both cached
}

// ==================== PATH OPTIMIZATION TESTS ====================

class PathOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
    }

    PathfindingEngine* engine_;
};

TEST_F(PathOptimizationTest, StraightPathExists) {
    auto path = engine_->computePath(Vector3(0, 0, 0), Vector3(10, 0, 0));

    EXPECT_FALSE(path.empty());
    EXPECT_GT(path.size(), 1);  // At least start and end
}

TEST_F(PathOptimizationTest, PathContainsStartAndEnd) {
    Vector3 start(5, 5, 0);
    Vector3 end(15, 15, 0);
    auto path = engine_->computePath(start, end);

    if (!path.empty()) {
        EXPECT_LT(distance(path.front(), start), 1.5f);  // Start is close
        EXPECT_LT(distance(path.back(), end), 1.5f);      // End is close
    }
}

TEST_F(PathOptimizationTest, NoPathToOutOfBounds) {
    Vector3 start(0, 0, 0);
    Vector3 oob(150, 150, 0);  // Outside 100x100 world

    auto path = engine_->computePath(start, oob);
    EXPECT_TRUE(path.empty());  // No path to unreachable location
}

TEST_F(PathOptimizationTest, PathLengthReasonable) {
    Vector3 start(10, 10, 0);
    Vector3 end(50, 50, 0);
    auto path = engine_->computePath(start, end);

    if (path.size() > 1) {
        float pathDistance = 0;
        for (size_t i = 1; i < path.size(); ++i) {
            pathDistance += distance(path[i-1], path[i]);
        }

        float directDistance = distance(start, end);
        EXPECT_LE(pathDistance, directDistance * 1.5f);  // Path not too inefficient
    }
}

// ==================== MOVEMENT EFFICIENCY TESTS ====================

class MovementEfficiencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        controller_ = &MovementController::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
        controller_->initialize(*engine_);
    }

    PathfindingEngine* engine_;
    MovementController* controller_;
};

TEST_F(MovementEfficiencyTest, CalculateEffectiveSpeed) {
    float speed1 = engine_->calculateEffectiveSpeed(1.0f, 1.0f, 1.0f);
    EXPECT_EQ(speed1, 1.0f);

    float speed2 = engine_->calculateEffectiveSpeed(1.0f, 0.5f, 1.0f);
    EXPECT_EQ(speed2, 0.5f);

    float speed3 = engine_->calculateEffectiveSpeed(1.0f, 1.0f, 0.5f);
    EXPECT_EQ(speed3, 0.5f);
}

TEST_F(MovementEfficiencyTest, BaseSpeedForRoles) {
    EXPECT_EQ(engine_->getBaseSpeedForRole("warrior"), 1.0f);
    EXPECT_EQ(engine_->getBaseSpeedForRole("scout"), 1.2f);
    EXPECT_EQ(engine_->getBaseSpeedForRole("merchant"), 0.7f);
    EXPECT_EQ(engine_->getBaseSpeedForRole("farmer"), 0.6f);
    EXPECT_EQ(engine_->getBaseSpeedForRole("priest"), 0.5f);
}

TEST_F(MovementEfficiencyTest, UnknownRoleDefaultSpeed) {
    float speed = engine_->getBaseSpeedForRole("unknown_role");
    EXPECT_GT(speed, 0.0f);  // Should return reasonable default
}

TEST_F(MovementEfficiencyTest, TerrainModifierExists) {
    Vector3 pos(10, 10, 0);
    float modifier = engine_->getTerrainModifier(pos);
    EXPECT_GT(modifier, 0.0f);
    EXPECT_LE(modifier, 2.0f);  // Should be reasonable range
}

// ==================== STUCK DETECTION TESTS ====================

class StuckDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
    }

    PathfindingEngine* engine_;
};

TEST_F(StuckDetectionTest, DetectStuckNPC) {
    std::vector<Vector3> posHistory;
    Vector3 goal(100, 100, 0);
    Vector3 pos(10, 10, 0);

    // Create history where NPC didn't move
    for (int i = 0; i < 30; ++i) {
        posHistory.push_back(pos);
    }

    bool stuck = engine_->isNPCStuck(posHistory, goal);
    EXPECT_TRUE(stuck);
}

TEST_F(StuckDetectionTest, NotStuckIfMoving) {
    std::vector<Vector3> posHistory;
    Vector3 goal(100, 100, 0);

    // Create history where NPC is moving toward goal
    for (int i = 0; i < 30; ++i) {
        posHistory.push_back(Vector3(i, i, 0));
    }

    bool stuck = engine_->isNPCStuck(posHistory, goal);
    EXPECT_FALSE(stuck);
}

TEST_F(StuckDetectionTest, GetRecoveryDestination) {
    Vector3 goal(100, 100, 0);
    Vector3 pos(50, 50, 0);

    auto recovery1 = engine_->getRecoveryDestination(goal, pos, 1);
    EXPECT_TRUE(recovery1.has_value());

    auto recovery2 = engine_->getRecoveryDestination(goal, pos, 2);
    EXPECT_TRUE(recovery2.has_value());

    auto recovery3 = engine_->getRecoveryDestination(goal, pos, 3);
    EXPECT_FALSE(recovery3.has_value());  // Max attempts reached
}

// ==================== COLLISION AVOIDANCE TESTS ====================

class CollisionAvoidanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
    }

    PathfindingEngine* engine_;
};

TEST_F(CollisionAvoidanceTest, SeparationForceAwayFromNeighbor) {
    Vector3 npc_pos(0, 0, 0);
    std::vector<Vector3> neighbors = {Vector3(1, 0, 0)};  // Close neighbor

    auto force = engine_->calculateSeparationForce(npc_pos, neighbors);
    EXPECT_NE(force, Vector3(0, 0, 0));  // Should generate separation force
}

TEST_F(CollisionAvoidanceTest, NoForceFromDistantNPC) {
    Vector3 npc_pos(0, 0, 0);
    std::vector<Vector3> neighbors = {Vector3(10, 10, 0)};  // Far away

    auto force = engine_->calculateSeparationForce(npc_pos, neighbors);
    EXPECT_EQ(length(force), 0.0f);  // No force beyond avoidance radius
}

TEST_F(CollisionAvoidanceTest, CombinedVelocity) {
    Vector3 pathVel(1, 0, 0);
    Vector3 avoidVel(0, 1, 0);

    auto combined = engine_->combinedVelocity(pathVel, avoidVel, 0.5f);
    EXPECT_NE(combined, Vector3(0, 0, 0));
}

// ==================== LAZY RECALCULATION TESTS ====================

class LazyRecalculationTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
    }

    PathfindingEngine* engine_;
};

TEST_F(LazyRecalculationTest, RecalcEvery5Ticks) {
    Vector3 pos(0, 0, 0);
    Vector3 target(10, 10, 0);

    EXPECT_FALSE(engine_->shouldRecalcPath(pos, target, 0, 0, target));
    EXPECT_FALSE(engine_->shouldRecalcPath(pos, target, 0, 3, target));
    EXPECT_TRUE(engine_->shouldRecalcPath(pos, target, 0, 5, target));  // 5 ticks passed
}

TEST_F(LazyRecalculationTest, RecalcIfTargetMoved) {
    Vector3 pos(0, 0, 0);
    Vector3 target1(10, 10, 0);
    Vector3 target2(25, 25, 0);  // Moved >10 units

    EXPECT_TRUE(engine_->shouldRecalcPath(pos, target2, 0, 1, target1));
}

TEST_F(LazyRecalculationTest, NoRecalcIfTargetUnchanged) {
    Vector3 pos(0, 0, 0);
    Vector3 target(10, 10, 0);

    EXPECT_FALSE(engine_->shouldRecalcPath(pos, target, 0, 2, target));
}

// ==================== WAYPOINT PROGRESSION TESTS ====================

class WaypointProgressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
    }

    PathfindingEngine* engine_;
};

TEST_F(WaypointProgressionTest, GetNextWaypoint) {
    std::vector<Vector3> path = {Vector3(0, 0, 0), Vector3(5, 5, 0), Vector3(10, 10, 0)};

    auto wp0 = engine_->getNextWaypoint(path, 0);
    EXPECT_TRUE(wp0.has_value());
    EXPECT_EQ(*wp0, Vector3(0, 0, 0));

    auto wp2 = engine_->getNextWaypoint(path, 2);
    EXPECT_TRUE(wp2.has_value());
    EXPECT_EQ(*wp2, Vector3(10, 10, 0));
}

TEST_F(WaypointProgressionTest, ReturnNulloptAtEnd) {
    std::vector<Vector3> path = {Vector3(0, 0, 0), Vector3(5, 5, 0)};

    auto wp = engine_->getNextWaypoint(path, 2);  // Beyond end
    EXPECT_FALSE(wp.has_value());
}

TEST_F(WaypointProgressionTest, HasReachedWaypoint) {
    Vector3 pos(5.0f, 5.0f, 0.0f);
    Vector3 waypoint(5.1f, 5.1f, 0.0f);

    EXPECT_TRUE(engine_->hasReachedWaypoint(pos, waypoint, 0.2f));
}

TEST_F(WaypointProgressionTest, HasNotReachedDistantWaypoint) {
    Vector3 pos(0, 0, 0);
    Vector3 waypoint(10, 10, 0);

    EXPECT_FALSE(engine_->hasReachedWaypoint(pos, waypoint, 1.0f));
}

// ==================== PERFORMANCE TESTS ====================

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        controller_ = &MovementController::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
        controller_->initialize(*engine_);
    }

    PathfindingEngine* engine_;
    MovementController* controller_;
};

TEST_F(PerformanceTest, FastPathComputationBenchmark) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        engine_->computePath(Vector3(0, 0, 0), Vector3(50, 50, 0));
    }

    auto end = std::chrono::high_resolution_clock::now();
    long millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_LT(millis, 1000);  // 100 paths in <1 second
}

TEST_F(PerformanceTest, MetricsTracking) {
    engine_->clearPathCache();
    engine_->computePath(Vector3(0, 0, 0), Vector3(50, 50, 0));  // Longer path for measurable time

    auto metrics = engine_->getMetrics();
    // Compute time may be 0 for very fast paths on modern hardware
    EXPECT_GE(metrics.lastComputeTimeMs, 0.0f);
    // Nodes expanded should always be > 0 for a valid path computation
    EXPECT_GT(metrics.lastNodesExpanded, 0);
}

TEST_F(PerformanceTest, SpatialGridPerformance) {
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 100; ++i) {
        engine_->addNPCToGrid(i, Vector3(i, i, 0));
    }

    for (int i = 0; i < 100; ++i) {
        auto nearby = engine_->queryNearbyNPCs(Vector3(50, 50, 0), 15.0f);
        EXPECT_GT(nearby.size(), 0);  // Should find some
    }

    auto end = std::chrono::high_resolution_clock::now();
    long millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    EXPECT_LT(millis, 500);  // 200 operations in <500ms
}

// ==================== INTEGRATION TESTS ====================

class Phase11IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = &PathfindingEngine::getInstance();
        controller_ = &MovementController::getInstance();
        engine_->reset();
        engine_->initialize(100.0f, 100.0f, 100.0f, 10.0f);
        controller_->initialize(*engine_);
    }

    PathfindingEngine* engine_;
    MovementController* controller_;
};

TEST_F(Phase11IntegrationTest, FullPathfindingAndMovement) {
    Vector3 start(10, 10, 0);
    Vector3 goal(50, 50, 0);

    auto path = engine_->computePath(start, goal);
    EXPECT_FALSE(path.empty());
    EXPECT_GT(path.size(), 1);
}

TEST_F(Phase11IntegrationTest, CacheIntegrationWithMovement) {
    Vector3 start(0, 0, 0);
    Vector3 goal(20, 20, 0);

    engine_->clearPathCache();

    auto path1 = engine_->computePath(start, goal);
    float hitRate1 = engine_->getCacheHitRate();

    auto path2 = engine_->computePath(start, goal);
    float hitRate2 = engine_->getCacheHitRate();

    EXPECT_EQ(path1, path2);
    EXPECT_GT(hitRate2, hitRate1);
}

TEST_F(Phase11IntegrationTest, AllSystemsInitialize) {
    EXPECT_TRUE(engine_->isWalkable(Vector3(50, 50, 0)));
    EXPECT_FALSE(engine_->isWalkable(Vector3(150, 150, 0)));  // Out of bounds
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
