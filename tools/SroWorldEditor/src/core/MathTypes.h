#pragma once
#include <cmath>
#include <cstring>

// ============================================================================
// Core math types for the SroMapViewer
// ============================================================================

struct Vector2 {
    float x, y;
    Vector2() : x(0.0f), y(0.0f) {}
    Vector2(float x, float y) : x(x), y(y) {}
};

inline Vector2 operator+(const Vector2& a, const Vector2& b) { return Vector2(a.x + b.x, a.y + b.y); }
inline Vector2 operator-(const Vector2& a, const Vector2& b) { return Vector2(a.x - b.x, a.y - b.y); }
inline Vector2 operator*(const Vector2& a, float f) { return Vector2(a.x * f, a.y * f); }
inline Vector2& operator+=(Vector2& a, const Vector2& b) { a.x += b.x; a.y += b.y; return a; }
inline Vector2& operator-=(Vector2& a, const Vector2& b) { a.x -= b.x; a.y -= b.y; return a; }

struct Vector3 {
    float x, y, z;
    Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vector4 {
    float x, y, z, w;
    Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
};

inline Vector3 operator+(const Vector3& a, const Vector3& b) { return Vector3(a.x + b.x, a.y + b.y, a.z + b.z); }
inline Vector3 operator-(const Vector3& a, const Vector3& b) { return Vector3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline Vector3 operator*(const Vector3& a, float f) { return Vector3(a.x * f, a.y * f, a.z * f); }
inline Vector3& operator+=(Vector3& a, const Vector3& b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
inline Vector3& operator-=(Vector3& a, const Vector3& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }

inline float Vec3Dot(const Vector3& a, const Vector3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vector3 Vec3Cross(const Vector3& a, const Vector3& b) {
    return Vector3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

inline Vector3 Vec3Normalize(const Vector3& v) {
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 0.00001f) {
        return Vector3(v.x / len, v.y / len, v.z / len);
    }
    return Vector3(0.0f, 0.0f, 0.0f);
}

struct Matrix4 {
    float m[4][4];
    Matrix4() {
        std::memset(m, 0, sizeof(m));
    }
    static Matrix4 Identity() {
        Matrix4 mat;
        mat.m[0][0] = mat.m[1][1] = mat.m[2][2] = mat.m[3][3] = 1.0f;
        return mat;
    }
};

inline Matrix4 operator*(const Matrix4& a, const Matrix4& b) {
    Matrix4 r;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r.m[i][j] = a.m[i][0] * b.m[0][j] +
                        a.m[i][1] * b.m[1][j] +
                        a.m[i][2] * b.m[2][j] +
                        a.m[i][3] * b.m[3][j];
        }
    }
    return r;
}

inline Matrix4 MatrixTranslation(float x, float y, float z) {
    Matrix4 r = Matrix4::Identity();
    r.m[3][0] = x;
    r.m[3][1] = y;
    r.m[3][2] = z;
    return r;
}

inline Matrix4 MatrixScaling(float x, float y, float z) {
    Matrix4 r = Matrix4::Identity();
    r.m[0][0] = x;
    r.m[1][1] = y;
    r.m[2][2] = z;
    return r;
}

inline Matrix4 MatrixRotationY(float angle) {
    Matrix4 r = Matrix4::Identity();
    float c = cosf(angle);
    float s = sinf(angle);
    // D3D left-handed Y rotation (matches Joymax / D3DXMatrixRotationY)
    r.m[0][0] = c;
    r.m[0][2] = s;
    r.m[2][0] = -s;
    r.m[2][2] = c;
    return r;
}

inline Matrix4 MatrixLookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up) {
    Vector3 zaxis = Vec3Normalize(target - eye);
    Vector3 xaxis = Vec3Normalize(Vec3Cross(up, zaxis));
    Vector3 yaxis = Vec3Cross(zaxis, xaxis);

    Matrix4 r = Matrix4::Identity();
    r.m[0][0] = xaxis.x; r.m[1][0] = xaxis.y; r.m[2][0] = xaxis.z;
    r.m[0][1] = yaxis.x; r.m[1][1] = yaxis.y; r.m[2][1] = yaxis.z;
    r.m[0][2] = zaxis.x; r.m[1][2] = zaxis.y; r.m[2][2] = zaxis.z;

    r.m[3][0] = -Vec3Dot(xaxis, eye);
    r.m[3][1] = -Vec3Dot(yaxis, eye);
    r.m[3][2] = -Vec3Dot(zaxis, eye);
    return r;
}

inline Matrix4 MatrixPerspectiveFovLH(float fov, float aspect, float zn, float zf) {
    float yscale = 1.0f / tanf(fov / 2.0f);
    float xscale = yscale / aspect;

    Matrix4 r;
    r.m[0][0] = xscale;
    r.m[1][1] = yscale;
    r.m[2][2] = zf / (zf - zn);
    r.m[2][3] = 1.0f;
    r.m[3][2] = -zn * zf / (zf - zn);
    return r;
}

inline Matrix4 MatrixOrthoOffCenterLH(float l, float r, float b, float t, float zn, float zf) {
    Matrix4 mat = Matrix4::Identity();
    mat.m[0][0] = 2.0f / (r - l);
    mat.m[1][1] = 2.0f / (t - b);
    mat.m[2][2] = 1.0f / (zf - zn);
    mat.m[3][0] = (l + r) / (l - r);
    mat.m[3][1] = (t + b) / (b - t);
    mat.m[3][2] = zn / (zn - zf);
    return mat;
}
