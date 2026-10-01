#pragma once

/****************************************************************************
 *
 * Vector2
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------

inline vector2f::vector2f() noexcept
  : XMFLOAT2(0.f, 0.f) {
}

inline vector2f::vector2f(float ix) noexcept
  : XMFLOAT2(ix, ix) {
}

inline vector2f::vector2f(float ix, float iy) noexcept
  : XMFLOAT2(ix, iy) {
}

inline vector2f::vector2f(const vector2f* vec) noexcept
  : XMFLOAT2(vec->x, vec->y) {
}

inline vector2f::vector2f(const vector3f* vec) noexcept
  : XMFLOAT2(vec->x, vec->z) {
}

inline vector2f::vector2f(const vector3f& vec) noexcept
  : XMFLOAT2(vec.x, vec.z) {
}

inline vector2f::vector2f(const XMFLOAT2& v) noexcept
  : XMFLOAT2(v) {
}

inline vector2f::vector2f(DirectX::FXMVECTOR v) noexcept
  : XMFLOAT2() {
  XMStoreFloat2(this, v);
}

//------------------------------------------------------------------------------
// Casting
//------------------------------------------------------------------------------

inline vector2f::operator float*() noexcept {
  return &x;
}

inline vector2f::operator const float*() const noexcept {
  return &x;
}

//------------------------------------------------------------------------------
// Comparision operators
//------------------------------------------------------------------------------

inline bool vector2f::operator ==(const vector2f& v) const noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR v2 = XMLoadFloat2(&v);
  return XMVector2Equal(v1, v2);
}

inline bool vector2f::operator !=(const vector2f& v) const noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR v2 = XMLoadFloat2(&v);
  return XMVector2NotEqual(v1, v2);
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline vector2f& vector2f::operator=(const DirectX::XMVECTORF32& f) noexcept {
  x = f.f[0];
  y = f.f[1];
  return *this;
}

inline vector2f& vector2f::operator+=(const vector2f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR v2 = XMLoadFloat2(&v);
  const XMVECTOR X = XMVectorAdd(v1, v2);
  XMStoreFloat2(this, X);
  return *this;
}

inline vector2f& vector2f::operator-=(const vector2f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR v2 = XMLoadFloat2(&v);
  const XMVECTOR X = XMVectorSubtract(v1, v2);
  XMStoreFloat2(this, X);
  return *this;
}

inline vector2f& vector2f::operator*=(const vector2f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR v2 = XMLoadFloat2(&v);
  const XMVECTOR X = XMVectorMultiply(v1, v2);
  XMStoreFloat2(this, X);
  return *this;
}

inline vector2f& vector2f::operator*=(float s) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR X = XMVectorScale(v1, s);
  XMStoreFloat2(this, X);
  return *this;
}

inline vector2f& vector2f::operator/=(float s) noexcept {
  if (s == 0.f)
    return *this;
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(this);
  const XMVECTOR X = XMVectorScale(v1, 1.f / s);
  XMStoreFloat2(this, X);
  return *this;
}

//------------------------------------------------------------------------------
// Unary operators
//------------------------------------------------------------------------------

inline vector2f vector2f::operator+() const noexcept {
  return *this;
}

inline vector2f vector2f::operator-() const noexcept {
  return { -x, -y };
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline vector2f operator+(const vector2f& v1, const vector2f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat2(&v1);
  const XMVECTOR vec2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorAdd(vec1, vec2);
  vector2f R;
  XMStoreFloat2(&R, X);
  return R;
}

inline vector2f operator-(const vector2f& v1, const vector2f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat2(&v1);
  const XMVECTOR vec2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorSubtract(vec1, vec2);
  vector2f R;
  XMStoreFloat2(&R, X);
  return R;
}

inline vector2f operator*(const vector2f& v1, const vector2f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat2(&v1);
  const XMVECTOR vec2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorMultiply(vec1, vec2);
  vector2f R;
  XMStoreFloat2(&R, X);
  return R;
}

inline vector2f operator*(const vector2f& v, float s) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(&v);
  const XMVECTOR X = XMVectorScale(v1, s);
  vector2f R;
  XMStoreFloat2(&R, X);
  return R;
}

inline vector2f operator/(const vector2f& v1, const vector2f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat2(&v1);
  const XMVECTOR vec2 = XMLoadFloat2(&v2);
  const XMVECTOR X = XMVectorDivide(vec1, vec2);
  vector2f R;
  XMStoreFloat2(&R, X);
  return R;
}

inline vector2f operator/(const vector2f& v, float s) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(&v);
  const XMVECTOR X = XMVectorScale(v1, 1.f / s);
  vector2f R;
  XMStoreFloat2(&R, X);
  return R;
}

inline vector2f operator*(float s, const vector2f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat2(&v);
  const XMVECTOR X = XMVectorScale(v1, s);
  vector2f R;
  XMStoreFloat2(&R, X);
  return R;
}

/****************************************************************************
 *
 * Vector3
 *
 ****************************************************************************/

