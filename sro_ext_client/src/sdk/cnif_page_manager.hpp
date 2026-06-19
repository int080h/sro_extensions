#pragma once

#include "cnif_wnd.hpp"
#include "utils/offsets.hpp"

#include <cstdint>

struct cnif_page_manager_vtable {
  VFN_CDECL(get_res, void*);
  VFN_THISCALL(on_timer, void, void* self, float delta);
  VFN_THISCALL(scalar_deleting_dtor, void*, void* self, char should_free);
  VFN_THISCALL(on_post_ctor, void, void* self);
  VFN_THISCALL(equals, int, void* self, void* other);
  VFN_THISCALL(null_05, void, void* self);
  VFN_CDECL(create_instance, void*);
  VFN_THISCALL(on_create, int, void* self, int mode);
  VFN_THISCALL(on_destroy, int, void* self, int mode);
  VFN_THISCALL(on_init, int, void* self, int mode);
  VFN_THISCALL(null_10, int, void* self);
  VFN_THISCALL(null_11, int, void* self);
  VFN_THISCALL(null_12, void, void* self);
  VFN_THISCALL(on_cmd, int, void* self, int cmd);
  VFN_THISCALL(on_render, int, void* self, int pass);
  VFN_THISCALL(null_15, int, void* self);
  VFN_THISCALL(run_update_chain, int, void* self);
  VFN_THISCALL(on_wnd_message, int, void* self, int msg);
  VFN_THISCALL(on_timer_id, int, void* self, int timer_id);
  VFN_THISCALL(on_size, int, void* self, int size_param);
  VFN_THISCALL(null_19, int, void* self);
  VFN_THISCALL(on_move, int, void* self, int move_param);
  VFN_THISCALL(on_show, int, void* self, int show_param);
  VFN_THISCALL(on_hide, int, void* self, int hide_param);
  VFN_THISCALL(set_visible, int, void* self, std::uint8_t visible);
};

class cnif_page_manager : public cnif_wnd {
public:
  static auto create_instance() -> cnif_page_manager*;
  static auto is_instance(const void* ptr) -> bool;

  DECLARE_SDK_CAST(cgwnd, as_gwnd)

  auto page_id() const -> std::uint32_t { return m_page_id; }

  std::uint32_t m_page_state_a;
  std::uint32_t m_page_state_b;
  std::uint32_t m_page_state_c;
  std::uint8_t m_page_flag_a;
  std::uint8_t m_page_flag_b;
  std::uint8_t m_page_flag_c;
  PAD_TO(ext_client::offsets::cnif_page_manager::fields::page_flag_c + sizeof(std::uint8_t),
         ext_client::offsets::cnif_page_manager::fields::page_id);
  std::uint32_t m_page_id;
  std::uint32_t m_page_color_a;
  std::uint32_t m_page_color_b;
  std::uint32_t m_page_embed_a;
  std::uint32_t m_page_embed_b;
  std::uint32_t m_page_embed_c;
  std::uint32_t m_page_embed_d;
  std::uint32_t m_page_embed_e;
  PAD_TO(ext_client::offsets::cnif_page_manager::fields::page_embed_e + sizeof(std::uint32_t),
         ext_client::offsets::cnif_page_manager::size);
};

static_assert(sizeof(cnif_page_manager) == ext_client::offsets::cnif_page_manager::size, "cnif_page_manager size mismatch");
static_assert(offsetof(cnif_page_manager, m_page_state_a) == ext_client::offsets::cnif_page_manager::fields::page_state_a);
static_assert(offsetof(cnif_page_manager, m_page_id) == ext_client::offsets::cnif_page_manager::fields::page_id);
