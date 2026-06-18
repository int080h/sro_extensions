#pragma once
#include <Windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#define M_E        static_cast<float>(2.71828182845904523536)   // e
#define M_LOG2E    static_cast<float>(1.44269504088896340736)   // log2(e)
#define M_LOG10E   static_cast<float>(0.434294481903251827651)  // log10(e)
#define M_LN2      static_cast<float>(0.693147180559945309417)  // ln(2)
#define M_LN10     static_cast<float>(2.30258509299404568402)   // ln(10)
#define M_PI       static_cast<float>(3.14159265358979323846)   // pi
#define M_PI_2     static_cast<float>(1.57079632679489661923)   // pi/2
#define M_PI_4     static_cast<float>(0.785398163397448309616)  // pi/4
#define M_1_PI     static_cast<float>(0.318309886183790671538)  // 1/pi
#define M_2_PI     static_cast<float>(0.636619772367581343076)  // 2/pi
#define M_2_SQRTPI static_cast<float>(1.12837916709551257390)   // 2/sqrt(pi)
#define M_SQRT2    static_cast<float>(1.41421356237309504880)   // sqrt(2)
#define M_SQRT1_2  static_cast<float>(0.707106781186547524401)  // 1/sqrt(2)
#define M_3D_TOL   static_cast<float>(0.0001)
#define M_TO_DEGREE(radian) ((radian) * (180.0f / M_PI))
#define M_TO_RADIAN(degree) ((degree) * (M_PI / 180.0f))

struct vector2f;
struct vector3f;
struct projection_info;
struct intersection_result;

//------------------------------------------------------------------------------
// 2D vector
struct vector2f : DirectX::XMFLOAT2 {
  vector2f() noexcept;
  explicit vector2f(float ix) noexcept;
  vector2f(float ix, float iy) noexcept;
  explicit vector2f(const vector2f* vec) noexcept;
  explicit vector2f(const vector3f* vec) noexcept;
  explicit vector2f(const vector3f& vec) noexcept;
  explicit vector2f(const XMFLOAT2& v) noexcept;
  explicit vector2f(DirectX::FXMVECTOR v) noexcept;

  vector2f(const vector2f&) = default;
  vector2f& operator=(const vector2f&) = default;
  vector2f(vector2f&&) = default;
  vector2f& operator=(vector2f&&) = default;

  // Casting
  explicit operator float*() noexcept;
  explicit operator const float*() const noexcept;

  // Comparison operators
  bool operator ==(const vector2f& v) const noexcept;
  bool operator !=(const vector2f& v) const noexcept;

  // Assignment operators
  vector2f& operator=(const DirectX::XMVECTORF32& f) noexcept;
  vector2f& operator+=(const vector2f& v) noexcept;
  vector2f& operator-=(const vector2f& v) noexcept;
  vector2f& operator*=(const vector2f& v) noexcept;
  vector2f& operator*=(float s) noexcept;
  vector2f& operator/=(float s) noexcept;

  // Unary operators
  vector2f operator+() const noexcept;
  vector2f operator-() const noexcept;

  // Vector operations
  auto is_valid() const noexcept -> bool;
  auto angle_between(const vector2f& v2) const noexcept -> float;
  auto angle_between(const vector3f& v2) const noexcept -> float;
  auto dot(const vector2f& v2) const noexcept -> float;
  auto cross(const vector2f& v2) const noexcept -> vector2f;
  auto distance(const vector2f& v2) const noexcept -> float;
  auto distance(const vector3f& v2) const noexcept -> float;
  auto distance(const vector2f& segment_start, const vector2f& segment_end, bool only_if_on_segment = false) const noexcept -> float;
  auto distance_sqr(const vector2f& v2) const noexcept -> float;
  auto distance_sqr(const vector3f& v2) const noexcept -> float;
  auto distance_sqr(const vector2f& segment_start, const vector2f& segment_end, bool only_if_on_segment = false) const noexcept -> float;
  auto extended(const vector2f& v2, float distance) const noexcept -> vector2f;
  auto extended(const vector3f& v2, float distance) const noexcept -> vector2f;
  auto rotated(float angle) const noexcept -> vector2f;
  auto rotated(const vector2f& v2, float angle) const noexcept -> vector2f;
  auto project_on(const vector2f& segment_start, const vector2f& segment_end) const noexcept -> projection_info;
  auto intersection(const vector2f& line_segment_end, const vector2f& line_segment2_start, const vector2f& line_segment2_end) const noexcept -> intersection_result;
  auto length() const noexcept -> float;
  auto length_sqr() const noexcept -> float;
  auto normalize() const noexcept -> vector2f;
  auto perpendicular(bool inverse = false) const noexcept -> vector2f;
  auto clamp(const vector2f& vmin, const vector2f& vmax) noexcept -> void;
  auto clamp(const vector2f& vmin, const vector2f& vmax, vector2f& result) const noexcept -> void;
  auto to_3d(float h = 0.0f) const noexcept -> vector3f;