//------------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------------

inline vector3f::vector3f() noexcept
  : XMFLOAT3() {
}

inline vector3f::vector3f(float ix) noexcept
  : XMFLOAT3(ix, ix, ix) {
}

inline vector3f::vector3f(float ix, float iz) noexcept
  : XMFLOAT3(ix, 0.f, iz) {
}

inline vector3f::vector3f(float ix, float iy, float iz) noexcept
  : XMFLOAT3(ix, iy, iz) {
}

inline vector3f::vector3f(const vector3f* vec) noexcept
  : XMFLOAT3(vec->x, vec->y, vec->z) {
}

inline vector3f::vector3f(const vector2f* vec) noexcept
  : XMFLOAT3(vec->x, 0.f, vec->y) {
}

inline vector3f::vector3f(const vector2f& vec) noexcept
  : XMFLOAT3(vec.x, 0.f, vec.y) {
}

inline vector3f::vector3f(const XMFLOAT3& v) noexcept
  : XMFLOAT3(v) {
}

inline vector3f::vector3f(DirectX::FXMVECTOR v) noexcept
  : XMFLOAT3() {
  XMStoreFloat3(this, v);
}

//------------------------------------------------------------------------------
// Casting
//------------------------------------------------------------------------------

inline vector3f::operator float*() noexcept {
  return &x;
}

inline vector3f::operator const float*() const noexcept {
  return &x;
}

//------------------------------------------------------------------------------
// Comparison operators
//------------------------------------------------------------------------------

inline bool vector3f::operator==(const vector3f& v) const noexcept {
  return this->x == v.x && this->z == v.z;
}

inline bool vector3f::operator!=(const vector3f& v) const noexcept {
  return this->x != v.x || this->z != v.z;
}

//------------------------------------------------------------------------------
// Assignment operators
//------------------------------------------------------------------------------

inline vector3f& vector3f::operator=(const DirectX::XMVECTORF32& f) noexcept {
  x = f.f[0];
  y = f.f[1];
  z = f.f[2];
  return *this;
}

inline vector3f& vector3f::operator+=(const vector3f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR v2 = XMLoadFloat3(&v);
  const XMVECTOR X = XMVectorAdd(v1, v2);
  XMStoreFloat3(this, X);
  return *this;
}

inline vector3f& vector3f::operator-=(const vector3f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR v2 = XMLoadFloat3(&v);
  const XMVECTOR X = XMVectorSubtract(v1, v2);
  XMStoreFloat3(this, X);
  return *this;
}

inline vector3f& vector3f::operator*=(const vector3f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR v2 = XMLoadFloat3(&v);
  const XMVECTOR X = XMVectorMultiply(v1, v2);
  XMStoreFloat3(this, X);
  return *this;
}

inline vector3f& vector3f::operator*=(float s) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR X = XMVectorScale(v1, s);
  XMStoreFloat3(this, X);
  return *this;
}

inline vector3f& vector3f::operator/=(float s) noexcept {
  if (s == 0.f)
    return *this;
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR X = XMVectorScale(v1, 1.f / s);
  XMStoreFloat3(this, X);
  return *this;
}

//------------------------------------------------------------------------------
// Unary operators
//------------------------------------------------------------------------------

inline vector3f vector3f::operator+() const noexcept {
  return *this;
}

inline vector3f vector3f::operator-() const noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(this);
  const XMVECTOR X = XMVectorNegate(v1);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}

//------------------------------------------------------------------------------
// Binary operators
//------------------------------------------------------------------------------

inline vector3f operator+(const vector3f& v1, const vector3f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat3(&v1);
  const XMVECTOR vec2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorAdd(vec1, vec2);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}

inline vector3f operator-(const vector3f& v1, const vector3f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat3(&v1);
  const XMVECTOR vec2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorSubtract(vec1, vec2);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}

inline vector3f operator*(const vector3f& v1, const vector3f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat3(&v1);
  const XMVECTOR vec2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorMultiply(vec1, vec2);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}

inline vector3f operator*(const vector3f& v, float s) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(&v);
  const XMVECTOR X = XMVectorScale(v1, s);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}

inline vector3f operator/(const vector3f& v1, const vector3f& v2) noexcept {
  using namespace DirectX;
  const XMVECTOR vec1 = XMLoadFloat3(&v1);
  const XMVECTOR vec2 = XMLoadFloat3(&v2);
  const XMVECTOR X = XMVectorDivide(vec1, vec2);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}

inline vector3f operator/(const vector3f& v, float s) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(&v);
  const XMVECTOR X = XMVectorScale(v1, 1.f / s);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}

inline vector3f operator*(float s, const vector3f& v) noexcept {
  using namespace DirectX;
  const XMVECTOR v1 = XMLoadFloat3(&v);
  const XMVECTOR X = XMVectorScale(v1, s);
  vector3f R;
  XMStoreFloat3(&R, X);
  return R;
}
