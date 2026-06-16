#include "EditorCamera.h"

EditorCamera::EditorCamera(Vector3 position, float aspect)
    : Position(position), Yaw(-90.0f), Pitch(-30.0f), AspectRatio(aspect), Fov(60.0f), FarPlane(16000.0f) {
    WorldUp = Vector3(0.0f, 1.0f, 0.0f);
    UpdateCameraVectors();
}

void EditorCamera::UpdateCameraVectors() {
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

Matrix4 EditorCamera::GetViewMatrix() const {
    return MatrixLookAtLH(Position, Position + Front, WorldUp);
}

Matrix4 EditorCamera::GetProjectionMatrix() const {
    float fovRad = Fov * (3.14159265f / 180.0f);
    return MatrixPerspectiveFovLH(fovRad, AspectRatio, 1.0f, FarPlane);
}

void EditorCamera::SetLoadRadius(int radius) {
    static const float farPlanes[] = {4000.0f, 12000.0f, 20000.0f, 26000.0f, 32000.0f};
    int idx = std::clamp(radius, 0, 4);
    FarPlane = farPlanes[idx];
}

void EditorCamera::ProcessKeyboard(int direction, float deltaTime, bool speedUp) {
    float velocity = 250.0f * deltaTime;
    if (speedUp) velocity *= 4.0f;
    if (direction == 1) Position += Front * velocity;
    if (direction == 2) Position -= Front * velocity;
    if (direction == 3) Position += Right * velocity;
    if (direction == 4) Position -= Right * velocity;
    if (direction == 5) Position += WorldUp * velocity;
    if (direction == 6) Position -= WorldUp * velocity;
}

void EditorCamera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= 0.2f;
    yoffset *= 0.2f;
    Yaw -= xoffset;
    Pitch -= yoffset;
    Pitch = std::clamp(Pitch, -89.0f, 89.0f);
    UpdateCameraVectors();
}

void EditorCameraFrustum::Plane::Normalize() {
    float len = sqrtf(a * a + b * b + c * c);
    if (len > 0.0f) { a /= len; b /= len; c /= len; d /= len; }
}

EditorCameraFrustum EditorCameraFrustum::Extract(const Matrix4& m) {
    EditorCameraFrustum f;
    f.planes[0] = {m.m[0][3] + m.m[0][0], m.m[1][3] + m.m[1][0], m.m[2][3] + m.m[2][0], m.m[3][3] + m.m[3][0]};
    f.planes[1] = {m.m[0][3] - m.m[0][0], m.m[1][3] - m.m[1][0], m.m[2][3] - m.m[2][0], m.m[3][3] - m.m[3][0]};
    f.planes[2] = {m.m[0][3] + m.m[0][1], m.m[1][3] + m.m[1][1], m.m[2][3] + m.m[2][1], m.m[3][3] + m.m[3][1]};
    f.planes[3] = {m.m[0][3] - m.m[0][1], m.m[1][3] - m.m[1][1], m.m[2][3] - m.m[2][1], m.m[3][3] - m.m[3][1]};
    f.planes[4] = {m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2]};
    f.planes[5] = {m.m[0][3] - m.m[0][2], m.m[1][3] - m.m[1][2], m.m[2][3] - m.m[2][2], m.m[3][3] - m.m[3][2]};
    for (int i = 0; i < 6; ++i) f.planes[i].Normalize();
    return f;
}

bool EditorCameraFrustum::IsBoxVisible(const Vector3& min, const Vector3& max) const {
    for (int i = 0; i < 6; ++i) {
        float px = (planes[i].a > 0.0f) ? max.x : min.x;
        float py = (planes[i].b > 0.0f) ? max.y : min.y;
        float pz = (planes[i].c > 0.0f) ? max.z : min.z;
        if (planes[i].a * px + planes[i].b * py + planes[i].c * pz + planes[i].d < 0.0f) return false;
    }
    return true;
}
