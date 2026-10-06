#pragma once

#include "sdk/game/crt_bone.hpp"
#include "utils/memory.hpp"
#include "utils/msvc9_stl.hpp"

#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------------
// CRTSkeleton — Joymax RenderTech Model Rig / Skeleton Instance
// Native VTable: 0x01079124
// ---------------------------------------------------------------------------
class crt_skeleton {
public:
  static constexpr std::uint32_t k_vtable_addr = 0x01079124;

  // 1. Bones STL Vector View (Live game memory projection at this + 0x020)
  auto bones() const -> ext_client::msvc9::vector_view<crt_bone*>;

  // Convenience accessors delegating to vector_view
  auto bone_count() const -> std::size_t { return bones().size(); }
  auto get_bone(std::size_t index) const -> crt_bone* {
    auto* ptr = bones().safe_at(index);
    return ptr ? *ptr : nullptr;
  }
  auto bones_begin() const -> crt_bone* const* { return bones().begin(); }
  auto bones_end() const -> crt_bone* const* { return bones().end(); }

  // 2. Query by Bone Name (e.g. "Bip01 Head", "Bip01 Spine")
  auto find_bone(const char* bone_name) const -> crt_bone*;

  // 3. Iteration Helper
  template<typename Func>
  auto for_each_bone(Func&& fn) const -> void {
    bones().for_each([&](crt_bone* b) -> bool {
      if (b && ext_client::utils::memory::is_game_ptr(b)) {
        if constexpr (std::is_invocable_r_v<bool, Func, crt_bone*>) {
          return fn(b);
        } else {
          fn(b);
          return true;
        }
      }
      return true;
    });
  }
};
