#pragma once

#include <Windows.h>
#include <d3d9.h>
#include <mutex>

namespace ext_client::render {

  class render_system {
  public:
    static auto get() -> render_system&;

    render_system(const render_system&) = delete;
    render_system& operator=(const render_system&) = delete;
    render_system(render_system&&) = delete;
    render_system& operator=(render_system&&) = delete;

    auto install() -> bool;
    auto uninstall() -> void;
    auto is_installed() const -> bool;
    auto is_imgui_ready() const -> bool;

    auto game_hwnd() const -> HWND;
    auto client_mouse_pos(int& x, int& y) const -> bool;
    auto apply_from_control() -> void;

  private:
    render_system() = default;
    ~render_system() = default;

    auto on_end_scene(IDirect3DDevice9* device) -> void;
    auto init_imgui(IDirect3DDevice9* device) -> void;

    bool m_imgui_ready = false;
    std::once_flag m_init_once;
  };

} // namespace ext_client::render
