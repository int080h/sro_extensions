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
  if (!ptr ||
      !ext_client::msvc9::is_readable_ptr(ptr, ext_client::offsets::cnif_page_manager::fields::textboard_vftable + sizeof(std::uint32_t))) {
    return false;
  }
  const auto* bytes = reinterpret_cast<const std::uint8_t*>(ptr);
  if (*reinterpret_cast<const std::uint32_t*>(bytes) != ext_client::offsets::cnif_page_manager::vtable::address) {
    return false;
  }
  const auto secondary =
      *reinterpret_cast<const std::uint32_t*>(bytes + ext_client::offsets::cnif_page_manager::fields::textboard_vftable);
  return secondary == ext_client::offsets::cnif_page_manager::vtable::secondary;
}
