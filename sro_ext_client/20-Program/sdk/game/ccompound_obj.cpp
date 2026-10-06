#include "pch.hpp"
#include "sdk/game/ccompound_obj.hpp"
#include "sdk/game/crt_skeleton.hpp"

#include "utils/memory.hpp"
#include "utils/offsets.hpp"
#include "utils/msvc9_stl.hpp"

// ===========================================================================
// 1. Identity & File Paths
// ===========================================================================
auto ccompound_obj::name() const -> const char* {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return "";
  }
  using string_ref = ext_client::msvc9::string_ref;
  const auto* str_ptr = &ext_client::off::field_at<char>(this, 0x054);
  return string_ref::from(str_ptr).data();
}

auto ccompound_obj::path() const -> const char* {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return "";
  }
  using string_ref = ext_client::msvc9::string_ref;
  const auto* str_ptr = &ext_client::off::field_at<char>(this, 0x0D4);
  return string_ref::from(str_ptr).data();
}

// ===========================================================================
// 2. Alpha & Blending State
// ===========================================================================
auto ccompound_obj::alpha() const -> float {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return 1.0f;
  }
  return ext_client::off::field_at<float>(this, 0x07C);
}

auto ccompound_obj::alpha_mode() const -> int {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return 0;
  }
  return ext_client::off::field_at<int>(this, 0x080);
}

auto ccompound_obj::set_alpha(float alpha, int force) -> void {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return;
  }
  using set_alpha_fn = void(__thiscall*)(ccompound_obj* this_ptr, float alpha, int force);
  auto* self = const_cast<ccompound_obj*>(this);
  auto** vtable = *reinterpret_cast<void***>(self);
  if (!ext_client::utils::memory::is_game_ptr(vtable)) {
    return;
  }
  const auto set_alpha_func = reinterpret_cast<set_alpha_fn>(vtable[2/*set_alpha*/]);
  if (set_alpha_func) {
    set_alpha_func(self, alpha, force);
  }
}

// ===========================================================================
// 3. Transform & World Matrix
// ===========================================================================
auto ccompound_obj::forward_vector() const -> vector3f {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return vector3f(0.0f, 0.0f, 0.0f);
  }
  const auto* vec = &ext_client::off::field_at<float>(this, 0x0A0);
  return vector3f(vec[0], vec[1], vec[2]);
}

auto ccompound_obj::world_matrix() const -> const D3DMATRIX* {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return nullptr;
  }
  const auto* ptr = ext_client::off::field_at<const D3DMATRIX*>(this, 0x0AC);
  if (!ext_client::utils::memory::is_game_ptr(ptr)) {
    return nullptr;
  }
  return ptr;
}

auto ccompound_obj::set_world_matrix(const D3DMATRIX* matrix) -> void {
  if (!ext_client::utils::memory::is_game_ptr(this) || !ext_client::utils::memory::is_game_ptr(matrix)) {
    return;
  }
  using set_world_matrix_fn = void(__thiscall*)(ccompound_obj* this_ptr, const D3DMATRIX* matrix);
  auto* self = const_cast<ccompound_obj*>(this);
  auto** vtable = *reinterpret_cast<void***>(self);
  if (!ext_client::utils::memory::is_game_ptr(vtable)) {
    return;
  }
  const auto set_world_matrix_func = reinterpret_cast<set_world_matrix_fn>(vtable[3/*set_world_matrix*/]);
  if (set_world_matrix_func) {
    set_world_matrix_func(self, matrix);
  }
}

