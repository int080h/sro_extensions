#include "cirm_manager.hpp"

namespace {

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