  // Static functions
  static auto distance(const std::vector<vector2f>& container) noexcept -> float;
  static auto distance(const vector2f& v1, const vector2f& v2) noexcept -> float;
  static auto distance_sqr(const vector2f& v1, const vector2f& v2) noexcept -> float;
  static auto cross_product(const vector2f& v1, const vector2f& v2) noexcept -> float;
  static auto min(const vector2f& v1, const vector2f& v2, vector2f& result) noexcept -> void;
  static auto min(const vector2f& v1, const vector2f& v2) noexcept -> vector2f;
  static auto max(const vector2f& v1, const vector2f& v2, vector2f& result) noexcept -> void;
  static auto max(const vector2f& v1, const vector2f& v2) noexcept -> vector2f;
  static auto lerp(const vector2f& v1, const vector2f& v2, float t, vector2f& result) noexcept -> void;
  static auto lerp(const vector2f& v1, const vector2f& v2, float t) noexcept -> vector2f;
  static auto smooth_step(const vector2f& v1, const vector2f& v2, float t, vector2f& result) noexcept -> void;
  static auto smooth_step(const vector2f& v1, const vector2f& v2, float t) noexcept -> vector2f;
  static auto barycentric(const vector2f& v1, const vector2f& v2, const vector2f& v3, float f, float g, vector2f& result) noexcept -> void;
  static auto barycentric(const vector2f& v1, const vector2f& v2, const vector2f& v3, float f, float g) noexcept -> vector2f;
  static auto catmull_rom(const vector2f& v1, const vector2f& v2, const vector2f& v3, const vector2f& v4, float t, vector2f& result) noexcept -> void;
  static auto catmull_rom(const vector2f& v1, const vector2f& v2, const vector2f& v3, const vector2f& v4, float t) noexcept -> vector2f;
  static auto hermite(const vector2f& v1, const vector2f& t1, const vector2f& v2, const vector2f& t2, float t, vector2f& result) noexcept -> void;
  static auto hermite(const vector2f& v1, const vector2f& t1, const vector2f& v2, const vector2f& t2, float t) noexcept -> vector2f;
  static auto reflect(const vector2f& ivec, const vector2f& nvec, vector2f& result) noexcept -> void;
  static auto reflect(const vector2f& ivec, const vector2f& nvec) noexcept -> vector2f;
  static auto refract(const vector2f& ivec, const vector2f& nvec, float refraction_index, vector2f& result) noexcept -> void;
  static auto refract(const vector2f& ivec, const vector2f& nvec, float refraction_index) noexcept -> vector2f;

  // Constants
  static const vector2f zero;
  static const vector2f one;
  static const vector2f unit_x;
  static const vector2f unit_y;
};

// Binary operators
vector2f operator+(const vector2f& v1, const vector2f& v2) noexcept;
vector2f operator-(const vector2f& v1, const vector2f& v2) noexcept;
vector2f operator*(const vector2f& v1, const vector2f& v2) noexcept;
vector2f operator*(const vector2f& v, float s) noexcept;
vector2f operator/(const vector2f& v1, const vector2f& v2) noexcept;
vector2f operator/(const vector2f& v, float s) noexcept;
vector2f operator*(float s, const vector2f& v) noexcept;

//------------------------------------------------------------------------------
// 3D vector
struct vector3f : DirectX::XMFLOAT3 {
  vector3f() noexcept;
  explicit vector3f(float ix) noexcept;
  vector3f(float ix, float iz) noexcept;
  vector3f(float ix, float iy, float iz) noexcept;
  explicit vector3f(const vector3f* vec) noexcept;
  explicit vector3f(const vector2f* vec) noexcept;
  explicit vector3f(const vector2f& vec) noexcept;
  explicit vector3f(const XMFLOAT3& v) noexcept;
  explicit vector3f(DirectX::FXMVECTOR v) noexcept;

  vector3f(const vector3f&) = default;
  vector3f& operator=(const vector3f&) = default;
  vector3f(vector3f&&) = default;
  vector3f& operator=(vector3f&&) = default;

