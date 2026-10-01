#pragma once



#include "sdk/ui/cg_font_texture.hpp"
#include "sdk/ui/cnif_text_board.hpp"
#include "utils/msvc9_stl.hpp"
#include "utils/offsets.hpp"

#include <cstddef>
#include <cstdint>

// CTextBoard — secondary MI base @ +0x84 on CIFWnd (size 0x140, vt 0xFF47F4).
class ctext_board {
public:
  auto get_texture() const -> void* {
    return ext_client::off::field_at<void*>(this, 0x0E0);
  }

  auto get_texture_path() const -> const char* {
    return ext_client::msvc9::string_ref::from(reinterpret_cast<const std::uint8_t*>(this) + 0x0E4).data();
  }

  auto copy_texture_path(char* out, std::size_t max_len) const -> bool {
    return ext_client::msvc9::string_ref::from(reinterpret_cast<const std::uint8_t*>(this) + 0x0E4).copy_to(out, max_len);
  }

  auto is_c_ani_fade_alpha_max() -> bool;
  auto set_c_ani_fade_alpha_max(bool val) -> void;
  auto is_b_render_texture() -> bool;
  auto set_b_render_texture(bool val) -> void;
  auto get_texture_h_align() -> halign_type;
  auto get_texture_v_align() -> valign_type;
  auto get_font_texture() -> cg_font_texture;
  auto get_field_90() -> std::uint32_t;
  auto get_field_94() -> std::uint32_t;
  auto get_dw_bg_font_color() -> std::uint32_t;
  auto get_field_9c() -> std::uint32_t;
  auto get_p_font_text_data() -> void*;
  auto get_field_a4() -> std::uint32_t;
  auto get_f_ani_fade_time() -> float;
  auto get_f_ani_fade_current_time() -> float;
  auto get_field_bc() -> std::uint32_t;
  auto get_field_c0() -> std::uint32_t;
  auto get_field_c8() -> std::uint8_t;
  auto get_wstr_font_texture() -> std::n_wstring;
  auto get_str_bground_texture_path() -> std::n_string;
  auto set_texture_h_align(halign_type val) -> void;
  auto set_texture_v_align(valign_type val) -> void;
  auto set_font_texture(cg_font_texture val) -> void;
  auto set_field_90(std::uint32_t val) -> void;
  auto set_field_94(std::uint32_t val) -> void;
  auto set_dw_bg_font_color(std::uint32_t val) -> void;
  auto set_field_9c(std::uint32_t val) -> void;
  auto set_p_font_text_data(void* val) -> void;
  auto set_field_a4(std::uint32_t val) -> void;
  auto set_f_ani_fade_time(float val) -> void;
  auto set_f_ani_fade_current_time(float val) -> void;
  auto set_field_bc(std::uint32_t val) -> void;
  auto set_field_c0(std::uint32_t val) -> void;
  auto set_field_c8(std::uint8_t val) -> void;
  auto set_wstr_font_texture(std::n_wstring val) -> void;
  auto set_str_bground_texture_path(std::n_string val) -> void;

};

