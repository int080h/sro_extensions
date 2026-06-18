#include "cobj_render_manager.hpp"

auto cobj_render_manager::is_instance(const void* ptr) -> bool {
  if (!ptr || !ext_client::msvc9::is_readable_ptr(ptr, sizeof(void*))) {
    return false;
  }
  return *reinterpret_cast<const std::uint32_t*>(ptr) == ext_client::offsets::cobj_render_manager::vtable::address;
}
