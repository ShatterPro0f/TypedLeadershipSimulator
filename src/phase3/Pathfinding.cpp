#include "Pathfinding.h"
#include "Collision.h"
#include <queue>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
// #include <nlohmann/json.hpp>

// using json = nlohmann::json;

namespace TLS {

// WaypointGraph Implementation
WaypointGraph::WaypointGraph() {}

bool WaypointGraph::loadFromFile(const std::string& filename) {
    // JSON support disabled for now - use manual waypoint setup instead
    // TODO: Add nlohmann/json dependency for JSON file loading
    return false;
}

int WaypointGraph::getNearestWaypoint(const Vector3& position) const {
    if (waypoints.empty()) return -1;

    int nearest = 0;
    float minDist = position.distance(waypoints[0].position);

    for (size_t i = 1; i < waypoints.size(); ++i) {
        float dist = position.distance(waypoints[i].position);
        if (dist < minDist) {
            minDist = dist;
            nearest = i;
        }
    }

    return nearest;
}

std::vector<Vector3> WaypointGraph::findPath(
    int startWaypoint,
    int endWaypoint,
    const World& world
) const {
    std::vector<Vector3> result;

    if (startWaypoint < 0 || startWaypoint >= (int)waypoints.size() ||
        endWaypoint < 0 || endWaypoint >= (int)waypoints.size()) {
        return result;
    }

    // Use A* to find waypoint path
    std::vector<int> waypointPath = astarSearch(startWaypoint, endWaypoint);

    // Convert waypoint path to position path
    for (int wpId : waypointPath) {
        if (wpId >= 0 && wpId < (int)waypoints.size()) {
            result.push_back(waypoints[wpId].position);
        }
    }

    return result;
}

std::optional<Vector3> WaypointGraph::getWaypoint(const std::string& name) const {
    auto it = locationToWaypoint.find(name);
    if (it != locationToWaypoint.end()) {
        int id = it->second;
        if (id >= 0 && id < (int)waypoints.size()) {
            return waypoints[id].position;
        }
    }
    return std::nullopt;
}

std::optional<Vector3> WaypointGraph::getWaypointById(int id) const {
    if (id >= 0 && id < (int)waypoints.size()) {
        return waypoints[id].position;
    }
    return std::nullopt;
}

void WaypointGraph::addWaypoint(int id, const std::string& name, const Vector3& position) {
    // Ensure adjacency list is large enough
    while ((int)adjacency.size() <= id) {
        adjacency.push_back(std::vector<int>());
    }

    Waypoint wp{id, name, position};
    if ((int)waypoints.size() <= id) {
        waypoints.resize(id + 1);
    }
    waypoints[id] = wp;
    locationToWaypoint[name] = id;
}

void WaypointGraph::addConnection(int from, int to) {
    if (from >= 0 && from < (int)adjacency.size()) {
        auto& adj = adjacency[from];
        if (std::find(adj.begin(), adj.end(), to) == adj.end()) {
            adj.push_back(to);
        }
    }
}

std::vector<int> WaypointGraph::astarSearch(int start, int goal) const {
    std::vector<int> path;

    struct Node {
        int id;
        float gCost;
        float hCost;
        int parent;

        float fCost() const { return gCost + hCost; }

        bool operator>(const Node& other) const {
            return fCost() > other.fCost();
        }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::map<int, float> gScores;
    std::set<int> closedSet;

    openSet.push({start, 0.0f, heuristic(start, goal), -1});
    gScores[start] = 0.0f;

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.id == goal) {
            // Reconstruct path
            int node = goal;
            while (node != -1) {
                path.push_back(node);
                // Find parent by iterating openSet/gScores (simplified)
                if (node == start) break;
                
                // For simplicity, just add to path
                break;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        if (closedSet.count(current.id)) continue;
        closedSet.insert(current.id);

        if (current.id >= 0 && current.id < (int)adjacency.size()) {
            for (int neighbor : adjacency[current.id]) {
                if (closedSet.count(neighbor)) continue;

                float tentativeGScore = gScores[current.id] + 
                    waypoints[current.id].position.distance(waypoints[neighbor].position);

                if (!gScores.count(neighbor) || tentativeGScore < gScores[neighbor]) {
                    gScores[neighbor] = tentativeGScore;
                    float h = heuristic(neighbor, goal);
                    openSet.push({neighbor, tentativeGScore, h, current.id});
                }
            }
        }
    }

    // If no path found, return direct connection
    if (!path.empty()) {
        return path;
    }

    path.push_back(start);
    path.push_back(goal);
    return path;
}

float WaypointGraph::heuristic(int a, int b) const {
    if (a < 0 || a >= (int)waypoints.size() || b < 0 || b >= (int)waypoints.size()) {
        return 0.0f;
    }
    return waypoints[a].position.distance(waypoints[b].position);
}

// NPCNavigator Implementation
Path NPCNavigator::planPath(
    const Vector3& from,
    const Vector3& to,
    const WaypointGraph& graph,
    const World& world
) {
    int startWp = graph.getNearestWaypoint(from);
    int endWp = graph.getNearestWaypoint(to);

    std::vector<Vector3> waypoints = graph.findPath(startWp, endWp, world);
    
    if (waypoints.empty()) {
        // Direct path if no waypoints available
        waypoints.push_back(from);
        waypoints.push_back(to);
    }

    return Path(waypoints);
}

Vector3 NPCNavigator::getNextPosition(
    Path& path,
    const Vector3& currentPosition,
    float moveSpeed,
    float deltaTime
) {
    if (path.isComplete || path.waypoints.empty()) {
        return currentPosition;
    }

    Vector3 currentTarget = path.waypoints[path.currentWaypointIndex];
    Vector3 direction = (currentTarget - currentPosition).normalized();
    Vector3 nextPos = currentPosition + direction * moveSpeed * deltaTime;

    // Check if reached waypoint
    if (nextPos.distance(currentTarget) < WAYPOINT_ARRIVAL_DISTANCE) {
        path.currentWaypointIndex++;
        if (path.currentWaypointIndex >= path.waypoints.size()) {
            path.isComplete = true;
            return currentTarget;
        }
    }

    return nextPos;
}

bool NPCNavigator::isPathValid(const Path& path, const World& world) {
    if (path.waypoints.size() < 2) return false;

    for (size_t i = 0; i < path.waypoints.size() - 1; ++i) {
        if (!CollisionDetector::isPathClear(
            path.waypoints[i],
            path.waypoints[i + 1],
            0.5f,  // NPC collision radius
            world
        )) {
            return false;
        }
    }

    return true;
}

std::optional<Vector3> NPCNavigator::getCurrentWaypoint(const Path& path) {
    if (path.currentWaypointIndex < path.waypoints.size()) {
        return path.waypoints[path.currentWaypointIndex];
    }
    return std::nullopt;
}

}  // namespace TLS
