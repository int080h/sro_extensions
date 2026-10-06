#include "pch.hpp"
#include "sdk/game/crt_skeleton.hpp"

#include "utils/memory.hpp"
#include "utils/offsets.hpp"

#include <cstring>

// ===========================================================================
// 1. Bones STL Vector View (Live game memory projection at this + 0x020)
// ===========================================================================
auto crt_skeleton::bones() const -> ext_client::msvc9::vector_view<crt_bone*> {
  if (!ext_client::utils::memory::is_valid_ptr(this)) {
    return {};
  }

  // Map std::vector<CRTBone*> at this + 0x020
  auto view = ext_client::msvc9::vector_view<crt_bone*>::from_field(this, 0x020);
  if (view.size() > 256) {
    return {}; // sanity check: Silkroad rigs have at most ~120 bones
  }
  return view;
}

// ===========================================================================
// 2. Query by Bone Name (e.g. "Bip01 Head", "Bip01 Spine")
// ===========================================================================
auto crt_skeleton::find_bone(const char* bone_name) const -> crt_bone* {
  if (!bone_name || bone_name[0] == '\0') {
    return nullptr;
  }

  crt_bone* found = nullptr;
  bones().for_each([&](crt_bone* b) -> bool {
    if (b && ext_client::utils::memory::is_valid_ptr(b)) {
      if (b->is_dummy()) {
        return true;
      }
      const char* bname = b->name();
      if (bname && _stricmp(bname, bone_name) == 0) {
        found = b;
        return false; // early exit on match
      }
    }
    return true;
  });

  return found;
}