// ===========================================================================
// 4. Skeleton & Bone Hierarchy
// ===========================================================================
auto ccompound_obj::skeleton() const -> crt_skeleton* {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return nullptr;
  }
  // this + 0x0F4: Skeleton manager / list container
  auto* mgr = ext_client::off::field_at<void*>(this, 0x0F4);
  if (!ext_client::utils::memory::is_valid_ptr(mgr)) {
    return nullptr;
  }

  // Call engine sub_E646F0 (0x00E646F0)
  using get_skeleton_fn = crt_skeleton*(__thiscall*)(void* mgr);
  const auto fn = ext_client::off::as_fn<get_skeleton_fn>(0x00E646F0);
  if (fn) {
    auto* skel = fn(mgr);
    if (ext_client::utils::memory::is_valid_ptr(skel)) {
      return skel;
    }
  }

  // Direct memory read fallback (MSVC9 std::list head at +0x18, size at +0x1C)
  const auto list_size = ext_client::off::field_at<std::uint32_t>(mgr, 0x01C);
  if (list_size > 0 && list_size < 100) {
    auto* head = ext_client::off::field_at<void**>(mgr, 0x018);
    if (ext_client::utils::memory::is_valid_ptr(head)) {
      auto* first_node = *head;
      if (first_node && first_node != head && ext_client::utils::memory::is_valid_ptr(first_node)) {
        // Node value at offset +0x08 (void* [2])
        auto* skel = reinterpret_cast<crt_skeleton**>(first_node)[2];
        if (ext_client::utils::memory::is_valid_ptr(skel)) {
          return skel;
        }
      }
    }
  }

  return nullptr;
}

auto ccompound_obj::get_bone_matrix(const char* bone_name) const -> const D3DMATRIX* {
  if (!ext_client::utils::memory::is_game_ptr(this) || !bone_name || bone_name[0] == '\0') {
    return nullptr;
  }

  // Fast path: search through primary crt_skeleton
  auto* skel = skeleton();
  if (skel) {
    auto* b = skel->find_bone(bone_name);
    if (b) {
      return b->combined_matrix();
    }
  }

  // Fallback: invoke virtual method 17 (0x00E31980)
  auto* self = const_cast<ccompound_obj*>(this);
  auto** vtable = *reinterpret_cast<void***>(self);
  if (ext_client::utils::memory::is_game_ptr(vtable)) {
    ext_client::msvc9::string str_name(bone_name);
    using get_bone_matrix_fn = const D3DMATRIX*(__thiscall*)(const ccompound_obj*, const ext_client::msvc9::string*);
    const auto fn = reinterpret_cast<get_bone_matrix_fn>(vtable[17]);
    if (fn) {
      return fn(this, &str_name);
    }
  }

  return nullptr;
}

// ===========================================================================
// 5. 3D Bounding Box (AABB in local model space)
// ===========================================================================
auto ccompound_obj::bounding_box(vector3f& out_min, vector3f& out_max) const -> bool {
  if (!ext_client::utils::memory::is_game_ptr(this)) {
    return false;
  }
  // this + 0x0B0: float min[3] (0xB0, 0xB4, 0xB8)
  // this + 0x0BC: float max[3] (0xBC, 0xC0, 0xC4)
  const auto* min_ptr = &ext_client::off::field_at<float>(this, 0x0B0);
  const auto* max_ptr = &ext_client::off::field_at<float>(this, 0x0BC);
  if (!ext_client::utils::memory::is_readable_ptr(min_ptr) ||
      !ext_client::utils::memory::is_readable_ptr(max_ptr)) {
    return false;
  }

  // Sanity check: max must be >= min, and not infinite/NaN
  if (max_ptr[0] < min_ptr[0] || max_ptr[1] < min_ptr[1] || max_ptr[2] < min_ptr[2]) {
    return false;
  }
  if (max_ptr[0] - min_ptr[0] > 10000.0f || max_ptr[1] - min_ptr[1] > 10000.0f || max_ptr[2] - min_ptr[2] > 10000.0f) {
    return false;
  }

  out_min = vector3f(min_ptr[0], min_ptr[1], min_ptr[2]);
  out_max = vector3f(max_ptr[0], max_ptr[1], max_ptr[2]);
  return true;
}

