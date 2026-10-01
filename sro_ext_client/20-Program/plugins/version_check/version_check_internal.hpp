#pragma once

#include "core/core_event_manager.hpp"
#include "render/menu_builder.hpp"
#include "sdk/process/cps_version_check.hpp"
#include "sdk/ui/cif_static.hpp"

#include <Windows.h>
#include <d3d9.h>
#include <gdiplus.h>
#include <cstdint>

namespace ext_client::plugins::version_check {

template<typename T>
class com_ptr {
  T* m_ptr = nullptr;

public:
  com_ptr() = default;
  explicit com_ptr(T* ptr)
    : m_ptr(ptr) {}
  ~com_ptr() { reset(); }

  com_ptr(const com_ptr&) = delete;
  com_ptr& operator=(const com_ptr&) = delete;

  com_ptr(com_ptr&& other) noexcept
    : m_ptr(other.m_ptr) {
    other.m_ptr = nullptr;
  }
  com_ptr& operator=(com_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      m_ptr = other.m_ptr;
      other.m_ptr = nullptr;
    }
    return *this;
  }

  auto get() const -> T* { return m_ptr; }
  auto operator->() const -> T* { return m_ptr; }
  auto operator&() -> T** { return &m_ptr; }
  operator T*() const { return m_ptr; }

  auto reset(T* ptr = nullptr) -> void {
    if (m_ptr) {
      m_ptr->Release();
    }
    m_ptr = ptr;
  }

  auto release() -> T* {
    T* temp = m_ptr;
    m_ptr = nullptr;
    return temp;
  }
};

inline constexpr std::uint32_t k_cif_static_loading_banner_vftable = 0x00FF46BC;
inline constexpr std::uint32_t k_loading_banner_descriptor = 0x01179A58;
inline constexpr int k_max_banner_frames = 32;

struct loading_banner_state {
  char path[256]{};
  std::uint32_t widget_vftable = 0;
  std::uint32_t image_vftable = 0;
  void* texture = nullptr;
  bool path_read = false;
};

struct intro_renderer_state {
  void* vftable;
};

enum class intro_render_stage : int {
  d3d_setup = 1,
  wire_stages = 2,
};

struct window_style_state {
  LONG orig_style = 0;
  LONG orig_ex_style = 0;
  bool saved = false;
  RECT orig_rect{};
};

extern std::uint32_t g_last_banner_switch_time;
extern int g_current_banner_index;
extern cif_static* g_banner_widget;

extern cif_static* g_banner_frames[k_max_banner_frames];
extern int g_banner_frame_count;
extern Gdiplus::Bitmap* g_overlay_bitmaps[k_max_banner_frames];
extern int g_overlay_bitmap_count;

extern ULONG_PTR g_version_check_gdiplus_token;

extern window_style_state g_wnd_style;

extern int g_target_width;
extern int g_target_height;
extern bool g_target_saved;

extern bool g_intro_render_pipeline_ready;

auto is_version_check_active_process() -> bool;
auto resolve_loading_hwnd() -> HWND;
auto ensure_minimize_button(HWND hwnd) -> void;
auto query_d3d_texture(void* candidate) -> IDirect3DTexture9*;
auto find_d3d_texture_in_resource(void* resource) -> IDirect3DTexture9*;
auto start_overlay_for_window(HWND hwnd) -> void;
auto restore_window_style() -> void;
auto update_banner_cycle(cps_version_check* self) -> void;
auto banner_frame_at(int idx) -> cif_static*;
auto setup_login_render_pipeline() -> void;
auto shutdown_gdiplus() -> void;

auto handle_version_check_create(ext_client::core::event::version_check_create_context& ctx) -> void;
auto handle_version_check_update(ext_client::core::event::version_check_update_context& ctx) -> void;
auto handle_set_child_process(ext_client::core::event::set_child_process_context& ctx) -> void;
auto handle_load_intro_camera(ext_client::core::event::load_intro_camera_context& ctx) -> void;
auto handle_shutdown() -> void;
auto handle_menu(ext_client::render::menu::menu_builder& ui) -> void;
auto handle_tick() -> void;

} // namespace ext_client::plugins::version_check
