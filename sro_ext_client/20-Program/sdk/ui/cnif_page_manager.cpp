#include "pch.hpp"
#include "sdk/ui/cnif_page_manager.hpp"

namespace {

  using ext_client::off::as_fn;

} // namespace

auto cnif_page_manager::create_instance() -> cnif_page_manager* {
  using alloc_fn = cnif_page_manager*(__cdecl*)();
  const auto fn = as_fn<alloc_fn>(0x004302E0);
  return fn();
}

auto cnif_page_manager::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  const auto vtable = *reinterpret_cast<const std::uint32_t*>(ptr);
  return vtable == 0x01084CF4;
}

auto cnif_page_manager::get_page_id() const -> std::uint32_t {
  return ext_client::off::field_at<std::uint32_t>(this, 0x360);
}
