#include "calarm_store.hpp"

#include "utils/offsets.hpp"

namespace {

  using ext_client::offsets::as_fn;

} // namespace

auto calarm_store::alarm_data() -> calarm_data* {
  using alarm_data_ptr_fn = calarm_data*(__thiscall*)(calarm_store * self);
  const auto fn = as_fn<alarm_data_ptr_fn>(ext_client::offsets::calarm_store::functions::alarm_data_ptr);
  return fn(this);
}

auto calarm_store::alarm_data() const -> const calarm_data* {
  return const_cast<calarm_store*>(this)->alarm_data();
}

auto calarm_store::from_interface(void* cg_interface) -> calarm_store* {
  if (!cg_interface) {
    return nullptr;
  }
  using from_interface_fn = calarm_store*(__thiscall*)(void* cg_interface);
  const auto fn = as_fn<from_interface_fn>(ext_client::offsets::calarm_store::functions::from_interface);
  return reinterpret_cast<calarm_store*>(fn(cg_interface));
}