  // Casting
  explicit operator float*() noexcept;
  explicit operator const float*() const noexcept;

  // Comparison operators
  bool operator ==(const vector3f& v) const noexcept;
  bool operator !=(const vector3f& v) const noexcept;

  // Assignment operators
  vector3f& operator=(const DirectX::XMVECTORF32& f) noexcept;
  vector3f& operator+=(const vector3f& v) noexcept;
  vector3f& operator-=(const vector3f& v) noexcept;
  vector3f& operator*=(const vector3f& v) noexcept;
  vector3f& operator*=(float s) noexcept;
  vector3f& operator/=(float s) noexcept;

  // Unary operators
  vector3f operator+() const noexcept;
  vector3f operator-() const noexcept;

  // Vector operations
  auto is_valid() const noexcept -> bool;
  auto angle_between(const vector3f& v2) const noexcept -> float;
  auto angle_between(const vector2f& v2) const noexcept -> float;
  auto dot(const vector3f& v2) const noexcept -> float;
  auto cross(const vector3f& v2) const noexcept -> vector3f;
  auto distance(const vector3f& v2) const noexcept -> float;
  auto distance(const vector3f* v2) const noexcept -> float;
  auto distance(const vector2f& v2) const noexcept -> float;
  auto distance(const vector3f& segment_start, const vector3f& segment_end, bool only_if_on_segment = false) const noexcept -> float;
  auto distance_sqr(const vector3f& v2) const noexcept -> float;
  auto distance_sqr(const vector3f* v2) const noexcept -> float;
  auto distance_sqr(const vector2f& v2) const noexcept -> float;
  auto distance_sqr(const vector3f& segment_start, const vector3f& segment_end, bool only_if_on_segment = false) const noexcept -> float;
  auto extended(const vector3f& v2, float distance) const noexcept -> vector3f;
  auto extended(const vector3f* v2, float distance) const noexcept -> vector3f;
  auto extended(const vector2f& v2, float distance) const noexcept -> vector3f;
  auto project_on(const vector3f& segment_start, const vector3f& segment_end) const noexcept -> projection_info;
  auto project_on(const vector2f& segment_start, const vector2f& segment_end) const noexcept -> projection_info;
  auto rotated(float angle) const noexcept -> vector3f;
  auto rotated(const vector3f& v2, float angle) const noexcept -> vector3f;
  auto length() const noexcept -> float;
  auto length_sqr() const noexcept -> float;
  auto normalize() const noexcept -> vector3f;
  auto perpendicular(bool inverse = false) const noexcept -> vector3f;
  auto clamp(const vector3f& vmin, const vector3f& vmax) noexcept -> void;
  auto clamp(const vector3f& vmin, const vector3f& vmax, vector3f& result) const noexcept -> void;
  auto to_2d() const noexcept -> vector2f;

  // Static functions
  static auto distance(const std::vector<vector3f>& container) noexcept -> float;
  static auto distance(const vector3f& v1, const vector3f& v2) noexcept -> float;
  static auto distance_sqr(const vector3f& v1, const vector3f& v2) noexcept -> float;
  static auto cross_product(const vector3f& v1, const vector3f& v2) noexcept -> float;
  static auto min(const vector3f& v1, const vector3f& v2, vector3f& result) noexcept -> void;
  static auto min(const vector3f& v1, const vector3f& v2) noexcept -> vector3f;
  static auto max(const vector3f& v1, const vector3f& v2, vector3f& result) noexcept -> void;
  static auto max(const vector3f& v1, const vector3f& v2) noexcept -> vector3f;
  static auto lerp(const vector3f& v1, const vector3f& v2, float t, vector3f& result) noexcept -> void;
  static auto lerp(const vector3f& v1, const vector3f& v2, float t) noexcept -> vector3f;
  static auto smooth_step(const vector3f& v1, const vector3f& v2, float t, vector3f& result) noexcept -> void;
  static auto smooth_step(const vector3f& v1, const vector3f& v2, float t) noexcept -> vector3f;
  static auto barycentric(const vector3f& v1, const vector3f& v2, const vector3f& v3, float f, float g, vector3f& result) noexcept -> void;
  static auto barycentric(const vector3f& v1, const vector3f& v2, const vector3f& v3, float f, float g) noexcept -> vector3f;
  static auto catmull_rom(const vector3f& v1, const vector3f& v2, const vector3f& v3, const vector3f& v4, float t, vector3f& result) noexcept -> void;
  static auto catmull_rom(const vector3f& v1, const vector3f& v2, const vector3f& v3, const vector3f& v4, float t) noexcept -> vector3f;
  static auto hermite(const vector3f& v1, const vector3f& t1, const vector3f& v2, const vector3f& t2, float t, vector3f& result) noexcept -> void;
  static auto hermite(const vector3f& v1, const vector3f& t1, const vector3f& v2, const vector3f& t2, float t) noexcept -> vector3f;
  static auto reflect(const vector3f& ivec, const vector3f& nvec, vector3f& result) noexcept -> void;
  static auto reflect(const vector3f& ivec, const vector3f& nvec) noexcept -> vector3f;
  static auto refract(const vector3f& ivec, const vector3f& nvec, float refraction_index, vector3f& result) noexcept -> void;
  static auto refract(const vector3f& ivec, const vector3f& nvec, float refraction_index) noexcept -> vector3f;
  static auto extended_length(const vector3f& v1, const vector3f& v2, float length) noexcept -> vector3f;

