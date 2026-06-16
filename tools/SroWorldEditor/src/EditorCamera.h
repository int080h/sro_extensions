#pragma once
#include "core/MathTypes.h"
#include <d3d9.h>
#include <algorithm>

class EditorCamera {
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
    float FarPlane;

    EditorCamera(Vector3 position = Vector3(960.0f, 150.0f, 960.0f), float aspect = 1.77f);

    void UpdateCameraVectors();
    Matrix4 GetViewMatrix() const;
    Matrix4 GetProjectionMatrix() const;
    void SetLoadRadius(int radius);
    void ProcessKeyboard(int direction, float deltaTime, bool speedUp = false);
    void ProcessMouseMovement(float xoffset, float yoffset);
};

struct EditorCameraFrustum {
    struct Plane { float a, b, c, d; void Normalize(); };
    Plane planes[6];
    static EditorCameraFrustum Extract(const Matrix4& m);
    bool IsBoxVisible(const Vector3& min, const Vector3& max) const;
};
