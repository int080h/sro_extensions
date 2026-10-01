#include "pch.hpp"
#include "sdk/ui/cninterface_manager.hpp"

#include "sdk/ui/cgwnd.hpp"
#include "utils/offsets.hpp"

namespace {

  using ext_client::off::as_fn;

} // namespace

auto cninterface_manager::get_instance() -> cninterface_manager* {
  return reinterpret_cast<cninterface_manager*>(0x01420408);
}

auto cninterface_manager::get_interface_obj_raw(int res_id) -> void* {
  // sub_4016F0: __thiscall(this, int key) → CNIFWnd* or 0
  using find_fn = int(__thiscall*)(void*, int);
  const auto fn = as_fn<find_fn>(0x004016F0);
  const auto raw = fn(this, res_id);
  if (raw == 0) {
    return nullptr;
  }
  return reinterpret_cast<void*>(raw);
}

auto cninterface_manager::find(int res_id) -> cgwnd* {
  auto* raw = get_interface_obj_raw(res_id);
  if (!raw) {
    return nullptr;
  }
  auto* wnd = reinterpret_cast<cgwnd*>(raw);
  return wnd && wnd->is_live() ? wnd : nullptr;
}

auto cninterface_manager::instantiate_dimensional(const char* filename, void* parent, bool b) -> void {
  // sub_401810: __thiscall(this, LPCSTR filename, void* parent, char b)
  using load_fn = int(__thiscall*)(void*, const char*, void*, char);
  const auto fn = as_fn<load_fn>(0x00401810);
  fn(this, filename, parent, b ? 1 : 0);
}

auto cninterface_manager::walk_roots(child_visitor_fn visit, void* ctx, int child_depth) -> void {
  if (!visit) {
    return;
  }
  ext_client::off::field_at<std::n_map<std::uint32_t, void*>>(this, 0x04).for_each([&](std::uint32_t /*key*/, void* value) {
    auto* wnd = reinterpret_cast<cgwnd*>(value);
    if (!wnd || !wnd->is_live()) {
      return;
    }
    visit(wnd, ctx);
    wnd->walk_each(child_depth, visit, ctx);
  });
}