  // Constants
  static const vector3f zero;
  static const vector3f one;
  static const vector3f unitx;
  static const vector3f unity;
  static const vector3f unitz;
  static const vector3f up;
  static const vector3f down;
  static const vector3f right;
  static const vector3f left;
  static const vector3f forward;
  static const vector3f backward;
};

// Binary operators
vector3f operator+(const vector3f& v1, const vector3f& v2) noexcept;
vector3f operator-(const vector3f& v1, const vector3f& v2) noexcept;
vector3f operator*(const vector3f& v1, const vector3f& v2) noexcept;
vector3f operator*(const vector3f& v, float s) noexcept;
vector3f operator/(const vector3f& v1, const vector3f& v2) noexcept;
vector3f operator/(const vector3f& v, float s) noexcept;
vector3f operator*(float s, const vector3f& v) noexcept;

//------------------------------------------------------------------------------
// Helper utilities
struct projection_info {
  bool is_on_segment;
  vector2f segment_point;
  vector2f line_point;

  projection_info()
    : is_on_segment(false) {
  }

  projection_info(const bool is_on_segment, const vector2f& segment_point, const vector2f& line_point)
    : is_on_segment(is_on_segment),
      segment_point(segment_point),
      line_point(line_point) {
  }
};

struct intersection_result {
  bool intersects;
  vector2f point;

  intersection_result()
    : intersects(false) {
  }

  intersection_result(const bool intersects, const vector2f& point)
    : intersects(intersects),
      point(point) {
  }
};

struct sdk_color {
  DirectX::XMFLOAT4 value {};

  sdk_color() {
    value.x = value.y = value.z = value.w = 0.0f;
  }
  sdk_color(int r, int g, int b, int a = 255) {
    constexpr float sc = 1.0f / 255.0f;
    value.x = static_cast<float>(r) * sc;
    value.y = static_cast<float>(g) * sc;
    value.z = static_cast<float>(b) * sc;
    value.w = static_cast<float>(a) * sc;
  }
  sdk_color(float r, float g, float b, float a = 1.0f) {
    value.x = r;
    value.y = g;
    value.z = b;
    value.w = a;
  }
  sdk_color(const sdk_color& col) {
    value = col.value;
  }
  explicit sdk_color(unsigned rgba) {
    constexpr float sc = 1.0f / 255.0f;
    value.x = static_cast<float>(rgba >> 0 & 0xFF) * sc;
    value.y = static_cast<float>(rgba >> 8 & 0xFF) * sc;
    value.z = static_cast<float>(rgba >> 16 & 0xFF) * sc;
    value.w = static_cast<float>(rgba >> 24 & 0xFF) * sc;
  }
  explicit operator unsigned() const {
    unsigned out = static_cast<unsigned>(f32_to_int8_sat(value.x)) << 0;
    out |= static_cast<unsigned>(f32_to_int8_sat(value.y)) << 8;
    out |= static_cast<unsigned>(f32_to_int8_sat(value.z)) << 16;
    out |= static_cast<unsigned>(f32_to_int8_sat(value.w)) << 24;
    return out;
  }
  explicit operator DirectX::XMFLOAT4() const {
    return value;
  }
private:
  static float saturate(float f) { return f < 0.0f ? 0.0f : f > 1.0f ? 1.0f : f; }
  static float f32_to_int8_sat(float f) { return saturate(f) * 255.f + 0.5f; }
};

#include "vectorf.inl"
