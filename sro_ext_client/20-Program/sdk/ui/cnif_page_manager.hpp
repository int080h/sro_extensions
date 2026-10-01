#pragma once

#include "sdk/ui/cnif_wnd.hpp"

#include <cstdint>

class cnif_page_manager : public cnif_wnd {
public:
  auto is_page_flag_a() -> bool;
  auto is_page_flag_b() -> bool;
  auto is_page_flag_c() -> bool;

  auto get_page_state_a() -> std::uint32_t;
  auto get_page_state_b() -> std::uint32_t;
  auto get_page_state_c() -> std::uint32_t;
  auto get_page_id() -> std::uint32_t;
  auto get_page_id() const -> std::uint32_t;
  auto get_page_color_a() -> std::uint32_t;
  auto get_page_color_b() -> std::uint32_t;
  auto get_page_embed_a() -> std::uint32_t;
  auto get_page_embed_b() -> std::uint32_t;
  auto get_page_embed_c() -> std::uint32_t;
  auto get_page_embed_d() -> std::uint32_t;
  auto get_page_embed_e() -> std::uint32_t;

  auto set_page_flag_a(bool val) -> void;
  auto set_page_flag_b(bool val) -> void;
  auto set_page_flag_c(bool val) -> void;
  auto set_page_state_a(std::uint32_t val) -> void;
  auto set_page_state_b(std::uint32_t val) -> void;
  auto set_page_state_c(std::uint32_t val) -> void;
  auto set_page_id(std::uint32_t val) -> void;
  auto set_page_color_a(std::uint32_t val) -> void;
  auto set_page_color_b(std::uint32_t val) -> void;
  auto set_page_embed_a(std::uint32_t val) -> void;
  auto set_page_embed_b(std::uint32_t val) -> void;
  auto set_page_embed_c(std::uint32_t val) -> void;
  auto set_page_embed_d(std::uint32_t val) -> void;
  auto set_page_embed_e(std::uint32_t val) -> void;

  static auto is_instance(const void* ptr) -> bool;

  static auto create_instance() -> cnif_page_manager*;
};
