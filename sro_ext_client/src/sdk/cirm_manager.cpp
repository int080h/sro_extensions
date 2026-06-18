#include "cirm_manager.hpp"

namespace {

  using ext_client::offsets::global_at;

} // namespace

auto cirm_manager::get() -> cirm_manager* {
  if (!ext_client::msvc9::is_readable_ptr(reinterpret_cast<const void*>(ext_client::offsets::cirm_manager::globals::instance),
                                          sizeof(void*))) {
    return nullptr;
  }
  cirm_manager* mgr = global_at<cirm_manager*>(ext_client::offsets::cirm_manager::globals::instance);
  if (!mgr || !is_instance(mgr)) {
    return nullptr;
  }
  return mgr;
}

auto cirm_manager::is_instance(const void* ptr) -> bool {
  if (!ptr || !ext_client::msvc9::is_readable_ptr(ptr, sizeof(void*))) {
    return false;
  }
  return *reinterpret_cast<const std::uint32_t*>(ptr) == ext_client::offsets::cirm_manager::vtable::address;
}
