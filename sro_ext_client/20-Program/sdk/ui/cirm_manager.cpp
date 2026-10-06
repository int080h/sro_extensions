#include "pch.hpp"
#include "sdk/ui/cirm_manager.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;
  using ext_client::off::global_at;
} // namespace

auto cirm_manager::get() -> cirm_manager* {
  cirm_manager* mgr = global_at<cirm_manager*>(0x0117ED1C);
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
  return vtable == 0x0102CB9C;
}

auto cirm_manager::get_raw() const -> const void* {
  return this;
}

auto cirm_manager::load_and_parse_file(const char* filename) -> void* {
  // sub_925B00: __thiscall(this, const char* filename) -> parsed document*
  using parse_fn = void*(__thiscall*)(void*, const char*);
  const auto fn = as_fn<parse_fn>(0x00925B00);
  return fn(this, filename);
}

auto cirm_manager::get_section_map_ref() const -> ext_client::msvc9::stdext_hash_map_ref {
  return ext_client::msvc9::stdext_hash_map_ref::from(&ext_client::off::field_at<section_map_t>(this, 0x04));
}
