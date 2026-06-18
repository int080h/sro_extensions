#include "cres_id_manager.hpp"

namespace {

  using ext_client::offsets::as_fn;

} // namespace

auto cres_id_manager::is_instance(const void* ptr) -> bool {
  if (!ptr || !ext_client::msvc9::is_readable_ptr(ptr, ext_client::msvc9::ui_res_map_size)) {
    return false;
  }
  const auto vtable = *reinterpret_cast<const std::uint32_t*>(ptr);
  return vtable == ext_client::offsets::cres_id_manager::vtable::address;
}

auto cres_id_manager::from(void* ptr) -> cres_id_manager* {
  return reinterpret_cast<cres_id_manager*>(ptr);
}

auto cres_id_manager::from(const void* ptr) -> const cres_id_manager* {
  return reinterpret_cast<const cres_id_manager*>(ptr);
}

auto cres_id_manager::find(int res_id, bool add_base_key) const -> void* {
  // sub_9CF790 returns the widget pointer in EAX (int), not a C++ void*.
  using find_fn = int(__thiscall*)(const void*, int, int);
  const auto fn = as_fn<find_fn>(ext_client::offsets::cres_id_manager::functions::find);
  const int result = fn(storage_, res_id, add_base_key ? 1 : 0);
  auto* wnd = reinterpret_cast<void*>(result);
  return ext_client::msvc9::is_game_ptr(wnd) ? wnd : nullptr;
}

auto cres_id_manager::map_view() const -> ext_client::msvc9::map_ref {
  return ext_client::msvc9::map_ref::from(storage_);
}
