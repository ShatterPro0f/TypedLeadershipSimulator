#include "Player.h"
#include "Collision.h"
#include "MovementConfig.h"
#include <cmath>
#include <cstdint>

namespace TLS {

Player::Player()
    : name("Player"), position(0, 0, 0), velocity(0, 0, 0),
      moveSpeed(MovementConfig::PLAYER_BASE_SPEED),
      sprintSpeed(MovementConfig::PLAYER_SPRINT_SPEED),
      forward(0, 1, 0), up(0, 0, 1), right(1, 0, 0),
      verticalAngle(0), horizontalAngle(0), fov(90.0f),
      collisionRadius(MovementConfig::PLAYER_COLLISION_RADIUS),
      eyeHeight(MovementConfig::PLAYER_EYE_HEIGHT) {}

Player::Player(const std::string& name_, const Vector3& startPosition)
    : name(name_), position(startPosition), velocity(0, 0, 0),
      moveSpeed(MovementConfig::PLAYER_BASE_SPEED),
      sprintSpeed(MovementConfig::PLAYER_SPRINT_SPEED),
      forward(0, 1, 0), up(0, 0, 1), right(1, 0, 0),
      verticalAngle(0), horizontalAngle(0), fov(90.0f),
      collisionRadius(MovementConfig::PLAYER_COLLISION_RADIUS),
      eyeHeight(MovementConfig::PLAYER_EYE_HEIGHT) {
    updateOrientation();
}

void Player::moveForward(float amount) {
    velocity += forward * amount * moveSpeed;
}

void Player::moveRight(float amount) {
    velocity += right * amount * moveSpeed;
}

void Player::moveUp(float amount) {
    velocity += up * amount * moveSpeed;
}

void Player::rotateHorizontal(float deltaAngle) {
    horizontalAngle += deltaAngle;
    updateOrientation();
}

void Player::rotateVertical(float deltaAngle) {
    verticalAngle += deltaAngle;
    // Clamp vertical rotation to prevent gimbal lock
    constexpr float MAX_PITCH = 1.57079632679f;  // 90 degrees
    if (verticalAngle > MAX_PITCH) verticalAngle = MAX_PITCH;
    if (verticalAngle < -MAX_PITCH) verticalAngle = -MAX_PITCH;
    updateOrientation();
}

void Player::updateOrientation() {
    // Calculate forward vector from angles
    // horizontalAngle = yaw (rotation around Z axis)
    // verticalAngle = pitch (rotation around X axis)
    float cosH = std::cos(horizontalAngle);
    float sinH = std::sin(horizontalAngle);
    float cosV = std::cos(verticalAngle);
    float sinV = std::sin(verticalAngle);

    // Forward direction considering both angles
    forward = Vector3(
        sinH * cosV,
        cosH * cosV,
        sinV
    ).normalized();

    // Recalculate right vector (perpendicular to forward in XY plane)
    right = Vector3(
        cosH,
        -sinH,
        0
    ).normalized();

    // Recalculate up vector (perpendicular to forward and right)
    up = right.cross(forward).normalized();
}

void Player::update(float deltaTime, const World& world) {
    if (deltaTime <= 0) return;

    // Apply velocity
    Vector3 newPosition = position + velocity * deltaTime;

    // Collision detection and response
    if (CollisionDetector::isPathClear(position, newPosition, collisionRadius, world)) {
        position = newPosition;
    } else {
        // Path blocked, try sliding along obstacles (simplified)
        // For now, just stop movement
        velocity = Vector3(0, 0, 0);
    }

    // Gravity (simple downward acceleration if not on ground)
    constexpr float GRAVITY = -9.8f;
    if (position.z > 0) {  // Rough ground check
        velocity.z += GRAVITY * deltaTime;
    } else if (position.z <= 0) {
        position.z = 0;
        velocity.z = 0;  // Stop at ground level
    }

    // Friction/velocity decay
    velocity *= 0.95f;
}

Vector3 Player::getEyePosition() const {
    return position + up * eyeHeight;
}

bool Player::canReach(const Vector3& target, float reachDistance) const {
    return getDistanceTo(target) <= reachDistance;
}

float Player::getDistanceTo(const Vector3& target) const {
    return position.distance(target);
}

bool Player::toBinary(std::ofstream& file) const {
    // Write name
    uint32_t nameLen = static_cast<uint32_t>(name.length());
    if (!file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen))) return false;
    if (!file.write(name.c_str(), nameLen)) return false;

    // Write position, velocity
    if (!file.write(reinterpret_cast<const char*>(&position.x), sizeof(position.x))) return false;
    if (!file.write(reinterpret_cast<const char*>(&position.y), sizeof(position.y))) return false;
    if (!file.write(reinterpret_cast<const char*>(&position.z), sizeof(position.z))) return false;
    if (!file.write(reinterpret_cast<const char*>(&velocity.x), sizeof(velocity.x))) return false;
    if (!file.write(reinterpret_cast<const char*>(&velocity.y), sizeof(velocity.y))) return false;
    if (!file.write(reinterpret_cast<const char*>(&velocity.z), sizeof(velocity.z))) return false;

    // Write angles
    if (!file.write(reinterpret_cast<const char*>(&verticalAngle), sizeof(verticalAngle))) return false;
    if (!file.write(reinterpret_cast<const char*>(&horizontalAngle), sizeof(horizontalAngle))) return false;

    return true;
}

Player Player::fromBinary(std::ifstream& file) {
    // Read name
    uint32_t nameLen = 0;
    file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    std::string name(nameLen, '\0');
    file.read(&name[0], nameLen);

    Player player(name, Vector3(0, 0, 0));

    // Read position, velocity
    file.read(reinterpret_cast<char*>(&player.position.x), sizeof(player.position.x));
    file.read(reinterpret_cast<char*>(&player.position.y), sizeof(player.position.y));
    file.read(reinterpret_cast<char*>(&player.position.z), sizeof(player.position.z));
    file.read(reinterpret_cast<char*>(&player.velocity.x), sizeof(player.velocity.x));
    file.read(reinterpret_cast<char*>(&player.velocity.y), sizeof(player.velocity.y));
    file.read(reinterpret_cast<char*>(&player.velocity.z), sizeof(player.velocity.z));

    // Read angles
    file.read(reinterpret_cast<char*>(&player.verticalAngle), sizeof(player.verticalAngle));
    file.read(reinterpret_cast<char*>(&player.horizontalAngle), sizeof(player.horizontalAngle));

    player.updateOrientation();
    return player;
}

}  // namespace TLS
