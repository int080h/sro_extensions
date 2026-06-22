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

private:
  union {
    DEFINE_MEMBER_0(std::uint32_t m_page_state_a, "page_state_a");
    DEFINE_MEMBER_N(std::uint32_t m_page_state_b, 0x04);
    DEFINE_MEMBER_N(std::uint32_t m_page_state_c, 0x08);
    DEFINE_MEMBER_N(std::uint8_t m_page_flag_a, 0x0C);
    DEFINE_MEMBER_N(std::uint8_t m_page_flag_b, 0x0D);
    DEFINE_MEMBER_N(std::uint8_t m_page_flag_c, 0x0E);
    DEFINE_MEMBER_N(std::uint32_t m_page_id, 0x14);
    DEFINE_MEMBER_N(std::uint32_t m_page_color_a, 0x18);
    DEFINE_MEMBER_N(std::uint32_t m_page_color_b, 0x1C);
    DEFINE_MEMBER_N(std::uint32_t m_page_embed_a, 0x20);
    DEFINE_MEMBER_N(std::uint32_t m_page_embed_b, 0x24);
    DEFINE_MEMBER_N(std::uint32_t m_page_embed_c, 0x28);
    DEFINE_MEMBER_N(std::uint32_t m_page_embed_d, 0x2C);
    DEFINE_MEMBER_N(std::uint32_t m_page_embed_e, 0x30);
    DEFINE_MEMBER_0(std::uint8_t m_pad_end[ext_client::offsets::cnif_page_manager::size - sizeof(cnif_wnd)], "pad_end");
  };
};

