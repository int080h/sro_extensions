#include "pch.hpp"
#include "sdk/game/crt_bone.hpp"
#include "sdk/game/crt_skeleton.hpp"

#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

// ===========================================================================
// 1. Identity & Hierarchy
// ===========================================================================
auto crt_bone::name() const -> const char* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return "";
  }

  // this + 0x04: CPrimBone* (Bone metadata & definition node)
  const void* prim = ext_client::off::field_at<const void*>(this, 0x004);
  if (!ext_client::utils::memory::is_valid_ptr(prim)) {
    return "";
  }

  auto** vtable = *reinterpret_cast<void***>(const_cast<void*>(prim));
  if (!ext_client::utils::memory::is_valid_ptr(vtable)) {
    return "";
  }

  // vtable[2] is GetName() -> returns pointer to msvc9::string
  using get_name_fn = const void*(__thiscall*)(const void*);
  const auto get_name = reinterpret_cast<get_name_fn>(vtable[2]);
  if (!get_name) {
    return "";
  }

  const void* str_obj = get_name(prim);
  if (!ext_client::utils::memory::is_valid_ptr(str_obj)) {
    return "";
  }

  return ext_client::msvc9::string_ref::from(str_obj).data();
}

auto crt_bone::is_dummy() const -> bool {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return true;
  }
  // this + 0x94 is 1 if it is a dummy socket helper (e.g. starts with '[')
  return ext_client::off::field_at<std::uint8_t>(this, 0x094) != 0;
}

auto crt_bone::parent() const -> crt_bone* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }
  // this + 0x0C: parent CRTBone* (nullptr if root bone)
  auto* p = ext_client::off::field_at<crt_bone*>(this, 0x00C);
  return ext_client::utils::memory::is_valid_ptr(p) ? p : nullptr;
}

auto crt_bone::skeleton() const -> crt_skeleton* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }
  // this + 0x08: parent CRTSkeleton*
  auto* s = ext_client::off::field_at<crt_skeleton*>(this, 0x008);
  return ext_client::utils::memory::is_valid_ptr(s) ? s : nullptr;
}

// ===========================================================================
// 2. Matrices & Transforms
// ===========================================================================
auto crt_bone::local_matrix() const -> const D3DMATRIX* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }
  const auto* node = ext_client::off::field_at<const std::uint8_t*>(this, 0x080);
  if (!ext_client::utils::memory::is_valid_ptr(node)) {
    return nullptr;
  }
  return reinterpret_cast<const D3DMATRIX*>(node);
}

auto crt_bone::combined_matrix() const -> const D3DMATRIX* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }
  const auto* node = ext_client::off::field_at<const std::uint8_t*>(this, 0x080);
  if (!ext_client::utils::memory::is_valid_ptr(node)) {
    return nullptr;
  }
  // node + 0x40 is the combined model-space transform matrix
  return reinterpret_cast<const D3DMATRIX*>(node + 0x040);
}

// ===========================================================================
// 3. Positions
// ===========================================================================
auto crt_bone::local_position() const -> vector3f {
  const auto* m = combined_matrix();
  if (!m) {
    return vector3f(0.0f, 0.0f, 0.0f);
  }
  return vector3f(m->_41, m->_42, m->_43);
}

auto crt_bone::world_position(const D3DMATRIX* compound_world_matrix) const -> vector3f {
  const auto local = local_position();
  if (!compound_world_matrix || !ext_client::utils::memory::is_valid_ptr(compound_world_matrix)) {
    return local;
  }

  // Row vector transformation: V_world = V_local * M_world
  const auto& w = *compound_world_matrix;
  return vector3f(
    local.x * w._11 + local.y * w._21 + local.z * w._31 + w._41,
    local.x * w._12 + local.y * w._22 + local.z * w._32 + w._42,
    local.x * w._13 + local.y * w._23 + local.z * w._33 + w._43
  );
}
