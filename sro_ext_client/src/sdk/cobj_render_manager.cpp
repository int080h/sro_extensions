#include "cobj_render_manager.hpp"

auto cobj_render_manager::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  const auto vtable = *reinterpret_cast<const std::uint32_t*>(ptr);
  return vtable == ext_client::offsets::cobj_render_manager::vtable::address;
}
