#ifndef VECTOR3_H
#define VECTOR3_H

#include <cmath>
#include <ostream>

namespace TLS {

// 3D Vector for positions, velocities, and directional calculations
struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // Constructors
    Vector3() = default;
    Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    // Vector arithmetic
    Vector3 operator+(const Vector3& v) const {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }

    Vector3 operator-(const Vector3& v) const {
        return Vector3(x - v.x, y - v.y, z - v.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator/(float scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    Vector3& operator+=(const Vector3& v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    Vector3& operator-=(const Vector3& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }

    Vector3& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    // Comparison
    bool operator==(const Vector3& v) const {
        const float epsilon = 1e-6f;
        return std::fabs(x - v.x) < epsilon &&
               std::fabs(y - v.y) < epsilon &&
               std::fabs(z - v.z) < epsilon;
    }

    bool operator!=(const Vector3& v) const {
        return !(*this == v);
    }

    bool operator<(const Vector3& v) const {
        if (std::fabs(x - v.x) > 1e-6f) return x < v.x;
        if (std::fabs(y - v.y) > 1e-6f) return y < v.y;
        return z < v.z;
    }

    // Vector operations
    float dot(const Vector3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    Vector3 cross(const Vector3& v) const {
        return Vector3(y * v.z - z * v.y,
                       z * v.x - x * v.z,
                       x * v.y - y * v.x);
    }

    float magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    float magnitudeSquared() const {
        return x * x + y * y + z * z;
    }

    Vector3 normalized() const {
        float mag = magnitude();
        if (mag < 1e-6f) return Vector3(0, 0, 0);
        return *this / mag;
    }

    float distance(const Vector3& v) const {
        return (*this - v).magnitude();
    }

    float distanceSquared(const Vector3& v) const {
        return (*this - v).magnitudeSquared();
    }

    // Manhattan distance (for heuristics)
    float manhattanDistance(const Vector3& v) const {
        return std::fabs(x - v.x) + std::fabs(y - v.y) + std::fabs(z - v.z);
    }

    // Direction to another point
    Vector3 directionTo(const Vector3& other) const {
        Vector3 dir = other - *this;
        float mag = dir.magnitude();
        if (mag < 1e-6f) return Vector3(0, 0, 0);
        return dir / mag;
    }

    // Check if within range (uses squared distance for efficiency)
    bool isWithinRange(const Vector3& other, float range) const {
        return distanceSquared(other) <= range * range;
    }

    // Clamp vector to bounds
    Vector3 clamp(const Vector3& min, const Vector3& max) const {
        return Vector3(
            std::max(min.x, std::min(x, max.x)),
            std::max(min.y, std::min(y, max.y)),
            std::max(min.z, std::min(z, max.z))
        );
    }

    // Output for debugging
    friend std::ostream& operator<<(std::ostream& os, const Vector3& v) {
        os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return os;
    }
};

// Free function utilities
inline float distance(const Vector3& a, const Vector3& b) {
    return a.distance(b);
}

inline float manhattanDistance(const Vector3& a, const Vector3& b) {
    return a.manhattanDistance(b);
}

inline Vector3 normalize(const Vector3& v) {
    return v.normalized();
}

inline float dot(const Vector3& a, const Vector3& b) {
    return a.dot(b);
}

inline Vector3 cross(const Vector3& a, const Vector3& b) {
    return a.cross(b);
}

inline Vector3 lerp(const Vector3& a, const Vector3& b, float t) {
    return a + (b - a) * t;
}

inline float length(const Vector3& v) {
    return v.magnitude();
}

}  // namespace TLS

#endif  // VECTOR3_H
