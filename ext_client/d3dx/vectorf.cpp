#include "vectorf.h"

const vector2f vector2f::zero = { 0.f, 0.f };
const vector2f vector2f::one = { 1.f, 1.f };
const vector2f vector2f::unit_x = { 1.f, 0.f };
const vector2f vector2f::unit_y = { 0.f, 1.f };

const vector3f vector3f::zero = { 0.f, 0.f, 0.f };
const vector3f vector3f::one = { 1.f, 1.f, 1.f };
const vector3f vector3f::unitx = { 1.f, 0.f, 0.f };
const vector3f vector3f::unity = { 0.f, 1.f, 0.f };
const vector3f vector3f::unitz = { 0.f, 0.f, 1.f };
const vector3f vector3f::up = { 0.f, 1.f, 0.f };
const vector3f vector3f::down = { 0.f, -1.f, 0.f };
const vector3f vector3f::right = { 1.f, 0.f, 0.f };
const vector3f vector3f::left = { -1.f, 0.f, 0.f };
const vector3f vector3f::forward = { 0.f, 0.f, -1.f };
const vector3f vector3f::backward = { 0.f, 0.f, 1.f };

/****************************************************************************
 *
 * Vector2
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

auto vector2f::is_valid() const noexcept -> bool {
  return x != 0 && y != 0;
}

auto vector2f::angle_between(const vector2f& v2) const noexcept -> float {
  using namespace DirectX;
  const XMVECTOR x1 = XMVector2Normalize(XMLoadFloat2(this));
  const XMVECTOR x2 = XMVector2Normalize(XMLoadFloat2(&v2));
  const XMVECTOR X = XMVector2AngleBetweenVectors(x1, x2);
  float angle_in_radians;
  XMStoreFloat(&angle_in_radians, X);
  return XMConvertToDegrees(angle_in_radians);
}

auto vector2f::angle_between(const vector3f& v2) const noexcept -> float {
  return angle_between(v2.to_2d());
}

auto vector2f::dot(const vector2f& v2) const noexcept -> float {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat2(this);
  const XMVECTOR vec2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVector2Dot(vec1, vec2);
  return XMVectorGetX(X);
}

auto vector2f::cross(const vector2f& v2) const noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat2(this);
  const XMVECTOR vec2 = XMLoadFloat2(&v2);
  const XMVECTOR R = XMVector2Cross(vec1, vec2);

  vector2f result;
  XMStoreFloat2(&result, R);
  return result;
}

auto vector2f::distance(const vector2f& v2) const noexcept -> float {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(this);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR V = XMVectorSubtract(x2, x1);
  const XMVECTOR X = XMVector2Length(V);
  return XMVectorGetX(X);
}

auto vector2f::distance(const vector3f& v2) const noexcept -> float {
  return distance(v2.to_2d());
}

auto vector2f::distance(const vector2f& segment_start, const vector2f& segment_end, bool only_if_on_segment) const noexcept -> float {
  const auto objects = project_on(segment_start, segment_end);
  return objects.is_on_segment || !only_if_on_segment ? distance(objects.segment_point) : std::numeric_limits<float>::max();
}

auto vector2f::distance_sqr(const vector2f& v2) const noexcept -> float {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(this);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR V = XMVectorSubtract(x2, x1);
  const XMVECTOR X = XMVector2LengthSq(V);
  return XMVectorGetX(X);
}

auto vector2f::distance_sqr(const vector3f& v2) const noexcept -> float {
  return distance_sqr(v2.to_2d());
}

auto vector2f::distance_sqr(const vector2f& segment_start, const vector2f& segment_end, bool only_if_on_segment) const noexcept -> float {
  const auto objects = project_on(segment_start, segment_end);
  return objects.is_on_segment || !only_if_on_segment ? distance_sqr(objects.segment_point) : std::numeric_limits<float>::max();
}

auto vector2f::extended(const vector2f& v2, float distance) const noexcept -> vector2f {
  return *this + (v2 - *this).normalize() * distance;
}

auto vector2f::extended(const vector3f& v2, float distance) const noexcept -> vector2f {
  return *this + (v2.to_2d() - *this).normalize() * distance;
}

auto vector2f::rotated(float angle) const noexcept -> vector2f {
  const auto cos = std::cosf(angle);
  const auto sin = std::sinf(angle);
  return { x * cos - y * sin, y * cos + x * sin };
}

auto vector2f::rotated(const vector2f& v2, float angle) const noexcept -> vector2f {
  return (*this - v2).rotated(angle) + v2;
}

auto vector2f::project_on(const vector2f& segment_start, const vector2f& segment_end) const noexcept -> projection_info {
  constexpr float EPSILON = 1e-6f;

  // Calculate the vector from segment start to end
  vector2f segment_vec = segment_end - segment_start;
  float segment_length_sq = segment_vec.length_sqr();

  // Handle degenerate case (zero-length segment)
  if (segment_length_sq < EPSILON * EPSILON) {
    return projection_info(true, segment_start, segment_start);
  }

  // Calculate the vector from segment start to this point
  vector2f point_vec = *this - segment_start;

  // Calculate the projection scalar
  float t = point_vec.dot(segment_vec) / segment_length_sq;

  // Calculate the projected point on the infinite line
  vector2f point_line = segment_start + segment_vec * t;

  // Clamp t to [0, 1] for the segment
  float t_clamped = std::clamp(t, 0.0f, 1.0f);
  vector2f point_segment = segment_start + segment_vec * t_clamped;

  // Check if the projection is on the segment
  bool is_on_segment = (std::abs(t - t_clamped) < EPSILON);

  return projection_info(is_on_segment, point_segment, point_line);
}

auto vector2f::intersection(const vector2f& line_segment_end, const vector2f& line_segment2_start, const vector2f& line_segment2_end) const noexcept -> intersection_result {
  constexpr float EPSILON = 1e-6f;
  // For clarity, treat this as the ray origin 'p'
  const auto p = *this;
  const auto q = line_segment_end; // p -> q is our "ray"
  const auto r = line_segment2_start; // edge start
  const auto s = line_segment2_end; // edge end
  const auto v1 = q - p; // ray direction (not necessarily normalized)
  const auto v2 = s - r; // edge vector
  // Compute the 2D cross product between v1 and v2.
  const auto cross = v1.x * v2.y - v1.y * v2.x;
  // If the cross product is near zero, the segments are parallel or collinear.
  if (std::abs(cross) < EPSILON) {
    // Check collinearity
    if (std::abs((r - p).x * v1.y - (r - p).y * v1.x) < EPSILON) {
      // They are collinear; compute parameters along the ray for the edge endpoints.
      const auto t0 = ((r - p).dot(v1)) / v1.length_sqr();
      const auto t1 = t0 + (v2.dot(v1)) / v1.length_sqr();
      // We only accept intersections that occur in front of the ray origin (t > EPSILON)
      if ((t0 > EPSILON && t0 <= 1) || (t1 > EPSILON && t1 <= 1) ||
        (t0 < EPSILON && t1 > 1) || (t0 > 1 && t1 < 0)) {
        // Return the point closest to p (with t clamped between 0 and 1)
        return { true, p + v1 * std::clamp(t0, 0.0f, 1.0f) };
      }
    }
    return { false, vector2f() };
  }
  // Compute parameters t and u such that:
  // p + v1 * t = r + v2 * u
  const auto t = ((r - p).x * v2.y - (r - p).y * v2.x) / cross;
  const auto u = ((r - p).x * v1.y - (r - p).y * v1.x) / cross;
  // Accept only intersections that are in front of the ray origin (t > EPSILON)
  // and lie within the edge segment (u between 0 and 1).
  if (t > EPSILON && t <= 1 && u >= 0 && u <= 1) {
    return { true, p + v1 * t };
  }
  return { false, vector2f() };
}

auto vector2f::length() const noexcept -> float {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR X = XMVector2Length(v1);
  return XMVectorGetX(X);
}

auto vector2f::length_sqr() const noexcept -> float {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR X = XMVector2LengthSq(v1);
  return XMVectorGetX(X);
}

auto vector2f::normalize() const noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR X = XMVector2Normalize(v1);
  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::perpendicular(bool inverse) const noexcept -> vector2f {
  return inverse ? vector2f(y, -x) : vector2f(-y, x);
}

auto vector2f::clamp(const vector2f& vmin, const vector2f& vmax) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR v2 = XMLoadFloat2(&vmin);
  const XMVECTOR v3 = XMLoadFloat2(&vmax);
  const XMVECTOR X = XMVectorClamp(v1, v2, v3);
  XMStoreFloat2(this, X);
}

auto vector2f::clamp(const vector2f& vmin, const vector2f& vmax, vector2f& result) const noexcept -> void {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR v2 = XMLoadFloat2(&vmin);
  const XMVECTOR v3 = XMLoadFloat2(&vmax);
  const XMVECTOR X = XMVectorClamp(v1, v2, v3);
  XMStoreFloat2(&result, X);
}

auto vector2f::to_3d(float h) const noexcept -> vector3f {
  return { x, h, y };
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

auto vector2f::distance(const std::vector<vector2f>& container) noexcept -> float {
  if (container.size() < 2)
    return 0.f;
  auto result = 0.f;
  for (size_t i = 0; i < container.size() - 1; i++)
    result += container[i].distance(container[i + 1]);
  return result;
}

auto vector2f::distance(const vector2f& v1, const vector2f& v2) noexcept -> float {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR V = XMVectorSubtract(x2, x1);
  const XMVECTOR X = XMVector2Length(V);
  return XMVectorGetX(X);
}

auto vector2f::distance_sqr(const vector2f& v1, const vector2f& v2) noexcept -> float {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR V = XMVectorSubtract(x2, x1);
  const XMVECTOR X = XMVector2LengthSq(V);
  return XMVectorGetX(X);
}

auto vector2f::cross_product(const vector2f& v1, const vector2f& v2) noexcept -> float {
  return v2.y * v1.x - v2.x * v1.y;
}

auto vector2f::min(const vector2f& v1, const vector2f& v2, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorMin(x1, x2);
  XMStoreFloat2(&result, X);
}

auto vector2f::min(const vector2f& v1, const vector2f& v2) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorMin(x1, x2);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::max(const vector2f& v1, const vector2f& v2, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorMax(x1, x2);
  XMStoreFloat2(&result, X);
}

auto vector2f::max(const vector2f& v1, const vector2f& v2) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorMax(x1, x2);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::lerp(const vector2f& v1, const vector2f& v2, float t, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);
  XMStoreFloat2(&result, X);
}

auto vector2f::lerp(const vector2f& v1, const vector2f& v2, float t) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::smooth_step(const vector2f& v1, const vector2f& v2, float t, vector2f& result) noexcept -> void {
  using namespace DirectX;
  t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
  t = t * t * (3.f - 2.f * t);
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);
  XMStoreFloat2(&result, X);
}

auto vector2f::smooth_step(const vector2f& v1, const vector2f& v2, float t) noexcept -> vector2f {
  using namespace DirectX;
  t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
  t = t * t * (3.f - 2.f * t);
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::barycentric(const vector2f& v1, const vector2f& v2, const vector2f& v3, float f, float g, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR x3 = XMLoadFloat2(&v3);
  const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
  XMStoreFloat2(&result, X);
}

auto vector2f::barycentric(const vector2f& v1, const vector2f& v2, const vector2f& v3, float f, float g) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR x3 = XMLoadFloat2(&v3);
  const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::catmull_rom(const vector2f& v1, const vector2f& v2, const vector2f& v3, const vector2f& v4, float t, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR x3 = XMLoadFloat2(&v3);
  const XMVECTOR x4 = XMLoadFloat2(&v4);
  const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
  XMStoreFloat2(&result, X);
}

auto vector2f::catmull_rom(const vector2f& v1, const vector2f& v2, const vector2f& v3, const vector2f& v4, float t) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&v2);
  const XMVECTOR x3 = XMLoadFloat2(&v3);
  const XMVECTOR x4 = XMLoadFloat2(&v4);
  const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::hermite(const vector2f& v1, const vector2f& t1, const vector2f& v2, const vector2f& t2, float t, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&t1);
  const XMVECTOR x3 = XMLoadFloat2(&v2);
  const XMVECTOR x4 = XMLoadFloat2(&t2);
  const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
  XMStoreFloat2(&result, X);
}

auto vector2f::hermite(const vector2f& v1, const vector2f& t1, const vector2f& v2, const vector2f& t2, float t) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat2(&v1);
  const XMVECTOR x2 = XMLoadFloat2(&t1);
  const XMVECTOR x3 = XMLoadFloat2(&v2);
  const XMVECTOR x4 = XMLoadFloat2(&t2);
  const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::reflect(const vector2f& ivec, const vector2f& nvec, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat2(&ivec);
  const XMVECTOR n = XMLoadFloat2(&nvec);
  const XMVECTOR X = XMVector2Reflect(i, n);
  XMStoreFloat2(&result, X);
}

auto vector2f::reflect(const vector2f& ivec, const vector2f& nvec) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat2(&ivec);
  const XMVECTOR n = XMLoadFloat2(&nvec);
  const XMVECTOR X = XMVector2Reflect(i, n);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

auto vector2f::refract(const vector2f& ivec, const vector2f& nvec, float refraction_index, vector2f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat2(&ivec);
  const XMVECTOR n = XMLoadFloat2(&nvec);
  const XMVECTOR X = XMVector2Refract(i, n, refraction_index);
  XMStoreFloat2(&result, X);
}

auto vector2f::refract(const vector2f& ivec, const vector2f& nvec, float refraction_index) noexcept -> vector2f {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat2(&ivec);
  const XMVECTOR n = XMLoadFloat2(&nvec);
  const XMVECTOR X = XMVector2Refract(i, n, refraction_index);

  vector2f result;
  XMStoreFloat2(&result, X);
  return result;
}

/****************************************************************************
 *
 * Vector3
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Vector operations
//------------------------------------------------------------------------------

auto vector3f::is_valid() const noexcept -> bool {
  return x != 0 && z != 0;
}

auto vector3f::angle_between(const vector3f& v2) const noexcept -> float {
  return to_2d().angle_between(v2.to_2d());
}

auto vector3f::angle_between(const vector2f& v2) const noexcept -> float {
  return to_2d().angle_between(v2);
}

auto vector3f::dot(const vector3f& v2) const noexcept -> float {
  return to_2d().dot(v2.to_2d());
}

auto vector3f::cross(const vector3f& v2) const noexcept -> vector3f {
  return to_2d().cross(v2.to_2d()).to_3d();
}

auto vector3f::distance(const vector3f& v2) const noexcept -> float {
  return to_2d().distance(v2.to_2d());
}

auto vector3f::distance(const vector3f* v2) const noexcept -> float {
  return to_2d().distance(v2->to_2d());
}

auto vector3f::distance(const vector2f& v2) const noexcept -> float {
  return to_2d().distance(v2);
}

auto vector3f::distance(const vector3f& segment_start, const vector3f& segment_end, bool only_if_on_segment) const noexcept -> float {
  const auto projection_info = project_on(segment_start, segment_end);
  return projection_info.is_on_segment || !only_if_on_segment ? distance(projection_info.segment_point) : std::numeric_limits<float>::max();
}

auto vector3f::distance_sqr(const vector3f& v2) const noexcept -> float {
  return to_2d().distance_sqr(v2.to_2d());
}

auto vector3f::distance_sqr(const vector3f* v2) const noexcept -> float {
  return to_2d().distance_sqr(v2->to_2d());
}

auto vector3f::distance_sqr(const vector2f& v2) const noexcept -> float {
  return to_2d().distance_sqr(v2);
}

auto vector3f::distance_sqr(const vector3f& segment_start, const vector3f& segment_end, bool only_if_on_segment) const noexcept -> float {
  const auto projection_info = project_on(segment_start, segment_end);
  return projection_info.is_on_segment || !only_if_on_segment ? distance_sqr(projection_info.segment_point) : std::numeric_limits<float>::max();
}

auto vector3f::extended(const vector3f& v2, float distance) const noexcept -> vector3f {
  return to_2d().extended(v2.to_2d(), distance).to_3d(y);
}

auto vector3f::extended(const vector3f* v2, float distance) const noexcept -> vector3f {
  return to_2d().extended(v2->to_2d(), distance).to_3d(y);
}

auto vector3f::extended(const vector2f& v2, float distance) const noexcept -> vector3f {
  return to_2d().extended(v2, distance).to_3d(y);
}

auto vector3f::project_on(const vector3f& segment_start, const vector3f& segment_end) const noexcept -> projection_info {
  return to_2d().project_on(segment_start.to_2d(), segment_end.to_2d());
}

auto vector3f::project_on(const vector2f& segment_start, const vector2f& segment_end) const noexcept -> projection_info {
  return to_2d().project_on(segment_start, segment_end);
}

auto vector3f::rotated(float angle) const noexcept -> vector3f {
  return to_2d().rotated(angle).to_3d(y);
}

auto vector3f::rotated(const vector3f& v2, float angle) const noexcept -> vector3f {
  return to_2d().rotated(v2.to_2d(), angle).to_3d(y);
}

auto vector3f::length() const noexcept -> float {
  return to_2d().length();
}

auto vector3f::length_sqr() const noexcept -> float {
  return to_2d().length_sqr();
}

auto vector3f::normalize() const noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR X = XMVector3Normalize(v1);
  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::perpendicular(bool inverse) const noexcept -> vector3f {
  return inverse ? vector3f(-z, y, x) : vector3f(z, y, -x);
}

auto vector3f::clamp(const vector3f& vmin, const vector3f& vmax) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR v2 = XMLoadFloat3(&vmin);
  const XMVECTOR v3 = XMLoadFloat3(&vmax);
  const XMVECTOR X = XMVectorClamp(v1, v2, v3);
  XMStoreFloat3(this, X);
}

auto vector3f::clamp(const vector3f& vmin, const vector3f& vmax, vector3f& result) const noexcept -> void {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR v2 = XMLoadFloat3(&vmin);
  const XMVECTOR v3 = XMLoadFloat3(&vmax);
  const XMVECTOR X = XMVectorClamp(v1, v2, v3);
  XMStoreFloat3(&result, X);
}

auto vector3f::to_2d() const noexcept -> vector2f {
  return { x, z };
}

//------------------------------------------------------------------------------
// Static functions
//------------------------------------------------------------------------------

auto vector3f::distance(const std::vector<vector3f>& container) noexcept -> float {
  if (container.size() < 2)
    return 0.f;
  auto result = 0.f;
  for (size_t i = 0; i < container.size() - 1; i++)
    result += container[i].distance(container[i + 1]);
  return result;
}

auto vector3f::distance(const vector3f& v1, const vector3f& v2) noexcept -> float {
  return v1.to_2d().distance(v2.to_2d());
}

auto vector3f::distance_sqr(const vector3f& v1, const vector3f& v2) noexcept -> float {
  return v1.to_2d().distance_sqr(v2.to_2d());
}

auto vector3f::cross_product(const vector3f& v1, const vector3f& v2) noexcept -> float {
  return v2.z * v1.x - v2.x * v1.z;
}

auto vector3f::min(const vector3f& v1, const vector3f& v2, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorMin(x1, x2);
  XMStoreFloat3(&result, X);
}

auto vector3f::min(const vector3f& v1, const vector3f& v2) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorMin(x1, x2);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::max(const vector3f& v1, const vector3f& v2, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorMax(x1, x2);
  XMStoreFloat3(&result, X);
}

auto vector3f::max(const vector3f& v1, const vector3f& v2) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorMax(x1, x2);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::lerp(const vector3f& v1, const vector3f& v2, float t, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);
  XMStoreFloat3(&result, X);
}

auto vector3f::lerp(const vector3f& v1, const vector3f& v2, float t) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::smooth_step(const vector3f& v1, const vector3f& v2, float t, vector3f& result) noexcept -> void {
  using namespace DirectX;
  t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
  t = t * t * (3.f - 2.f * t);
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);
  XMStoreFloat3(&result, X);
}

auto vector3f::smooth_step(const vector3f& v1, const vector3f& v2, float t) noexcept -> vector3f {
  using namespace DirectX;
  t = (t > 1.0f) ? 1.0f : ((t < 0.0f) ? 0.0f : t); // Clamp value to 0 to 1
  t = t * t * (3.f - 2.f * t);
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorLerp(x1, x2, t);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::barycentric(const vector3f& v1, const vector3f& v2, const vector3f& v3, float f, float g, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR x3 = XMLoadFloat3(&v3);
  const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);
  XMStoreFloat3(&result, X);
}

auto vector3f::barycentric(const vector3f& v1, const vector3f& v2, const vector3f& v3, float f, float g) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR x3 = XMLoadFloat3(&v3);
  const XMVECTOR X = XMVectorBaryCentric(x1, x2, x3, f, g);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::catmull_rom(const vector3f& v1, const vector3f& v2, const vector3f& v3, const vector3f& v4, float t, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR x3 = XMLoadFloat3(&v3);
  const XMVECTOR x4 = XMLoadFloat3(&v4);
  const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);
  XMStoreFloat3(&result, X);
}

auto vector3f::catmull_rom(const vector3f& v1, const vector3f& v2, const vector3f& v3, const vector3f& v4, float t) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&v2);
  const XMVECTOR x3 = XMLoadFloat3(&v3);
  const XMVECTOR x4 = XMLoadFloat3(&v4);
  const XMVECTOR X = XMVectorCatmullRom(x1, x2, x3, x4, t);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::hermite(const vector3f& v1, const vector3f& t1, const vector3f& v2, const vector3f& t2, float t, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&t1);
  const XMVECTOR x3 = XMLoadFloat3(&v2);
  const XMVECTOR x4 = XMLoadFloat3(&t2);
  const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);
  XMStoreFloat3(&result, X);
}

auto vector3f::hermite(const vector3f& v1, const vector3f& t1, const vector3f& v2, const vector3f& t2, float t) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR x1 = XMLoadFloat3(&v1);
  const XMVECTOR x2 = XMLoadFloat3(&t1);
  const XMVECTOR x3 = XMLoadFloat3(&v2);
  const XMVECTOR x4 = XMLoadFloat3(&t2);
  const XMVECTOR X = XMVectorHermite(x1, x2, x3, x4, t);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::reflect(const vector3f& ivec, const vector3f& nvec, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat3(&ivec);
  const XMVECTOR n = XMLoadFloat3(&nvec);
  const XMVECTOR X = XMVector3Reflect(i, n);
  XMStoreFloat3(&result, X);
}

auto vector3f::reflect(const vector3f& ivec, const vector3f& nvec) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat3(&ivec);
  const XMVECTOR n = XMLoadFloat3(&nvec);
  const XMVECTOR X = XMVector3Reflect(i, n);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::refract(const vector3f& ivec, const vector3f& nvec, float refraction_index, vector3f& result) noexcept -> void {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat3(&ivec);
  const XMVECTOR n = XMLoadFloat3(&nvec);
  const XMVECTOR X = XMVector3Refract(i, n, refraction_index);
  XMStoreFloat3(&result, X);
}

auto vector3f::refract(const vector3f& ivec, const vector3f& nvec, float refraction_index) noexcept -> vector3f {
  using namespace DirectX;
  const XMVECTOR i = XMLoadFloat3(&ivec);
  const XMVECTOR n = XMLoadFloat3(&nvec);
  const XMVECTOR X = XMVector3Refract(i, n, refraction_index);

  vector3f result;
  XMStoreFloat3(&result, X);
  return result;
}

auto vector3f::extended_length(const vector3f& v1, const vector3f& v2, float length) noexcept -> vector3f {
  return v1.extended(v2, length);
}
