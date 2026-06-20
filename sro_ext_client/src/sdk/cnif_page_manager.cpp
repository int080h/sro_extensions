#include "cnif_page_manager.hpp"

namespace {

  using ext_client::offsets::as_fn;

} // namespace

auto cnif_page_manager::create_instance() -> cnif_page_manager* {
  using alloc_fn = cnif_page_manager*(__cdecl*)();
  const auto fn = as_fn<alloc_fn>(ext_client::offsets::cnif_page_manager::functions::allocate);
  return fn();
}

auto cnif_page_manager::is_instance(const void* ptr) -> bool {
  if (!ptr) {
    return false;
  }
  const auto vtable = *reinterpret_cast<const std::uint32_t*>(ptr);
  return vtable == ext_client::offsets::cnif_page_manager::vtable::address;
}
