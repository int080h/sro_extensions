#include "cirm_manager.hpp"

namespace {

  using ext_client::offsets::as_fn;
  using ext_client::offsets::global_at;

} // namespace

auto cirm_manager::get() -> cirm_manager* {
  cirm_manager* mgr = global_at<cirm_manager*>(ext_client::offsets::cirm_manager::globals::instance);
  if (!mgr || !is_instance(mgr)) {
    return nullptr;
  }
  return mgr;
}

auto cirm_manager::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  const auto vtable = *reinterpret_cast<const std::uint32_t*>(ptr);
  return vtable == ext_client::offsets::cirm_manager::vtable::address;
}

auto cirm_manager::load_and_parse_file(const char* filename) -> void* {
  // sub_925B00: __thiscall(this, const char* filename) -> parsed document*
  using parse_fn = void*(__thiscall*)(void*, const char*);
  const auto fn = as_fn<parse_fn>(ext_client::offsets::cirm_manager::functions::load_and_parse_file);
  return fn(this, filename);
}

auto cirm_manager::section_map() const -> ext_client::msvc9::stdext_hash_map_ref {
  return ext_client::msvc9::stdext_hash_map_ref::from(&m_section_map);
}
