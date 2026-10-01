#include "pch.hpp"
#include "sdk/game/cobj_render_manager.hpp"
#include "utils/offsets.hpp"

auto cobj_render_manager::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  const auto vtable = *reinterpret_cast<const std::uint32_t*>(ptr);
  return vtable == 0x01077254;
}

auto cobj_render_manager::get_vec_a_begin() -> void** {
  return &ext_client::off::field_at<void*>(this, 0x38);
}

auto cobj_render_manager::get_vec_a_end() -> void** {
  return &ext_client::off::field_at<void*>(this, 0x3C);
}

auto cobj_render_manager::get_vec_b_begin() -> void** {
  return &ext_client::off::field_at<void*>(this, 0x50);
}

auto cobj_render_manager::get_vec_b_end() -> void** {
  return &ext_client::off::field_at<void*>(this, 0x54);
}
