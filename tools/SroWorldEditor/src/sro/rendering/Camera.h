#pragma once
#include "Core/Math.h"
#include <d3d9.h>
#include <algorithm>

// ============================================================================
// Camera — FPS-style camera with view/projection matrix generation
// ============================================================================

class Camera {
public:
    Vector3 Position;
    Vector3 Front;
    Vector3 Up;
    Vector3 Right;
    Vector3 WorldUp;

    float Yaw;
    float Pitch;
    float AspectRatio;
    float Fov;
    float FarPlane; // Dynamic far plane based on load radius

    Camera(Vector3 position = Vector3(960.0f, 150.0f, 960.0f), float aspect = 1.77f) 
        : Position(position), Yaw(-90.0f), Pitch(-30.0f), AspectRatio(aspect), Fov(60.0f), FarPlane(16000.0f) {
        WorldUp = Vector3(0.0f, 1.0f, 0.0f);
        updateCameraVectors();
    }

    void updateCameraVectors() {
        float yawRad = Yaw * (3.14159265f / 180.0f);
        float pitchRad = Pitch * (3.14159265f / 180.0f);

        Vector3 front;
        front.x = cosf(pitchRad) * cosf(yawRad);
        front.y = sinf(pitchRad);
        front.z = cosf(pitchRad) * sinf(yawRad);
        Front = Vec3Normalize(front);

        Right = Vec3Normalize(Vec3Cross(Front, WorldUp));
        Up = Vec3Normalize(Vec3Cross(Right, Front));
    }

    Matrix4 GetViewMatrix() {
        Vector3 target = Position + Front;
        return MatrixLookAtLH(Position, target, Up);
    }

    Matrix4 GetProjectionMatrix() {
        float fovRad = Fov * (3.14159265f / 180.0f);
        return MatrixPerspectiveFovLH(fovRad, AspectRatio, 1.0f, FarPlane);
    }

    // Update far plane based on load radius for optimal depth buffer precision
    void SetLoadRadius(int radius) {
        static const float farPlanes[] = { 4000.0f, 12000.0f, 20000.0f, 26000.0f, 32000.0f };
        int idx = std::clamp(radius, 0, 4);
        FarPlane = farPlanes[idx];
    }

    void ProcessKeyboard(int direction, float deltaTime, bool speedUp = false) {
        float velocity = 250.0f * deltaTime;
        if (speedUp)
            velocity *= 4.0f;

        if (direction == 1) // W (Forward)
            Position += Front * velocity;
        if (direction == 2) // S (Backward)
            Position -= Front * velocity;
        if (direction == 3) // A (Left)
            Position += Right * velocity;
        if (direction == 4) // D (Right)
            Position -= Right * velocity;
        if (direction == 5) // E (Up)
            Position += WorldUp * velocity;
        if (direction == 6) // Q (Down)
            Position -= WorldUp * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        xoffset *= 0.2f;
        yoffset *= 0.2f;

        Yaw -= xoffset;
        Pitch -= yoffset;

        Pitch = std::clamp(Pitch, -89.0f, 89.0f);

        updateCameraVectors();
    }
};

// ============================================================================
// Camera Frustum — for visibility culling
// ============================================================================

struct FrustumPlane {
    float a, b, c, d;
    void Normalize() {
        float len = sqrtf(a * a + b * b + c * c);
        if (len > 0.0f) {
            a /= len;
            b /= len;
            c /= len;
            d /= len;
        }
    }
};

struct CameraFrustum {
    FrustumPlane planes[6];
    
    static CameraFrustum Extract(const Matrix4& m) {
        CameraFrustum f;
        // Left Plane
        f.planes[0].a = m.m[0][3] + m.m[0][0];
        f.planes[0].b = m.m[1][3] + m.m[1][0];
        f.planes[0].c = m.m[2][3] + m.m[2][0];
        f.planes[0].d = m.m[3][3] + m.m[3][0];
        // Right Plane
        f.planes[1].a = m.m[0][3] - m.m[0][0];
        f.planes[1].b = m.m[1][3] - m.m[1][0];
        f.planes[1].c = m.m[2][3] - m.m[2][0];
        f.planes[1].d = m.m[3][3] - m.m[3][0];
        // Bottom Plane
        f.planes[2].a = m.m[0][3] + m.m[0][1];
        f.planes[2].b = m.m[1][3] + m.m[1][1];
        f.planes[2].c = m.m[2][3] + m.m[2][1];
        f.planes[2].d = m.m[3][3] + m.m[3][1];
        // Top Plane
        f.planes[3].a = m.m[0][3] - m.m[0][1];
        f.planes[3].b = m.m[1][3] - m.m[1][1];
        f.planes[3].c = m.m[2][3] - m.m[2][1];
        f.planes[3].d = m.m[3][3] - m.m[3][1];
        // Near Plane
        f.planes[4].a = m.m[0][2];
        f.planes[4].b = m.m[1][2];
        f.planes[4].c = m.m[2][2];
        f.planes[4].d = m.m[3][2];
        // Far Plane
        f.planes[5].a = m.m[0][3] - m.m[0][2];
        f.planes[5].b = m.m[1][3] - m.m[1][2];
        f.planes[5].c = m.m[2][3] - m.m[2][2];
        f.planes[5].d = m.m[3][3] - m.m[3][2];

        for (int i = 0; i < 6; ++i) f.planes[i].Normalize();
        return f;
    }

    bool IsBoxVisible(const Vector3& min, const Vector3& max) const {
        for (int i = 0; i < 6; ++i) {
            float px = (planes[i].a > 0.0f) ? max.x : min.x;
            float py = (planes[i].b > 0.0f) ? max.y : min.y;
            float pz = (planes[i].c > 0.0f) ? max.z : min.z;

            float dot = planes[i].a * px + planes[i].b * py + planes[i].c * pz + planes[i].d;
            if (dot < 0.0f) return false;
        }
        return true;
    }
};
